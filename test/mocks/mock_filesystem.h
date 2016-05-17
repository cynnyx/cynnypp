#ifndef ATLAS_MOCKFILESYSTEM_H
#define ATLAS_MOCKFILESYSTEM_H

#include "io/async/fs/fs_manager_interface.h"
#include "boost/functional/hash/hash.hpp"
#include <unordered_map>
#include <iostream>
#include "boost/asio.hpp"



using namespace cynny::cynnypp::filesystem;

class TimerManager {
public:
    using TimerId = uintptr_t;
    /**
     * @brief Builds a TimerManager.
     * \param io reference to the io_service effectively running the timers
     */
    TimerManager(boost::asio::io_service& io) noexcept : io(io) {}


    TimerManager(const TimerManager&) = delete;

    TimerManager(TimerManager&&) = default;

    /**
     * @brief Schedules a callback after a certain interval.
     * \param cb the callback to be executed when the timer expires (move)
     * \param ms the number of ms to wait before the timer expires
     * \return an identifier of the timer
     */
    TimerId scheduleCallback(std::function<void()> && cb, std::chrono::milliseconds ms) {
        auto timer = new boost::asio::deadline_timer{io, boost::posix_time::milliseconds(ms.count())};
        timer->async_wait([cb = std::move(cb), timer](const boost::system::error_code &deleted){if(!deleted){cb();} delete timer; });
        return reinterpret_cast<uintptr_t>(timer);
    }

    /**
     * @brief Deschedules the execution of the timer identified by the parameter.
     * \param id the identifier of the timer
     */
    void deleteTimer(TimerId id)
    {
        auto it = reinterpret_cast<boost::asio::deadline_timer *>(id);
        it->cancel();
    }

private:
    std::list<boost::asio::deadline_timer> timerList;
    boost::asio::io_service &io;
};

class MockChunkedInterface : public ChunkedFstreamInterface {
public:

    MockChunkedInterface(boost::asio::io_service& io, const Path &p, Buffer b, size_t chunk_size) : io{io}, path(p), file(b), chunk(chunk_size) {
    }

    ~MockChunkedInterface() override = default;
    void next_chunk(FilesystemManagerInterface::ReadChunkHandler h) override;

private:
    boost::asio::io_service& io;
    Path path;
    Buffer file;
    size_t chunk;
    size_t currentOffset = 0;
};


class MockFilesystem : public  FilesystemManagerInterface{
public:
    MockFilesystem(boost::asio::io_service& io)
        : io{io}
        , timerManager{io}
    {}

    bool exists(const Path& p)override;

    bool removeFile(const Path& p) override;

    void move(const Path& from, const Path& to) override;

    void copyFile(const Path& from, const Path& to) override;

    void copyDirectory(const Path& from, const Path& to) override;

    uintmax_t removeDirectory(const Path& p) override;

    bool createDirectory(const Path& p, bool parents) override;

    Buffer readFile(const Path& p) override;

    void writeFile(const Path& p, const Buffer& buf) override;

    void appendToFile(const Path& p, const Buffer& bytes) override;

    void async_read(const Path& p, Buffer& buf, CompletionHandler h) override;

    void async_write(const Path& p, const Buffer& buf, CompletionHandler h) override;

    void async_append(const Path&p, const Buffer &buf, CompletionHandler h) override;

    std::shared_ptr<ChunkedFstreamInterface> make_chunked_stream(const Path& p, size_t chunk_size) override;

    ~MockFilesystem() = default; //todo: fix resources

    void clear();


private:
    template<typename T>
    struct arrayHash {
        size_t operator()(const T key) const {
            return boost::hash_range(key.begin(), key.end());
        }
    };

    boost::asio::io_service& io;
    std::unordered_map<Path, Buffer, arrayHash<Path>> fs;
    TimerManager timerManager;
};


#endif //ATLAS_MOCKFILESYSTEM_H
