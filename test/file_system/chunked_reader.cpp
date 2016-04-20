#include <iostream>
#include <boost/filesystem/operations.hpp>
#include <io/async/fs/fs_manager.h>
#include "catch.hpp"


using namespace cynny::cynnypp::filesystem;
const std::string input_dir = "../test/file_system/data";
boost::asio::io_service::work *w;
boost::asio::io_service io_;
boost::asio::deadline_timer *t = nullptr;
FilesystemManager fs(io_);

inline void createKeepAlive() {
    w = new boost::asio::io_service::work(io_);
    t = new boost::asio::deadline_timer(io_, boost::posix_time::milliseconds(10000)); t->async_wait([]( const boost::system::error_code& error){ if(!error) delete w;});
};

inline void deleteKeepAlive() {
    delete w;
    delete t;
    t = nullptr;
}

SCENARIO("Opening a file with size less than a chunk", "[fs][fs_chunked]"){
    io_.reset();
    auto chunkedReader = fs.make_chunked_stream(input_dir+"/read/multiplea", 1024);
    createKeepAlive();
    chunkedReader->next_chunk([](ErrorCode ec, Buffer b){
        REQUIRE(ec == ErrorCode::end_of_file);
        std::string s1 = "aaaaaaaaaaaaaaaaaaaa\n";
        Buffer r1;
        r1.insert(r1.begin(), s1.begin(), s1.end());
        REQUIRE(b == r1);
        deleteKeepAlive();
    });
    io_.run();
}

SCENARIO("Opening a file with chunk size equal to 0", "[fs][fs_chunked]"){
    REQUIRE_THROWS(fs.make_chunked_stream(input_dir+"/read/multiplea", 0));
    auto &io = io_;
    io.reset();
    io.run();
}

SCENARIO("Opening a file with size multiple of a page", "[fs][fs_chunked]") {
    auto chunkedReader = fs.make_chunked_stream(input_dir+"/read/chunkedmultiple.txt", 4096);
    auto &io = io_;
    createKeepAlive();
    io.reset();
    unsigned int count = 0;
    std::function<void(ErrorCode, Buffer b)> readCallback;
    Buffer ciao;
    readCallback = [&count, chunkedReader, &readCallback, &ciao](ErrorCode ec, Buffer b) {
        count++;
        bool allA = true;

        ciao.insert(ciao.end(), b.begin(), b.end());
        //REQUIRE(b.size() == 4096);
        for (auto i = b.begin(); i != b.end(); ++i) {
            if (*i != 'a') {
                allA = false;
                break;
            }
        }
        REQUIRE(allA == true);
        if(!ec) {
            chunkedReader->next_chunk(readCallback);
        } else {
            deleteKeepAlive();
        }

    };
    chunkedReader->next_chunk(readCallback);

    io.run();

    REQUIRE(ciao.size() == 8192);
}

SCENARIO("Opening a file with size not-multiple of a page but still larger", "[fs][fs_chunked]"){
    auto chunkedReader = fs.make_chunked_stream(input_dir+"/read/chunkedmultiple.txt", 17);
    auto &io = io_;
    //io.reset();
    createKeepAlive();
    io.reset();
    unsigned int count = 0;
    std::function<void(ErrorCode, Buffer b)> readCallback;
    Buffer ciao;
    readCallback = [&count, chunkedReader, &readCallback, &ciao](ErrorCode ec, Buffer b) {
        count++;
        bool allA = true;
        ciao.insert(ciao.end(), b.begin(), b.end());
        REQUIRE(b.size() <= 17);
        for (auto i = b.begin(); i != b.end(); ++i) {
            if (*i != 'a') {
                allA = false;
                break;
            }
        }
        REQUIRE(allA == true);
        if(!ec) {
            chunkedReader->next_chunk(readCallback);
        } else {
            deleteKeepAlive();
        }
    };
    chunkedReader->next_chunk(readCallback);

    io.run();
    REQUIRE(ciao.size() == 8192);
}

SCENARIO("Opening an empty file", "[fs][fs_chunked]") {
    auto chunkedReader = fs.make_chunked_stream(input_dir+"/read/empty", 4096);
    auto &io =io_;
    //io.reset();
    createKeepAlive();
    io.reset();
    unsigned int count = 0;
    std::function<void(ErrorCode, Buffer b)> readCallback;
    readCallback = [&count, chunkedReader, &readCallback](ErrorCode ec, Buffer b) {
        REQUIRE(ec);
        REQUIRE(b.size() == 0);
    };
    chunkedReader->next_chunk(readCallback);
    io.run();
}

SCENARIO("Opening a file that does not exist", "[fs][fs_chunked]"){
    REQUIRE_THROWS(fs.make_chunked_stream(input_dir+"/read/chunkedmughdfauihltiple.txt", 4878));
    auto &io = io_;
    io.reset();
    io.run();
}


SCENARIO("Never calling the nextChunk function", "[fs][fs_chunked]"){
    auto chunkedReader = fs.make_chunked_stream(input_dir+"/read/chunkedmultiple.txt", 8778);
    auto &io = io_;
    io.reset();
    io.run(); //should just exit withour issues.
}
