#include "swapping_buffer_overwrite.h"
#include "chunked_readers.h"
#include "boost/asio.hpp"

namespace cynny {
namespace cynnypp {
namespace swapping {


SwappingBufferOverwrite::SwappingBufferOverwrite(boost::asio::io_service& io, filesystem::FilesystemManagerInterface& fs, const std::string& root_dir)
    : SwappingBuffer(io, fs, root_dir)
{} //constructor

void SwappingBufferOverwrite::saveAllContents(const std::string &destinationPath, std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) {
    //default management is the one of overwrite.
    auto self = this->shared_from_this();
    enqueueAndRun([self, destinationPath, successCallback, errorCallback](){
        if(!self->isOnDisk) return self->saveLocalContents(destinationPath, successCallback, errorCallback);
        //just append to temporary file and then
            self->fs.async_append(self->tmp_path, *self->currentBuffer, [self, destinationPath, successCallback, errorCallback](const filesystem::ErrorCode& error, size_t length){
                if(error) {
                    errorCallback({filesystem::ErrorCode::append_failure, std::string("Could not append to temporary swapping file.")+error.what()});
                    return;
                }
                try {
                    //when this returns it means that the remaining part has been written. also the version has been overwritten.
                    self->fs.move(self->tmp_path, destinationPath); //move directly to destination!
                } catch (const filesystem::ErrorCode &ec) {
                    if((ec == filesystem::ErrorCode::open_failure || ec == filesystem::ErrorCode::invalid_argument) && self->isFirstSaveAttempt) {
                        self->isFirstSaveAttempt = false;
                        size_t fileBeginning = destinationPath.find_last_of('/');
                        auto dir = destinationPath.substr(0, fileBeginning);
                        self->fs.createDirectory(dir, true);
                        return;
                    }
                    errorCallback({filesystem::ErrorCode::append_failure, std::string("Error during commit: could not write down the requested copy of the resource.")+ec.what()});
                    return;
                }
                successCallback();
            });
            return;
        //in case it is not on disk, we save a move and directly write down to the desired location.
    });
}


void SwappingBufferOverwrite::readAll(std::function<void(const Buffer &b)> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) {
    auto self = this->shared_from_this();
    enqueueAndRun([self, successCallback, errorCallback]() {
        if(!self->isOnDisk) {
            return successCallback(*self->currentBuffer);
        }
        self->fs.async_read(self->tmp_path, self->tmp_read, [self, successCallback, errorCallback](const filesystem::ErrorCode& ec, size_t data){
            if(ec) return errorCallback({filesystem::ErrorCode::read_failure, std::string("Could not read from disk, because of ") + ec.what()});
            //erase version... sadly we need to place it in the beginning
            self->tmp_read.erase(self->tmp_read.begin(), self->tmp_read.begin());
            self->tmp_read.insert(self->tmp_read.end(), self->currentBuffer->begin(), self->currentBuffer->end());
            //append current file content
            successCallback(self->tmp_read);
        });
    });
}


void SwappingBufferOverwrite::startSwapping(std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) {
    SwappingBuffer::startSwapping(successCallback, errorCallback);
}


void SwappingBufferOverwrite::swappingOperation(std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) {
    if(!isOnDisk) { //first call, allocatee first 8 bytes to save version
        fs.async_write(tmp_path, *swappingBuffer, [this, successCallback, errorCallback](const filesystem::ErrorCode& ec, size_t length){
            this->postSwapRoutine(ec, length, successCallback, errorCallback);
        });
        return;
    }
    fs.async_append(tmp_path, *swappingBuffer, [this, successCallback, errorCallback](const filesystem::ErrorCode& ec, size_t length){
        this->postSwapRoutine(ec, length, successCallback, errorCallback);
    });
    return;
}


void SwappingBufferOverwrite::saveLocalContents(const std::string &destinationPath,
                                                   std::function<void()> successCallback,
                                                   std::function<void(const filesystem::ErrorCode&)> errorCallback) {
    auto self = this->shared_from_this();
    self->fs.async_write(destinationPath, *currentBuffer, [self, destinationPath, successCallback, errorCallback](const filesystem::ErrorCode& ec, size_t length) {

        if(!ec) return successCallback(); //done.
        if((ec == filesystem::ErrorCode::open_failure || ec == filesystem::ErrorCode::invalid_argument) && self->isFirstSaveAttempt) {
            self->isFirstSaveAttempt = false;
            size_t fileBeginning = destinationPath.find_last_of('/');
            auto dir = destinationPath.substr(0, fileBeginning);
            self->fs.createDirectory(dir, true);
            self->saveLocalContents(destinationPath, successCallback, errorCallback);
            return;
        }
        errorCallback({filesystem::ErrorCode::write_failure, "Error during commit: could not write down the requested copy of the resource"});

    });
}

void SwappingBufferOverwrite::make_chunked_stream(std::shared_ptr<sharedinfo> info, size_t chunk_size,
                                                     std::function<void(std::shared_ptr<filesystem::ChunkedFstreamInterface>)> successCallback,
                                                     std::function<void(const filesystem::ErrorCode&)> errorCallback) {
    auto self = this->shared_from_this();
    enqueueAndRun([self, info, chunk_size, successCallback, errorCallback](){
        if(!self->isOnDisk) return self->io.post(std::bind(successCallback, std::shared_ptr<filesystem::ChunkedFstreamInterface>(new CacheChunkedReader(self->io, self->fs, info, chunk_size))));
        return self->io.post(std::bind(successCallback, std::shared_ptr<filesystem::ChunkedFstreamInterface>( new SwappingBufferOverwriteChunkedReader(self->io, self->fs, info, self->tmp_path, chunk_size))));
    });

}


}
}
}
