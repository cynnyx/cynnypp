#include "mock_filesystem.h"
#include "io/async/fs/fs_manager.h"
#include "boost/asio.hpp"
#include <chrono>



// ------------------------

constexpr int default_fs_simulated_timeout = 90;

bool MockFilesystem::exists(const Path &p) {
    if(fs.find(p) != fs.end()) return true;
    /** Search for directories */
    for(auto element = fs.begin(); element != fs.end(); ++element) {
        if (element->first.find(p) == 0) {
            return true;
        }
    }
    return false;
}

bool MockFilesystem::removeFile(const Path &p) {
    auto res = fs.erase(p);
    return res > 0;
}

uintmax_t MockFilesystem::removeDirectory(const Path &p) {
    auto res = fs.erase(p);
    if(res) return res;
    /** Iterate over all path elements and verify which of them have as prefix the path passed as parameter; then, remove. */
    auto finalIterator = fs.end();
    uintmax_t els = 0;
    for(auto element = fs.begin(); element != finalIterator; ) {
        if (element->first.find(p) == 0) { //must be a prefix
            element = fs.erase(element); els++;
        } else {
            ++element;
        }
    }
    return els;
}

bool MockFilesystem::createDirectory(const Path &p, bool parents)
{
    // TODO
    assert("To be implemented");
    return false;
}

void MockFilesystem::move(const Path& from, const Path& to) {
    std::list<std::pair<const Path &, const Buffer&>> temporaryValues;
    for(auto element = fs.begin(); element != fs.end(); ) {
        if (element->first.find(from) == 0) {
            temporaryValues.push_back({element->first, element->second});
        }
        ++element;
    }
    //Once removed:
    for(auto tmp = temporaryValues.begin(); tmp != temporaryValues.end(); ++tmp) {
        auto newVal = tmp->first;
        newVal.replace(0, newVal.size(), to);
        auto res = fs.find(newVal);
        if(res != fs.end()) { //it is already there, hence overwrite
            res->second = tmp->second;
        } else {
            fs.insert({newVal, tmp->second});
        }
        fs.erase(tmp->first);
    }
}


void MockFilesystem::copyFile(const Path& from, const Path& to) {
    // TODO
    assert("to be implemented!!!!");
}

void MockFilesystem::copyDirectory(const Path& from, const Path& to) {
    // TODO
    assert("to be implemented!!!!");
}



Buffer MockFilesystem::readFile(const Path& p) {
    auto res = fs.find(p);
    if(res != fs.end()) {
        return res->second;
    }

    throw(ErrorCode(ErrorCode::invalid_argument, std::string{__func__} + " was not able to open the file " + p + " in read mode"));
}


void MockFilesystem::writeFile(const Path& p, const Buffer& buf) { //never throws
    auto res = fs.find(p);
    if(res != fs.end()) {
        res->second = buf;
    } else {
        fs.insert({p, buf});
    }
}

void MockFilesystem::appendToFile(const Path& p, const Buffer& bytes) {
    auto res = fs.find(p);
    if(res != fs.end()) {
        res->second.insert(res->second.end(), bytes.begin(), bytes.end());
        return;
    }
    writeFile(p, bytes);
}

void MockFilesystem::async_read(const Path& path, Buffer& buf, FilesystemManagerInterface::CompletionHandler h) {
    timerManager.scheduleCallback([h(std::move(h)), &buf, path, this](){
        try {
            buf = readFile(path);
            h(ErrorCode::success, buf.size());
        } catch(const ErrorCode& e) {
            h(e, 0);
        }
    }, std::chrono::milliseconds(default_fs_simulated_timeout));
}

void MockFilesystem::async_write(const Path& p, const Buffer& buf, FilesystemManagerInterface::CompletionHandler h) {
    timerManager.scheduleCallback([h(std::move(h)), buf, p, this](){
        writeFile(p, buf);
        h(ErrorCode::success, buf.size());
    }, std::chrono::milliseconds(default_fs_simulated_timeout));
}

void MockFilesystem::async_append(const Path &p, const Buffer &buf, FilesystemManagerInterface::CompletionHandler h) {
    timerManager.scheduleCallback([h(std::move(h)), buf, p, this](){
        appendToFile(p, buf);
        h(ErrorCode::success, buf.size());
    }, std::chrono::milliseconds(default_fs_simulated_timeout));
}

std::shared_ptr<ChunkedFstreamInterface> MockFilesystem::make_chunked_stream(const Path &p, size_t chunk_size)
{
    auto buf = readFile(p); //read all data; return it to an object which is dumb and implements chunkedfstreaminterface
    return std::shared_ptr<ChunkedFstreamInterface>(new MockChunkedInterface(io, p, buf, chunk_size));

}

void MockFilesystem::clear()
{
    fs.clear();
}



void MockChunkedInterface::next_chunk(FilesystemManagerInterface::ReadChunkHandler h) {
    auto nextValue = file.begin() + currentOffset; //current position get
    if (currentOffset > file.size() || nextValue == file.end()) {
        h(ErrorCode::end_of_file, Buffer{});
        return;
    }
    Buffer::iterator end = nextValue + chunk;
    auto ec = ErrorCode::success;
    if(currentOffset + chunk > file.size()) {
        ec = ErrorCode::end_of_file;
        end = file.end();
    }
    Buffer b(nextValue, end);
    currentOffset+=chunk;
    io.post(std::bind(h, ec, b));

}
