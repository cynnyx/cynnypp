#ifndef FILESYSTEMMANAGERINTERFACE_H_H
#define FILESYSTEMMANAGERINTERFACE_H_H

#include <cstdint>
#include <string>
#include <functional>
#include <type_traits>
#include "../utilities/event.h"

namespace cynny {

/**
 * @brief Contains all those components used by \ref commands to actually perform operations.
 *
 * These components handles transactions, filesystem, upload/download and timers.
 */
namespace utilities {

/**
 * @brief Contains (predictably) everything is related to read/write stuff from/to disk
 */
namespace filesystem {


// Path could be replaced with boost::filesystem::path or something similar if needed
using Path = std::string;
using Buffer = std::vector<uint8_t>;
constexpr int pageSize = 4096;

/**
 * @brief The ErrorCode class wraps together a boost::system style error code + an (optional) exception message
 */
class ErrorCode {
public:
    enum Error {
        success = 0,
        internal_failure = 1, // failure of the underlying libraries
        invalid_argument = 2,
        operation_not_permitted = 3,
        open_failure = 4,
        read_failure = 5,
        write_failure = 6,
        append_failure = 7,
        end_of_file = 8,
        unknown_error = 9,
        stopped = 10
    };

    ErrorCode(Error ec = success, const std::string &err_msg = {})
            : val{ec}, msg{err_msg} { }
    ErrorCode(const ErrorCode&) = default;
    ErrorCode(ErrorCode&&) = default;
    ErrorCode& operator=(const ErrorCode&) = default;
    ErrorCode& operator=(ErrorCode&&) = default;

    // conversion from/to Error
    ErrorCode& operator=(Error ec) { val = ec; msg = ""; return *this; }
    operator Error() const { return val; }

    // accessors
    std::string what() const;

private:
    Error val;
    std::string msg;
    static const std::string tag;
};

// fwd declaration
struct ChunkedFstreamInterface;

/**
 * @brief The FilesystemManagerInterface class is a pure virtual class that defines
 * an interface to the filesystem. It contains synchronous and asynchronous operations.
 */
class FilesystemManagerInterface {
public:
    using CompletionHandler = std::function<void(const ErrorCode &ec, size_t bytesRead)>;
    using ReadChunkHandler = std::function<void(const ErrorCode& e, Buffer)>;

    virtual ~FilesystemManagerInterface() = 0;

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
    virtual bool exists(const Path& p)=0;

    /**
     * Remove a file from the filesystem.
     *
     * If file p does not exists, the function does NOT throw and returns false
     *
     *  \param p path to the file
     *
     * \throws If some filesystem error occurs, or if p is a symlink or not a regular file, a FilesystemError is thrown
     */
    virtual bool removeFile(const Path& p)=0;

    /**
     * Move a file or a directory to a different path.
     *
     * \param from path to the file or directory to be moved
     * \param to path to the file or directory to move to
     *
     * \throws If some filesystem error occurs, or if form or to is a symlink or not a directory or a regular file, a FilesystemError is thrown
     */
    virtual void move(const Path& from, const Path& to)=0;

    /**
     * Copy a file to another path on the filesystem.
     *
     * \param from - path to the file to be moved
     * \param to - path to move the file to
     *
     * \throws If some filesystem error occurs, or if from or to is a symlink or not a regular file, a FilesystemError is thrown
     */
    virtual void copyFile(const Path& from, const Path& to)=0;

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
    virtual void copyDirectory(const Path& from, const Path& to)=0;

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
    virtual uintmax_t removeDirectory(const Path& p)=0;

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
    virtual bool createDirectory(const Path& p, bool parents = false) = 0;


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
    virtual Buffer readFile(const Path& p)=0;


    /**
     * Write a buffer to file
     *
     * \param p - path to the file to write
     * \param buf - buffer of chars to be written
     *
     * \throws If some filesystem error occurs, or if p is not a regular file, a FilesystemError is thrown
     */
    virtual void writeFile(const Path& p, const Buffer& buf)=0;


    /**
     * Append a vector of chars to a file.
     *
     * \param p - path to the file to append to
     * \param bytes - a vector of chars append to p
     *
     * \throws If some filesystem error occurs, or if p is a symlink or not a directory or a regular file, a FilesystemError is thrown
     */
    virtual void appendToFile(const Path& p, const Buffer& bytes)=0;




    //--------------------------------------------- asynchronous interface

    /**
     * Regaister an asynch read request to the fs manager.
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

    virtual void async_read(const Path &p, Buffer &buf, CompletionHandler h)=0;

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
    virtual void async_write(const Path &p, const Buffer &buf, CompletionHandler h)=0;


    /**
    * Register an asynch append request to the fs manager, overload.
    *
    * \param p - the path to the file to be written
    * \param buf - the buffer containing the data to be appended
    * \param h - completion handler for the append operation (must be convertible to FilesystemManager::CompletionHandler)
    */
    virtual void async_append(const Path&p, const Buffer &buf, FilesystemManagerInterface::CompletionHandler h)=0;



    /**
     * @brief make_chunked_stream
     * @param p
     * @param chunk_size
     * @return
     *
     * @throws If the file not exists, an ErrorCode::open_failure is thrown
     */
    virtual std::shared_ptr<ChunkedFstreamInterface> make_chunked_stream(const Path& p, size_t chunk_size = pageSize) = 0;
};

inline FilesystemManagerInterface::~FilesystemManagerInterface() {}


/**
 * @brief The ChunkedFstreamInterface struct is a pure virtual class that defines the
 * interface for a ChunkedFstream, an object used to read from the filesystem chunk by chunk.
 */
struct ChunkedFstreamInterface {

    virtual ~ChunkedFstreamInterface() = 0;

    /**
     * @brief next_chunk
     * Request the read of the next chunk.
     * @param h The handler to be called after the read has completed. NOTICE that the handler can be called with success with an amount of bytes which is less than the required chunk_size.
     */
    virtual void next_chunk(FilesystemManagerInterface::ReadChunkHandler h) = 0;
};

inline ChunkedFstreamInterface::~ChunkedFstreamInterface() {}

}
}
}
#endif //ATLAS_FILESYSTEMMANAGERINTERFACE_H_H
