#ifndef ATLAS_TRANSACTION_BUFFER_APPEND_H
#define ATLAS_TRANSACTION_BUFFER_APPEND_H

#include <string>
#include "swapping_buffer.h"

namespace cynny {
namespace cynnypp {
namespace swapping {

class SwappingBufferAppend : public SwappingBuffer, public std::enable_shared_from_this<SwappingBufferAppend> {
public:
    /** Saves all contents to disk
     * \param destinationPath the path on which the data has to be appended
     * \param v the version of the current contents
     * \param successCallback callback used to signal the user a successful append
     * \param errorCallback the callback used to signal the user a failure in append
     */
    void saveAllContents(const std::string &destinationPath, std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) override;
    /** reads all the logical content and returns it in a logical buffer. Notice that the buffer will last as long as the transaction will, so take care.
     * USED ONLY IN TESTS! See if it is the case to remove it
     * \param successCallback successCallback to be called upon successful read; parameter instantiated with a reference to the content
     * \param errorCallback called when an error occurs
     */
    void readAll(std::function<void(const Buffer &b)> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) override;
    /** Creates a chunked stream to access the data
     * \param info a shared pointer used to share information with the logical owner
     * \param chunk_size the size of the chunks to be loaded
     * \param successCallback the callback to be called in case of success
     * \param errorCallback to be called when a failure occurs
     */
    void make_chunked_stream(std::shared_ptr<sharedinfo> info, size_t chunk_size, std::function<void(std::shared_ptr<filesystem::ChunkedFstreamInterface>)> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) override; //every different kind of buffer will implement its own.

    /** Used to create a shared_ptr instance of the object
     * \param path the path on which the append will be performed
     */
    static std::shared_ptr<SwappingBufferAppend> make_shared(boost::asio::io_service& io, filesystem::FilesystemManagerInterface& fs, const std::string& root_dir, const std::string &path)
    {
        return std::shared_ptr<SwappingBufferAppend>(new SwappingBufferAppend(io, fs, root_dir, path));
    }

    ~SwappingBufferAppend() = default;
protected:
    //void startSwapping() override
    SwappingBufferAppend(boost::asio::io_service& io, filesystem::FilesystemManagerInterface& fs, const std::string& root_dir, const std::string &beginningFilePath);
    void saveLocalContents(const std::string& destinationPath, std::function<void ()> successCallback, std::function<void (const filesystem::ErrorCode&)> errorCallback) override;
private:
    void swappingOperation(std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) override;
    const std::string path;
    static void saveLambda(std::shared_ptr<SwappingBufferAppend> self, std::string destinationPath, std::shared_ptr<filesystem::ChunkedFstreamInterface> tmp_file_reader, std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback, const filesystem::ErrorCode &ec, Buffer b);

};

}
}
}


#endif //ATLAS_TRANSACTION_BUFFER_APPEND_H
