#include "chunked_readers.h"
#include "boost/asio.hpp"


namespace cynny { namespace cynnypp { namespace swapping {

using Buffer = SwappingBuffer::Buffer;

void CacheChunkedReader::next_chunk(filesystem::FilesystemManagerInterface::ReadChunkHandler h) {
    auto &buf = info->data->currentBuffer; //operates on the current buffer; the underlying assumption is that the currentBuffer never changes while we're reading it.
    auto beg = buf->begin() + pos;
    auto end = (std::distance(beg, buf->end()) > chunk_size) ? beg + chunk_size : buf->end();

    if(beg == buf->end() || pos > buf->size()) {
        io.post(std::bind(h, filesystem::ErrorCode::end_of_file, Buffer{}));
        return;
    }

    pos += chunk_size;

    filesystem::ErrorCode ec = end == buf->end() ? filesystem::ErrorCode::end_of_file : filesystem::ErrorCode::success;
    //stopReading is set when a sudden unlock arrives.
    if(info->stopReading == true) {
        ec = filesystem::ErrorCode::stopped;
        io.post(std::bind(h, ec, Buffer{}));
        return;
    }
    //creates a copy of the data
    io.post(std::bind(h, ec, Buffer{beg, end}));
}


SwappingBufferOverwriteChunkedReader::SwappingBufferOverwriteChunkedReader(boost::asio::io_service& io,
        filesystem::FilesystemManagerInterface& fs, std::shared_ptr<sharedinfo> i, const filesystem::Path &tmp_path, size_t chunk_size)
    : io(io)
    , fs(fs)
    , info(i)
    , path(tmp_path)
    , chunk_size(chunk_size)
{
    tmp_file = fs.make_chunked_stream(tmp_path, chunk_size);
}

void SwappingBufferOverwriteChunkedReader::next_chunk(filesystem::FilesystemManagerInterface::ReadChunkHandler h) {
    if(!tmp_file_finished) { //in this case we're swapping! hence the first thing we do is performing a next chunk on it.
        tmp_file->next_chunk([this, h](const filesystem::ErrorCode& ec, Buffer data){
            if(ec == filesystem::ErrorCode::end_of_file || ec == filesystem::ErrorCode::stopped) { //it has finished!
                tmp_file_finished = true;
                //prepare next file.
                cached_file = std::shared_ptr<filesystem::ChunkedFstreamInterface>(new CacheChunkedReader(io, fs, info, chunk_size));
                if(data.size() > 0) { //something has been loaded. Return it to the user.
                    io.post(std::bind(h, filesystem::ErrorCode::success, data));
                } else {
                    io.post([this, h](){cached_file->next_chunk(h);});
                }
                tmp_file = nullptr;
                return;
            }
            h(ec, data);
        });
    } else {
        //read directly from memory.
        io.post([this, h](){cached_file->next_chunk(h);});
    }
}


SwappingBufferAppendChunkedReader::SwappingBufferAppendChunkedReader(boost::asio::io_service& io, filesystem::FilesystemManagerInterface& fs,
                                                                           std::shared_ptr<sharedinfo> i,
                                                                           const filesystem::Path &path,
                                                                           const filesystem::Path &tmp_path,
                                                                           size_t chunk_size)
    : io(io)
    , fs(fs)
    , info(i)
    , path(path)
    , tmp_path(tmp_path)
    , chunk_size(chunk_size)

{
    file = fs.make_chunked_stream(path, chunk_size); //start reading from the original file.

}


void SwappingBufferAppendChunkedReader::next_chunk(filesystem::FilesystemManagerInterface::ReadChunkHandler h) {
    if(!file_finished) { //read from original file.
        file->next_chunk([this, h](const filesystem::ErrorCode& ec, Buffer data){
            if(ec == filesystem::ErrorCode::end_of_file || ec == filesystem::ErrorCode::stopped) {
                file_finished = true;
                //the file has finished: start reading from the swapping one.
                tmp_file = fs.make_chunked_stream(tmp_path, chunk_size); //raw because we're reading from a swapping append buffer, hence there's no version on it.
                if(data.size() > 0) {
                    h(filesystem::ErrorCode::success, data);
                } else {
                    io.post([this, h](){tmp_file->next_chunk(h);});
                }
                file = nullptr;
                return;
            }
            h(ec, data);
        });
    } else {
        //same as in the transaction overwrite buffer chunked reader. 
        if(!tmp_file_finished) {
            tmp_file->next_chunk([this, h](const filesystem::ErrorCode& ec, Buffer data){
                if(ec == filesystem::ErrorCode::end_of_file || ec == filesystem::ErrorCode::stopped) { //it has finished!
                    tmp_file_finished = true;
                    //prepare next file.
                    cached_file = std::shared_ptr<filesystem::ChunkedFstreamInterface>(new CacheChunkedReader(io, fs, info, chunk_size));
                    if(data.size() > 0) {
                        h(filesystem::ErrorCode::success, data);
                        tmp_file = nullptr;
                        return;
                    } else {
                        io.post([this, h](){cached_file->next_chunk(h);});
                        tmp_file = nullptr;
                        return;
                    }
                }
                h(ec, data);
            });
        } else io.post([this, h](){cached_file->next_chunk(h);});
    }
}


}}}
