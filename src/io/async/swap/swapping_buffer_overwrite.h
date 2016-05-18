#ifndef ATLAS_TRANSACTION_BUFFER_OVERWRITE_H
#define ATLAS_TRANSACTION_BUFFER_OVERWRITE_H


#include <string>
#include <memory>
#include "swapping_buffer.h"

namespace cynny {
namespace cynnypp {
namespace swapping {
/** TransactionBufferOverwrite is a transaction buffer used when the previous content of the referred resource has to
 * be discarded. It swaps to temporary file once a certain threshold is reached.
 *
 * Please see \ref TransactionBuffer to understand its usage.
 */
class SwappingBufferOverwrite : public SwappingBuffer , public std::enable_shared_from_this<SwappingBufferOverwrite> {
public:
    void saveAllContents(const std::string &destinationPath, std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) override;
    void readAll(std::function<void(const Buffer &b)> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) override;
    void make_chunked_stream(std::shared_ptr<sharedinfo> info, size_t chunk_size, std::function<void(std::shared_ptr<filesystem::ChunkedFstreamInterface>)> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) override;
    inline static std::shared_ptr<SwappingBufferOverwrite> make_shared(boost::asio::io_service& io, filesystem::FilesystemManagerInterface& fs, const std::string& root_dir)
    {
        return std::shared_ptr<SwappingBufferOverwrite>(new SwappingBufferOverwrite(io, fs, root_dir));
    }
     ~SwappingBufferOverwrite() = default;
protected:
    SwappingBufferOverwrite(boost::asio::io_service& io, filesystem::FilesystemManagerInterface& fs, const std::string& root_dir);
    void startSwapping(std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) override;
    void saveLocalContents(const std::string &destinationPath, std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) override;
private:
    void swappingOperation(std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) override;
};

}
}
}



#endif //ATLAS_TRANSACTION_BUFFER_OVERWRITE_H
