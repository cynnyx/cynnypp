#ifndef ATLAS_TRANSACTION_BUFFER_H
#define ATLAS_TRANSACTION_BUFFER_H


#ifndef MAX_OCCUPIED_MEMORY
#define MAX_OCCUPIED_MEMORY 2*1024*1024
#endif


#include "../fs/fs_manager_interface.h"
#include <list>
#include <vector>
#include <cstdint>

namespace boost {
namespace asio {

class io_service;

}
}


namespace cynny {
namespace cynnypp {
namespace swapping {

struct sharedinfo;
class CacheChunkedReader;

/** A transaction buffer is an abstract class. Transaction Buffer(s) are one for each of the transactions and can be of type Overwrite (if the previous content is discarded)
 * or Append (if the previous content is preserved and the new information is appended to the file).
 *
 * Transaction buffers allow to operate in ram as a normal vector until a threshold (MAX_OCCUPIED_MEMORY) is exceeded by the data.
 * After this point the data is swapped to disk on a temporary file, and can then be saved to its final destination or discarded.
 *
 * However, from the point of view of the transaction the behaviour is similar to the one we would expect to have from an asynchronous vector.
 */
class SwappingBuffer {
    friend class CacheChunkedReader;
public:
    using Byte = uint8_t;
    using Buffer = std::vector<Byte>;

    /** Clears all the content, both on disk and on memory.
     * \param successCallback the function to be called once the clear has been performed
     */
    void clear(std::function<void()> successCallback) noexcept;
    /** Retrieves (asynchronously) the size of the content.
     * \param successCallback the function to be called once the size can be retrieved reliably. The passed parameter is the actual size
     */
    void size(std::function<void(uint32_t)> successCallback) noexcept;
    /** Appends a chunk of data to the transaction buffer.
     * \param chunk - the data to apppend to the transaction
     * \param successCallback - the function to be called if the append is successful; instantiated with the actual size of the data.
     * \param errorCallback - the function to be called in case of error
     */
    void append(const std::vector<Byte> &chunk, std::function<void(uint32_t)> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback);
    /** readAll retrieves all logical content of the transaction buffer
     * \param successCallback the function to be called once all the content of the file has been readed; a reference to the vector is returned as parameter
     * \param errorCallback the function to be called in case there's an error.
     */
    virtual void readAll(std::function<void(const Buffer &b)> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback)=0;
    /** Saves all the logical contents of the transaction buffer in a specific destination, with a certain version.
     * \param destinationPath - the path on which the contents have to be saved
     * \param successCallback - the callback to be executed in case the write is executd successfully
     * \param errorCallback - the callback to be executed in case the write cannot be performed.
     */
    virtual void saveAllContents(const std::string &destinationPath, std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback)=0;
    /** Creates a chunkedReader to access the data logically contained in the transaction buffer
     * \param info the sharedinformation object used to communicate with the owner of the data (the transaction object)
     * \param chunk_size the desired chunksize
     * \param successCallback the function called when the chunked reader can be created successfully; it takes as parameter a shared_ptr to the ChunkedFstream
     * \param errorCallback the function called when the chunked reader cannot be created.
     */
    virtual void make_chunked_stream(std::shared_ptr<sharedinfo> info, size_t chunk_size, std::function<void(std::shared_ptr<filesystem::ChunkedFstreamInterface>)> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback)=0; //every different kind of buffer will implement its own.
    /** The maxBufferSize before swapping
     */
    static constexpr size_t maxBufferSize = MAX_OCCUPIED_MEMORY;

    virtual ~SwappingBuffer() = default;

protected:
    SwappingBuffer(boost::asio::io_service& io, filesystem::FilesystemManagerInterface& fs, const std::string& root_dir);

    // reference to a global io_service
    boost::asio::io_service& io;
    // reference to a global filesystem manager
    filesystem::FilesystemManagerInterface& fs;
    Buffer tmp_read;
    const std::string root_dir;
    const std::string tmp_path;
    size_t realSize;

    bool isOnDisk = false;
    bool swapping = false;
    bool error = false;
    bool isFirstSaveAttempt = true;

    //buffer used to write down data to disk
    Buffer *swappingBuffer;
    //buffer used to insert data
    Buffer *currentBuffer;

    std::list<std::function<void()>> callbacks;


    /** enqueues an operation and runs it if the buffer is not swapping in the moment in which the function is called.
     *
     */
    void enqueueAndRun(std::function<void()> afterSwappingCallback) ;
    /** Performs all enqueued operations
     *
     */
    void performPendingOperations();
    /** saves all the local contents to disk. This function is called in case we want to save the transaction buffer logical
     * data when its size does not exceed the maxBufferSize threshold.
     * \param destinationPath where to save the data
     * \param successCallback executed when the saving has been done with success
     * \param errorCallback executed when it was not possible to save the data
     */
    virtual void saveLocalContents(const std::string &destinationPath, std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback)=0;
    /** Sets the preconditions for a swapping operation
     * \param successCallback - function to execute when the swapping is completed successfully
     * \param errorCallback - function to execute when there's an error while swapping
     */
    virtual void startSwapping(std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback);
    /** Routine to execute when the swapping is completed, either successfully or with failure
     * \param ec the error code returned by the filesystem
     * \param length the amount of data written to disk
     * \param successCallback the callback executed once the routine has completed successfully
     * \param errorCallback the clalback executed if the routine could not complete successfully
     *
     */
    virtual void postSwapRoutine(const filesystem::ErrorCode &ec, const size_t length, std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback);

private:
    /** Operation effectively invokng the swapping operation
     *
     */
    virtual void swappingOperation(std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback)=0;

    bool isFirstSwappingAttempt = true;

    //first buffer in use
    Buffer bufA;
    //second buffer in use; since in the beginning it will be empty we will have next to no overhead for allocating it
    //at construction, even if we don't use it.
    Buffer bufB;

    static uint64_t currentTransactionId;

};


struct sharedinfo {
    sharedinfo(SwappingBuffer *data) : data(data){}
    std::shared_ptr<SwappingBuffer> data;
    bool stopReading = false;
};

}
}
}

#endif //ATLAS_TRANSACTION_BUFFER_H
