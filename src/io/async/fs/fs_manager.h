#ifndef CYNNYPP_FS_MANAGER_H_H
#define CYNNYPP_FS_MANAGER_H_H

#include "fs_manager_interface.h"
#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <boost/asio.hpp>
#include <queue>
#include <functional>
#include <tuple>
#include <type_traits>
#include <memory>
#include <map>


namespace cynny {
namespace cynnypp {

namespace filesystem {

using namespace cynny::cynnypp::utilities;

// Path could be replaced with boost::filesystem::path or something similar if needed
using pos_type = std::basic_fstream<uint8_t>::pos_type;


// fwd declaration
class FilesystemManager;

/**
 * @brief Implementation details used by the FilesystemManager
 */
namespace impl {

using ReadChunkHandler = FilesystemManagerInterface::ReadChunkHandler;

/**
 * @brief The HotDoubleBuffer class is an implementation of a double buffer
 * with a check for not writing on the buffer before someone has consumed it.
 */
class HotDoubleBuffer {
public:

    /**
     * @brief The BufferView class is an helper class that represents the view of the "current" buffer
     * inside the double buffer.
     *
     * It wraps the pair (pointer, size) representing the buffer,
     * a boolean (is_hot) that tells whether the buffer has not been consumed by the user,
     * and an ErrorCode that records if an error happend while filling the buffer (i.e. while reading).
     * BufferView is convertible to Buffer; this conversion create a brand-new buffer whose content
     * is the copy of the BufferView.
     */
    class BufferView {
    public:
        using pointer = uint8_t*;

        BufferView(HotDoubleBuffer& db, bool swapped)
            : db(db)
            , swapped(swapped)
        {}

        operator Buffer() const { return Buffer{db.ptrs[swapped], db.ptrs[swapped] + db.sizes[swapped]}; }

        pointer data() { return db.ptrs[swapped]; }
        const pointer data() const { return db.ptrs[swapped]; }
        size_t size() const { return db.sizes[swapped]; }
        bool is_hot() const { return db.hot[swapped].load(); }
        ErrorCode& error_code() { return db.ec[swapped]; }
        const ErrorCode& error_code() const { return db.ec[swapped]; }

        void set_hot(bool h) { db.hot[swapped].store(h); }
        void resize(size_t s)
        {
            assert(s <= db.max_size());
            db.sizes[swapped] = s;
        }
    private:
        HotDoubleBuffer& db;
        bool swapped;
    };

    HotDoubleBuffer(size_t single_buf_size)
        : buf(2 * single_buf_size)
        , ptrs{buf.data(), buf.data() + single_buf_size}
        , sizes{single_buf_size, single_buf_size}
        , hot{}
        , ec{}
        , swapped{}
    {}
    HotDoubleBuffer(const HotDoubleBuffer&) = delete;
    HotDoubleBuffer(HotDoubleBuffer&&) = default;
    HotDoubleBuffer& operator=(const HotDoubleBuffer&) = delete;
    HotDoubleBuffer& operator=(HotDoubleBuffer&&) = default;

    /**
     * @brief get_and_swap return the BufferView of the current buffer
     * and then set as current the other buffer.
     * @return a BufferView representing the current buffer at the time
     * of the function call
     */
    BufferView get_and_swap()
    {
        BufferView ret{*this, swapped};
        swapped = !swapped;
        return ret;
    }

    /**
     * @brief max_size returns the size beyond which the single buffer can't grow.
     * @return
     */
    size_t max_size() const { return buf.size() / 2; }

private:
    Buffer buf;
    std::array<uint8_t*,2> ptrs;
    std::array<size_t,2> sizes;
    std::array<std::atomic_bool,2> hot;
    std::array<ErrorCode,2> ec;
    bool swapped;
};


/**
 * @brief The ChunkedReader class asynchronously reads files from the disk chunk by chunk,
 * implementing prefetching using a HotDoubleBuffer.
 *
 * ChunkedReader uses the FilesystemManager worker thread to perform reads and returns
 * read data to the calling thread passing them to callbacks.
 */
class ChunkedReader : public std::enable_shared_from_this<ChunkedReader> {
public:
    ChunkedReader(FilesystemManager& fs, const Path& p, size_t chunk_size = default_chunk_size);
    ChunkedReader(const ChunkedReader&) = delete;
    ChunkedReader(ChunkedReader&&) = default;
    ChunkedReader& operator=(const ChunkedReader&) = delete;
    ChunkedReader& operator=(ChunkedReader&&) = default;

    /**
     * @brief next_chunk asynchronously reads another chunk and passes it (by copy) to h when done
     * @param h the completion handler for the read operation
     */
    void next_chunk(ReadChunkHandler h);

    /**
     * @brief read_file_chunk synchronously reads another chunk from the opened file.
     * @param buf the double buffer to be used to perform the read (in case the chunk has already been prefetched)
     * @param chunk_size the size of the chunk to be read
     * @param pos the position in the file where to start the read
     * @return a Buffer containing bytes read
     */
    Buffer read_file_chunk(HotDoubleBuffer::BufferView& buf, size_t chunk_size, size_t pos);
    void stop() { stopped = true; }

    size_t bytes_read() const { return bytes_read_; }
    bool is_stopped() const { return stopped; }
    bool eof() const { assert(bytes_read_ <= file_size); return bytes_read_ == file_size; }

    static const size_t default_chunk_size;

private:
    FilesystemManager& fs_manager;

    const Path path;
    std::basic_ifstream<uint8_t> is;
    const pos_type file_size;

    pos_type pos_to_schedule; // used only by "main" thread
    pos_type bytes_read_;  // used only by fs_manager thread
    size_t n_enqueued; // used only by "main" thread

    HotDoubleBuffer buf_;
    std::queue<ReadChunkHandler> q_handlers;
    std::queue<HotDoubleBuffer::BufferView> q_buf_ready;

    bool stopped;
};

}



// -----------------------------------   FilesystemManager for asynchrounous I/O

/**
 * @brief The FilesystemManager class implements an asynchronous interface to the filesystem
 */
class FilesystemManager : public FilesystemManagerInterface {
public:
    FilesystemManager(boost::asio::io_service& io);
    FilesystemManager(const FilesystemManager&) = delete;
    FilesystemManager(FilesystemManager&&) = default;

    FilesystemManager& operator=(FilesystemManager &&) = default;
    ~FilesystemManager() override;

    //-------------------------------------------    accessors functions

    boost::asio::io_service& get_io_service() { return io_; }


    //-------------------------------------------    operational functions

    /**
     * Checks whether a file or a directory exists.
     *
     * \param p path to a file or directory
     *
     * \returns true if the file or directory exists, false otherwise
     *
     * \throws If some filesystem error occurs, or if p is a symlink or not a directory or a regular file, a FilesystemError is thrown
     */
    bool exists(const Path& p)override;

    /**
     * Remove a file from the filesystem.
     *
     * If file p does not exists, the function does NOT throw and returns false
     *
     *  \param p path to the file
     *
     * \throws If some filesystem error occurs, or if p is a symlink or not a regular file, a FilesystemError is thrown
     */
    bool removeFile(const Path& p) override;

    /**
     * Move a file or a directory to a different path.
     *
     * \param from path to the file or directory to be moved
     * \param to path to the file or directory to move to
     *
     * \throws If some filesystem error occurs, or if form or to is a symlink or not a directory or a regular file, a FilesystemError is thrown
     */
    void move(const Path& from, const Path& to) override;

    /**
     * Copy a file to another path on the filesystem.
     *
     * If to is a (existing) directory, from is copied inside to.
     *
     * \param from - path to the file to be moved
     * \param to - path to move the file to
     *
     * \throws If some filesystem error occurs, or if from or to is a symlink or not a regular file, a FilesystemError is thrown
     */
    void copyFile(const Path& from, const Path& to) override;

    /**
     * Recursively copy a directory to another path on the filesystem.
     *
     * The recursive copy ignores everything that is not a regular file or a directory
     *
     * \param from - path to the directory to be copied
     * \param to - path to copy the directory to
     *
     * \throws If some filesystem error occurs, or if from or to is a symlink or not a directory, a FilesystemError is thrown
     */
    void copyDirectory(const Path& from, const Path& to) override;

    /**
     * Remove a directory from the filesystem.
     *
     *  \param p path to the directory
     *
     * If directory p does not exists, the function does NOT throw and returns 0
     *
     *  \returns The number of files removed
     *
     * \throws If some filesystem error occurs, or if p is a symlink or not a regular directory, a FilesystemError is thrown
     */
    uintmax_t removeDirectory(const Path& p) override;

    /**
     * @brief createDirectory
     * Create the DIRECTORY(ies), if they do not already exist.
     *
     * If parents is true, no exception is thrown when the directory already exists,
     * or when the parent directories need to be created and parent directories are
     * made as needed.
     *
     * @param p - path of the directory to create
     * @param parents - no error if existing, make parent directories as needed
     *
     * @return true if a directory was created, false otherwise
     *
     * @throws If p already exists and it's not a directory, an ErrorCode is thrown.
     * When parents is false, if p already exists or p is inside a not existing directory,
     * an ErrorCode is thrown.
     */
    bool createDirectory(const Path& p, bool parents) override;

    // file access

    /**
     * Read a whole file from the filesystem and return its content as a vector of chars.
     *
     * \param p - path to the file to be read
     *
     * \returns a vector of chars containing the file content
     *
     * \throws If some filesystem error occurs, or if p is a symlink or not a  a regular file, a FilesystemError is thrown
     * \throws If there is an error in allocating memory for the returned vector, an exception may be thrown
     */
    Buffer readFile(const Path& p) override;


    /**
     * Write a buffer to file
     *
     * \param p - path to the file to write
     * \param buf - buffer of chars to be written
     *
     * \throws If some filesystem error occurs, or if p is not a regular file, a FilesystemError is thrown
     */
    void writeFile(const Path& p, const Buffer& buf) override;


    /**
     * Append a vector of chars to a file.
     *
     * \param p - path to the file to append to
     * \param bytes - a vector of chars append to p
     *
     * \throws If some filesystem error occurs, or if p is a symlink or not a directory or a regular file, a FilesystemError is thrown
     */
    void appendToFile(const Path& p, const Buffer& bytes) override;



    //--------------------------------------------- asynchronous interface

    /**
     * Register an asynch read request to the fs manager.
     *
     * The request will be enqueued on the task queue of the fs manager and the read operation will be performed
     * on the worker thread as soon as all previous pending tasks will have been completed.
     *
     * It's very important to note that the caller should never pass a filesystem I/O operation
     * inside the completion handler. In fact, the CompletionHandler run on the "main/application" thread,
     * while all I/O fs operation should be enqueued on the worker thread of the fs manager, so to not create
     * contention on the file system.
     *
     * \param p - the path to the file to be read
     * \param buf - the buffer where the caller want the read data to be deposited
     * \param h - completion handler for the read operation (must be convertible to FilesystemManager::CompletionHandler)
     */
    void async_read(const Path& p, Buffer& buf, CompletionHandler h) override;

    /**
     * Register an asynch write request to the fs manager.
     *
     * The request will be enqueued on the task queue of the fs manager and the write operation will be performed
     * on the worker thread as soon as all previous pending tasks will have been completed.
     *
     * It's very important to note that the caller should never pass a filesystem I/O operation
     * inside the completion handler. In fact, the CompletionHandler run on the "main/application" thread,
     * while all I/O fs operation should be enqueued on the worker thread of the fs manager, so to not create
     * contention on the file system.
     *
     * \param p - the path to the file to be written
     * \param buf - the buffer containing the data to be written to fs
     * \param h - completion handler for the write operation (must be convertible to FilesystemManager::CompletionHandler)
     */
    void async_write(const Path& p, const Buffer& buf, CompletionHandler h) override;


    /**
     * Register an asynch append request to the fs manager.
     * \param p - the path to the file on which the append is performed
     * \param buf - the buffer to append to the file
     * \param h - the completion handler to be called on termination of the append operation.
     *
     */
    virtual void async_append(const Path &p, const Buffer &buf, FilesystemManagerInterface::CompletionHandler h) override;


    /** Reads a chunk asynchronously, using a chunked reader
     * \param r the chunkedreader to be used to perform the read
     * \param pos the position from which the read starts
     * \param buf the buffer to be used to save the data
     * \param h the completion handler to be called on read termination.
     */
    void async_read_chunk(std::shared_ptr<impl::ChunkedReader> r, size_t pos, impl::HotDoubleBuffer::BufferView& buf, CompletionHandler h)
    {
        // enque a read request to the waiting queue
        q_.push_chunked_read(r, pos, buf, h);
        available_.set_event();
    }

    /**
     * @brief make_chunked_stream
     * @param p
     * @param chunk_size
     * @return
     *
     * \throws filesystem:ErrorCode if the p is not the path to an existent regular file
     * \throws May throw std::bad_alloc or any other exception thrown by the constructor of T. If an exception is thrown, this function has no effect.
     * \throws std::invalid_argument in case the chunk size is 0.
     */
    std::shared_ptr<ChunkedFstreamInterface> make_chunked_stream(const Path& p, size_t chunk_size) override;


private:
    enum class OperationCode {
        async_read, async_write, async_read_chunk, async_append
    };

    /**
     * @brief perform_next_operation runs on the fs_manager thread.
     * It pops from the front of the asynchronous operations queue,
     * switches on the operation code, performs (synchronously) the
     * corresponding operation and schedules the completion handler
     * the be executed on the application's main thread.
     */
    void perform_next_operation();

    /**
     * @brief process_queue contains the loop to be executed by the
     * FilesystemManager thread. This function waits on the available_ event
     * of the FilesystemManager and invoke the execution of the next operation
     * when the event is signaled.
     * @param fs a reference to the FilesystemManager object who holds
     * the thread I'm running on
     */
    static void process_queue(FilesystemManager& fs);


    /**
     * @brief The OperationsQueue class is a thread-safe queue of asynchronous operations
     * that may be executed by a FilesystemManager. It stores the operation code,
     * the complation handler and any other data needed for that particular kind of
     * operation.
     */
    class OperationsQueue {
    public:
        // Having a mutex, for simplicity I make my queue not copyable
        OperationsQueue() = default;
        OperationsQueue(const OperationsQueue&) = default;
        OperationsQueue(OperationsQueue&&) = default;
        OperationsQueue& operator=(const OperationsQueue&) = delete;
        OperationsQueue& operator=(OperationsQueue&&) = default;

        // wrap all queue actions inside a locked scope
        bool empty() const { std::lock_guard<std::mutex> lck{mtx}; return q_operations.empty(); }
        size_t size() const { std::lock_guard<std::mutex> lck{mtx}; return q_operations.size(); }

        void push_read(const Path& path, Buffer& buf, CompletionHandler h);
        void push_write(const Path& path, const Buffer& buf, CompletionHandler h);
        void push_append(const Path& path, const Buffer& buf, CompletionHandler h);
        void push_chunked_read(std::shared_ptr<impl::ChunkedReader> r, size_t pos, impl::HotDoubleBuffer::BufferView& buf, CompletionHandler h);

        std::tuple<OperationCode,const Path,std::reference_wrapper<Buffer>,CompletionHandler> pop_read();
        std::tuple<OperationCode,const Path,std::reference_wrapper<const Buffer>,CompletionHandler> pop_write();
        std::tuple<OperationCode,std::shared_ptr<impl::ChunkedReader>,size_t,impl::HotDoubleBuffer::BufferView,CompletionHandler> pop_chunked_read();

        OperationCode front() const { std::lock_guard<std::mutex> lck{mtx}; return q_operations.front().first; }

    private:
        std::queue<std::pair<OperationCode,CompletionHandler>> q_operations;
        std::queue<std::tuple<const Path,std::reference_wrapper<const Buffer>>> q_write_data;
        std::queue<std::tuple<const Path,std::reference_wrapper<Buffer>>> q_read_data;
        std::queue<std::tuple<std::shared_ptr<impl::ChunkedReader>,size_t,impl::HotDoubleBuffer::BufferView>> q_as_read_data;
        mutable std::mutex mtx;
    };

    boost::asio::io_service& io_;
    OperationsQueue q_;
    Event available_;
    std::atomic_bool done_;
    std::thread thrd_;
};


/**
 * @brief The ChunkedFstream class
 * Represent an interface to the filesystem to read asynchronously a file chunk by chunk.
 * The stream gets closed when the object is destroyed.
 *
 * It can be created only through FilesystemManager::make_chunked_stream().
 */
class ChunkedFstream : public ChunkedFstreamInterface {
    friend std::shared_ptr<ChunkedFstreamInterface> FilesystemManager::make_chunked_stream(const Path& p, size_t chunk_size);
public:
    ChunkedFstream(const ChunkedFstream &) = delete;
    ChunkedFstream(ChunkedFstream&&) = default;
    ChunkedFstream& operator=(const ChunkedFstream&) = delete;
    ChunkedFstream& operator=(ChunkedFstream&&) = default;
    ~ChunkedFstream() override;

    /**
     * @brief Request the read of the next chunk.
     * @param h The handler to be called after the read has completed.
     */
    void next_chunk(FilesystemManager::ReadChunkHandler h) override;

protected:
    ChunkedFstream(std::shared_ptr<impl::ChunkedReader> r) : reader{r} {}
private:
    std::shared_ptr<impl::ChunkedReader> reader;
};



/* ------------------------------- free helper functions -----------------------------------
 * For every non static member function of the FilesystemManager we have a matching
 * free function that performs the operation on the filesystem.
 */

// operational static functions

/**
 * Checks whether a file or a directory exists.
 *
 * \param p path to a file or directory
 *
 * \returns true if the file or directory exists, false otherwise
 *
 * \throws If some filesystem error occurs, or if p is a symlink or not a directory or a regular file, a FilesystemError is thrown
 */
bool exists(const Path& p);

/**
 * Remove a file from the filesystem.
 *
 * If file p does not exists, the function does NOT throw and returns false
 *
 *  \param p path to the file
 *
 * \throws If some filesystem error occurs, or if p is a symlink or not a regular file, a FilesystemError is thrown
 */
bool removeFile(const Path& p);

/**
 * Move a file or a directory to a different path.
 *
 * \param from path to the file or directory to be moved
 * \param to path to the file or directory to move to
 *
 * \throws If some filesystem error occurs, or if form or to is a symlink or not a directory or a regular file, a FilesystemError is thrown
 */
void move(const Path& from, const Path& to);

/**
 * Copy a file to another path on the filesystem.
 *
 * If to is a (existing) directory, from is copied inside to.
 *
 * \param from - path to the file to be moved
 * \param to - path to move the file to
 *
 * \throws If some filesystem error occurs, or if from or to is a symlink or not a regular file, a FilesystemError is thrown
 */
void copyFile(const Path& from, const Path& to);

/**
 * Recursively copy a directory to another path on the filesystem.
 *
 * The recursive copy ignores everything that is not a regular file or a directory
 *
 * \param from - path to the directory to be copied
 * \param to - path to copy the directory to
 *
 * \throws If some filesystem error occurs, or if from or to is a symlink or not a directory, a FilesystemError is thrown
 */
void copyDirectory(const Path& from, const Path& to);

/**
 * Remove a directory from the filesystem.
 *
 *  \param p path to the directory
 *
 * If directory p does not exists, the function does NOT throw and returns 0
 *
 *  \returns The number of files removed
 *
 * \throws If some filesystem error occurs, or if p is a symlink or not a regular directory, a FilesystemError is thrown
 */
uintmax_t removeDirectory(const Path& p);

/**
 * @brief createDirectory
 * Create the DIRECTORY(ies), if they do not already exist.
 *
 * If parents is true, no exception is thrown when the directory already exists,
 * or when the parent directories need to be created and parent directories are
 * made as needed.
 *
 * @param p - path of the directory to create
 * @param parents - no error if existing, make parent directories as needed
 *
 * @return true if a directory was created, false otherwise
 *
 * @throws If p already exists and it's not a directory, an ErrorCode is thrown.
 * When parents is false, if p already exists or p is inside a not existing directory,
 * an ErrorCode is thrown.
 */
bool createDirectory(const Path& p, bool parents = false);

// file access

/**
 * Read a whole file from the filesystem and return its content as a vector of chars.
 *
 * \param p - path to the file to be read
 *
 * \returns a vector of chars containing the file content
 *
 * \throws If some filesystem error occurs, or if p is a symlink or not a  a regular file, a FilesystemError is thrown
 * \throws If there is an error in allocating memory for the returned vector, an exception may be thrown
 */
Buffer readFile(const Path& p);

/**
 * Write a buffer to file
 *
 * \param p - path to the file to write
 * \param buf - buffer of chars to be written
 *
 * \throws If some filesystem error occurs, or if p is not a regular file, a FilesystemError is thrown
 */
void writeFile(const Path& p, const Buffer& buf);

/**
 * Append a vector of chars to a file.
 *
 * \param p - path to the file to append to
 * \param bytes - a vector of chars append to p
 *
 * \throws If some filesystem error occurs, or if p is a symlink or not a directory or a regular file, a FilesystemError is thrown
 */
void appendToFile(const Path& p, const Buffer& bytes);


}   // namespace filesystem

} // namespace cynnypp
} // namespace cynny

#endif // CYNNYPP_FS_MANAGER_H_H
