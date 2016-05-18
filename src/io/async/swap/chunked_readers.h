#ifndef ATLAS_CACHECHUNKEDREADER_H
#define ATLAS_CACHECHUNKEDREADER_H

#include <iostream>
#include "../fs/fs_manager_interface.h"
#include "swapping_buffer.h"

namespace boost {
namespace asio {

class io_service;

}
}

namespace cynny {
namespace cynnypp {
/** @brief contains the auxiliary data structures used by the transaction manager or its nested components.
 *
 */
namespace swapping {

/** CacheChunkedReader is a chunked reader which returns chunk by chunk the data contained in a portion of the memory.
 * It is used by the transaction buffer in order to return the content to the user when it is not big enough to be
 * swapped and is hence cached in memory.
 */
class CacheChunkedReader : public filesystem::ChunkedFstreamInterface {
public:
    /** Creates a CacheChunkedReader
     * \param i - the sharedinfo object, used to communicate with the owner of the data (the transaction object)
     * \param chunk_size the size of the chunks to be returned to the user
     *
     */
    CacheChunkedReader(boost::asio::io_service& io, filesystem::FilesystemManagerInterface& fs, std::shared_ptr<sharedinfo> i, size_t chunk_size=1024)
        : io{io}
        , pos(0)
        , info(i)
        , chunk_size(chunk_size){}

    /** Triggers the read of the following chunk (if available) of the data
     * \param h the chunkhandler to be called once the read operation is complete.
     */
    void next_chunk(filesystem::FilesystemManagerInterface::ReadChunkHandler h);

    /** Destroys the object
     */
    ~CacheChunkedReader() = default;

private:
    boost::asio::io_service& io;
    size_t pos;
    std::shared_ptr<sharedinfo> info;
    const int chunk_size;
};

/** TransactionBufferOverwriteChunkedReader is a composite reader of chunks which is used by the transaction buffer of
 * type overwrite to operate on data which might be partially swapped to disk and partially contained in memory.
 * It is composed either by a cachedreader (in case the data size is below the swapping threshold) or by a
 * filesystem chunked reader followed by a cached reader.
 */
class SwappingBufferOverwriteChunkedReader : public filesystem::ChunkedFstreamInterface {
public:

    /** Creates a transaction buffer overwrite
     * \param i - the sharedinfo object used to comunicate with the owner of the data (transaction object)
     * \param tmp_path - the path eventually used to swap the content of the transaction to disk.
     * \param chunk_size the size of the chunks to return to the user.
     */
    SwappingBufferOverwriteChunkedReader(boost::asio::io_service& io,
                                            filesystem::FilesystemManagerInterface& fs,
                                            std::shared_ptr<sharedinfo> i,
                                            const filesystem::Path &tmp_path, size_t chunk_size = 1024);

    /** Triggers the read of the next chunk.
     */
    void next_chunk(filesystem::FilesystemManagerInterface::ReadChunkHandler h);

private:
    boost::asio::io_service& io;
    filesystem::FilesystemManagerInterface& fs;
    std::shared_ptr<sharedinfo> info;
    filesystem::Path path;
    const int chunk_size;
    std::shared_ptr<filesystem::ChunkedFstreamInterface> tmp_file, cached_file;
    bool tmp_file_finished = false;

};

/** TransactionBufferAppendChunkedReader is a composite reader of chunks which is used by the transaction buffer of type
 * append to operate on data which is partially contained on disk and partially in memory.
 *
 * It is composed by three readers: a file reader, a tmp_file reader and a cached reader. In case the
 * size of the data appended is below the threshold for swapping, the tmp_file reader is not used and does not
 * exist.
 */
class SwappingBufferAppendChunkedReader : public filesystem::ChunkedFstreamInterface {
public:
    /** Creates a transaction buffer append
     * \param i the sharedinfo object used to comunicate with the owner of the data (transaction object)
     * \param path the path on which we are performing the append operation
     * \param tmp_path the path used to swap the content before commit
     * \param chunk_size the size of the chunks to return to the user
     *
     */
    SwappingBufferAppendChunkedReader(boost::asio::io_service& io,
                                         filesystem::FilesystemManagerInterface& fs,
                                         std::shared_ptr<sharedinfo> i,
                                         const filesystem::Path &path ,
                                         const filesystem::Path &tmp_path, size_t chunk_size = 1024);

    /** Triggers the read of the nextchunk
     * \param h the chunkhandler to be called once the read operation is complete.
     */
    void next_chunk(filesystem::FilesystemManagerInterface::ReadChunkHandler h);

private:
    boost::asio::io_service& io;
    filesystem::FilesystemManagerInterface& fs;
    std::shared_ptr<sharedinfo> info;
    filesystem::Path path, tmp_path;
    const int chunk_size;
    std::shared_ptr<filesystem::ChunkedFstreamInterface> file, tmp_file, cached_file;
    bool file_finished = false;
    bool tmp_file_finished = false;


};


}
}
}

#endif //ATLAS_CACHECHUNKEDREADER_H
