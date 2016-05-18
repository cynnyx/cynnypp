#include "swapping_buffer_append.h"
#include "chunked_readers.h"
#include "boost/asio.hpp"


#ifndef DISK_MOVE_SIZE
#define DISK_MOVE_SIZE 4096*64
#endif

namespace cynny { namespace cynnypp { namespace swapping {

SwappingBufferAppend::SwappingBufferAppend(boost::asio::io_service& io,
                                           filesystem::FilesystemManagerInterface& fs,
                                           const std::string& root_dir,
                                           const std::string &beginningFilePath)
    : SwappingBuffer{io, fs, root_dir}
    , path(beginningFilePath) {
}

void SwappingBufferAppend::saveLocalContents(const std::string &destinationPath, std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) {
    auto self = this->shared_from_this();
    //REMEMBER: This works because the underlying assumption when this kind ofo buffer is used is that we will always have a previously existing file!
    fs.async_append(destinationPath, *currentBuffer, [successCallback, errorCallback](const filesystem::ErrorCode& ec, size_t length){
        if(ec) return errorCallback({filesystem::ErrorCode::append_failure, std::string("Error while saving transaction to disk: ") + ec.what() });
        successCallback();
    });
}


void SwappingBufferAppend::saveLambda(std::shared_ptr<SwappingBufferAppend> self, std::string destinationPath,
                                         std::shared_ptr<filesystem::ChunkedFstreamInterface> tmp_file_reader,
                                         std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback,
                                         const filesystem::ErrorCode &ec, Buffer b) {
    if(!ec) { //readed successfully
        self->tmp_read = b; //we spare an allocation, at least.
        self->fs.async_append(destinationPath, self->tmp_read, [self, destinationPath, successCallback, errorCallback, tmp_file_reader](const filesystem::ErrorCode& ec, size_t length){
            if(!ec) {
                tmp_file_reader->next_chunk(std::bind(saveLambda, self, destinationPath, tmp_file_reader, successCallback, errorCallback, std::placeholders::_1, std::placeholders::_2));
            } else { //in this case the error is when the append is being performed; hence the only thing we can do is returning an error to the user.
                errorCallback({filesystem::ErrorCode::append_failure, std::string("Could not perform an append on the desired resource; Error is: ") + ec.what() });
            }
        });
        return;
    }
    if(ec == filesystem::ErrorCode::end_of_file || ec == filesystem::ErrorCode::stopped) {
        self->tmp_read = b;
        self->fs.async_append(destinationPath, self->tmp_read, [self, destinationPath, successCallback, errorCallback, tmp_file_reader](const filesystem::ErrorCode& ec, size_t length){
            if(!ec) {
                self->fs.async_append(destinationPath, (*self->currentBuffer), [self,destinationPath, successCallback, errorCallback](const filesystem::ErrorCode& ec, size_t length){
                    if(ec != filesystem::ErrorCode::success) errorCallback({filesystem::ErrorCode::append_failure, "Could not perform an append on the desired resource" });
                    //once also this has been done, call successcallback
                    self->currentBuffer->clear();
                    self->currentBuffer->shrink_to_fit(); //free memory from temporary data.
                    successCallback();
                });
            } else { //in this case the error is when the append is being performed; hence the only thing we can do is returning an error to the user.
                errorCallback({filesystem::ErrorCode::append_failure, std::string("Could not perform an append on the desired resource; Error is: ") + ec.what() });
            }
        });
        //append local content.
        return;
    }
    return errorCallback({filesystem::ErrorCode::append_failure, std::string("Could not perform an append on the desired resource; Error is: ") + ec.what() });
}


void SwappingBufferAppend::saveAllContents(const std::string &destinationPath, std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) {
    auto self = this->shared_from_this();
    enqueueAndRun([self, destinationPath, successCallback, errorCallback]() {
        if(self->error) return errorCallback({filesystem::ErrorCode::write_failure, "Error in the filesystem"});
        if(!self->isOnDisk) return self->saveLocalContents(destinationPath, successCallback, errorCallback);

        auto tmp_file_reader = self->fs.make_chunked_stream(self->tmp_path, DISK_MOVE_SIZE);
        self->tmp_read.reserve(DISK_MOVE_SIZE);
        self->tmp_read.shrink_to_fit();
        tmp_file_reader->next_chunk(std::bind(saveLambda, self, destinationPath, tmp_file_reader, successCallback, errorCallback, std::placeholders::_1, std::placeholders::_2));
    });
}


void SwappingBufferAppend::swappingOperation(std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) {
    auto self = this->shared_from_this();
    self->fs.async_append(tmp_path, *swappingBuffer, [self, successCallback, errorCallback](auto ec, auto length){
        self->postSwapRoutine(ec, length, successCallback, errorCallback);
    });

}

void SwappingBufferAppend::readAll(std::function<void(const Buffer &b)> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) {
    auto self = this->shared_from_this();
    enqueueAndRun([self, successCallback, errorCallback](){

        self->fs.async_read(self->path, self->tmp_read, [self, successCallback, errorCallback](auto er, auto length){
            if(er) return errorCallback({filesystem::ErrorCode::read_failure, std::string("Could not read file")+ er.what() });

            self->tmp_read.erase(self->tmp_read.begin(), self->tmp_read.begin());

            if(!self->isOnDisk) {
                self->tmp_read.insert(self->tmp_read.end(), self->currentBuffer->begin(), self->currentBuffer->end());
                successCallback(self->tmp_read);
                return;
            }

            Buffer *b = new Buffer(); //todo: fix
            self->fs.async_read(self->tmp_path, *b, [self, successCallback, errorCallback, b](const filesystem::ErrorCode& , size_t ){
                self->tmp_read.insert(self->tmp_read.end(), b->begin(), b->end());
                self->tmp_read.insert(self->tmp_read.end(), self->currentBuffer->begin(), self->currentBuffer->end());
                delete b;
                successCallback(self->tmp_read);
            });
        });
    });
}

void SwappingBufferAppend::make_chunked_stream(std::shared_ptr<sharedinfo> info,
                                               size_t chunk_size,
                                               std::function<void(std::shared_ptr<filesystem::ChunkedFstreamInterface>)> successCallback,
                                               std::function<void(const filesystem::ErrorCode&)> errorCallback) {
    auto self = this->shared_from_this();
    enqueueAndRun([self, successCallback, errorCallback, info, chunk_size](){
        try {
            if(!self->isOnDisk) {
                auto a = std::shared_ptr<filesystem::ChunkedFstreamInterface>( new SwappingBufferOverwriteChunkedReader(self->io, self->fs, info, self->path, chunk_size));
                self->io.post(std::bind(successCallback, a));
                return;
            }
            auto a = std::shared_ptr<filesystem::ChunkedFstreamInterface>( new SwappingBufferAppendChunkedReader(self->io, self->fs, info, self->path, self->tmp_path, chunk_size));
            self->io.post(std::bind(successCallback, a));
        } catch(const filesystem::ErrorCode &ec) {
            errorCallback({filesystem::ErrorCode::open_failure, std::string("Cannot create chunked stream because of file system error: ")+ec.what()});
        }
    });

}

}}}
