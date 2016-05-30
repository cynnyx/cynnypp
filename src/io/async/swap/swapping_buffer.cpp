#include "swapping_buffer.h"
#include "boost/asio.hpp"

#ifndef TRANSACTION_BUFFER_SWAP
#define TRANSACTION_BUFFER_SWAP "/tmp"
#endif

namespace cynny {
namespace cynnypp {
namespace swapping {
uint64_t SwappingBuffer::currentTransactionId = 0;


std::string calculateTmpPath(const std::string& root_dir, uint64_t id) {
    std::string t{root_dir};
    t.append(TRANSACTION_BUFFER_SWAP);
    t.append(std::to_string(id));
    return t;
}


SwappingBuffer::SwappingBuffer(boost::asio::io_service& io, filesystem::FilesystemManagerInterface& fs, const std::string& root_dir)
    : io(io)
    , fs(fs)
    , root_dir(root_dir)
    , tmp_path(calculateTmpPath(root_dir, currentTransactionId))
{
    //use consecutive filenames to represent sessions. we are sure there will be no collisions
    //initialize buffers
    currentTransactionId++;
    currentBuffer = &bufA;
    swappingBuffer = &bufB;
    //get actual size from filesystem?
    realSize = 0;

} //for now it does nothing; later it will initilize the vectors properly!


void SwappingBuffer::size(std::function<void(uint32_t)> successCallback) noexcept {
    enqueueAndRun([this, successCallback]() {
        successCallback(realSize);
    });
}

void SwappingBuffer::clear(std::function<void()> successCallback) noexcept { //this one i know what to do!
     enqueueAndRun([this, successCallback](){
         currentBuffer->clear(); //why? they should both be valid
         swappingBuffer->clear();
         realSize = 0;
         if(isOnDisk) {
            fs.removeFile(tmp_path);
            isOnDisk = false;
         }
         io.post(successCallback);
    });

}

void SwappingBuffer::append(const std::vector<Byte> &chunk, std::function<void(uint32_t)> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback){

    if(chunk.size() + currentBuffer->size() < maxBufferSize || chunk.size() > maxBufferSize) {

        //just insert.
        currentBuffer->insert(currentBuffer->end(), chunk.begin(), chunk.end());
        realSize += chunk.size();
        if(chunk.size() > maxBufferSize) return startSwapping(std::bind(successCallback, realSize), errorCallback);
        else return successCallback(realSize);

    }
    if(!swapping) {
        //directly insert.

        realSize += chunk.size();
        startSwapping(std::bind(successCallback, realSize), errorCallback); //write down to disk.
        currentBuffer->insert(currentBuffer->end(), chunk.begin(), chunk.end());
    } else {
        //if we're swapping, we append and then schedule another swap. Notice that it could be that the size will be greather
        //than the maximum size allowed for currentBuffer. However, otherwise we would need to perform a copy of the buffer:
        //hence we would occupy approximately the same space.
        currentBuffer->insert(currentBuffer->end(), chunk.begin(), chunk.end());
        realSize += chunk.size();
        enqueueAndRun([this, successCallback, errorCallback](){ //another swap!
           startSwapping(std::bind(successCallback, realSize), errorCallback);
        });
    }

}

void SwappingBuffer::startSwapping(std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback){
    //common swapping part :D
    auto tmp = currentBuffer;
    currentBuffer = swappingBuffer;
    swappingBuffer = tmp;
    currentBuffer->clear();
    //initialize collections.
    swapping = true;
    swappingOperation(successCallback, errorCallback);
}

void SwappingBuffer::performPendingOperations() {
    boost::asio::io_service::strand s(io); //the strand is used to be sure across implementations that the functions will be executed in order.
    auto iterator = callbacks.begin();
    while(iterator != callbacks.end()) {
        io.post(s.wrap(std::move(*iterator)));
        ++iterator;
    }
    callbacks.clear();
}

void SwappingBuffer::enqueueAndRun(std::function<void()> afterSwapCompletedCallback)  {
    callbacks.push_back(std::move(afterSwapCompletedCallback));
    if(!swapping) performPendingOperations();
}

/*
void SwappingBuffer::
*/
//management of the routine to be performed after the append operation, in practice.
void SwappingBuffer::postSwapRoutine(const filesystem::ErrorCode &ec, const size_t length, std::function<void()> successCallback, std::function<void(const filesystem::ErrorCode&)> errorCallback) {
    swapping = false;
    if(isFirstSwappingAttempt && (ec == filesystem::ErrorCode::invalid_argument || ec == filesystem::ErrorCode::open_failure)) {
        //maybe the directory is not available?
        isFirstSwappingAttempt = false;
        size_t fileBeginning = tmp_path.find_last_of('/');
        auto dir = tmp_path.substr(0, fileBeginning);
        try { //in case the problem is that the directory does not exist
            fs.createDirectory(dir, true);
            swapping = true;
            swappingOperation(successCallback, errorCallback); //swap again, with fingers crossed :D
            return;
        } catch (const filesystem::ErrorCode &createDirEc) {
//            ServiceLocator::setStatus(Status{0, 0, 1, 0});
            error = true;
            errorCallback({filesystem::ErrorCode::write_failure, std::string("Uknown error while swapping buffer to disk.")+ec.what()});
            return;
        }
    } else if (!ec) {
        isOnDisk = true;
        successCallback();
        performPendingOperations();
        return;
    }
    error = true;
    errorCallback({filesystem::ErrorCode::write_failure, std::string("Unknown filesystem error while swapping buffer to disk.") + ec.what()});
    return;

}

}
}
}
