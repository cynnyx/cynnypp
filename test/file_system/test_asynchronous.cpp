#include <iostream>
#include <boost/filesystem/operations.hpp>
#include <boost/asio.hpp>
#include "io/async/fs/fs_manager.h"
#include "catch.hpp"

using Buffer = std::vector<uint8_t>;
using namespace cynny::cynnypp::filesystem;

const std::string input_dir = "../test/file_system/data";
const std::string working_dir = "./data";

SCENARIO("Async Read", "[fs_async_read][fs_async][fs]")  {
    boost::asio::io_service io;
    std::string s1 = "aaaaaaaaaaaaaaaaaaaa\n";
    Buffer r1;
    r1.insert(r1.begin(), s1.begin(), s1.end());
    FilesystemManager fs(io);
    Buffer output1, output2;

    GIVEN("A normal file") {
        WHEN("Reading from it") {
            fs.async_read(input_dir+"/read/multiplea", output1, [&r1, &output1](const ErrorCode &ec, size_t bytesRead){ //std::function<void(const ErrorCode& ec, size_t bytesRead)>;
                if(ec) {
                    THEN("No Error must occour") {
                        REQUIRE(false);
                    }
                } else {
                    THEN("The readed value must be correct in content and size") {
                        REQUIRE(output1 == r1);
                        REQUIRE(output1.size() == r1.size());
                    }
                }
            });
        }
    }

    GIVEN("A file containing some invisible chars") {
        Buffer r2;
        for(uint8_t i = 0; i < 39; i++) {r2.push_back(i);}
        WHEN("Reading from it") {
            fs.async_read(input_dir+"/read/multiplea", output2, [&r2, &output2](const ErrorCode &ec, size_t bytesRead){ //std::function<void(const ErrorCode& ec, size_t bytesRead)>;
                if(ec) {
                    THEN("No Error must occour") {
                        REQUIRE(false);
                    }
                } else {
                    THEN("The readed value must be correct in content and size") {
                        REQUIRE(output2 == r2);
                        REQUIRE(output2.size() == r2.size());
                    }
                }
            });
        }
    }

    GIVEN("A directory") {
        WHEN("Performing a read"){
            fs.async_read(input_dir+"/read/dir/", output1, [](const ErrorCode &ec, size_t bytesRead) {
                if(ec) {
                THEN("An error must be returned"){ REQUIRE(true); }
                } else {
                    REQUIRE(false);
                }
            });
        }
    }

    GIVEN("A symlink") {
        WHEN("Performing a read") {
            fs.async_read(input_dir+"/read/linktomultiplea", output1, [&r1,&output1](const ErrorCode &ec, size_t bytesRead){
                if(ec) {
                    THEN("NO ERROR MUST BE RETURNED") {
                        REQUIRE(false);
                    }
                } else {
                    THEN("The content returned must be correct") {
                        REQUIRE(r1 == output1);
                        REQUIRE(r1.size() == bytesRead);
                    }
                }
            });
        }
    }

    GIVEN("A special file") {
        WHEN("reading") {
            fs.async_read(input_dir+"/read/dir/specialfile", output1, [](const ErrorCode &ec, size_t bytesRead){
                if(ec) {
                    REQUIRE(true);
                } else {
                    REQUIRE(false);
                }
            });
        }
    }
    Buffer hugeFile;
    GIVEN("A large file") {
        WHEN("reading") {
            fs.async_read(input_dir+"/read/hugefile", hugeFile, [&hugeFile](const ErrorCode &ec, size_t bytesRead) {
                if(ec) REQUIRE(false);
                else {
                    REQUIRE(hugeFile.size() == boost::filesystem::file_size(input_dir+"/read/hugefile"));
                }
            });
        }
    }

    GIVEN("An invalid path") {
        WHEN("Reading") {
        fs.async_read("", hugeFile, [](const ErrorCode &ec, size_t bytesRead) {
            if(ec) REQUIRE(true);
            else REQUIRE(false);
        });
        }
    }


    GIVEN("Two concurrent reads on existing files") {
        WHEN("Spawning two requests") {
            fs.async_read(input_dir+"/read/hugefile", hugeFile, [](const ErrorCode &ec, size_t bytesRead){
                if(ec) REQUIRE(false);
            });
            fs.async_read(input_dir+"/read/multiplea", output1, [](const ErrorCode &ec, size_t bytesRead){
                if(ec) REQUIRE(false);
            });

            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            io.run();
            REQUIRE(output1 == r1);
            REQUIRE(hugeFile.size() == boost::filesystem::file_size(input_dir+"/read/hugefile"));
            io.reset();
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    io.run();
}


SCENARIO("Write asynchronously", "[fs_async_write][fs_async][fs]"){
    boost::asio::io_service io;
    std::string s1 = "aaaaaaaaaaaaaaaaaaaa\n";
    Buffer r1;
    r1.insert(r1.begin(), s1.begin(), s1.end());
    FilesystemManager fs(io);
    Buffer input1, input2;
    /* copy of normal file. */
    fs.removeDirectory(working_dir);
    try{
        //we use the data in read to work on it
        fs.copyDirectory(input_dir+"/read", working_dir);
    } catch (ErrorCode &exc) {

        std::cout << "\033[1;31m====================== ERROR WHILE COPYING =====================\033[0m" << std::endl;
        std::cout << "not properly an error. However you should check what happened:" << std::endl;
        std::cout << exc.what() << std::endl;
        std::cout << "\033[1;31m================================================================\033[0m " << std::endl;
        //REQUIRE(false);
    }
    std::string content = "ciapooinasoinasoinnoasnoasnoasoiasnoinasoinasoinasoiasnoiasnasnoinasoiasoinasonas";
    Buffer b;
    for(auto i = content.begin(); i != content.end(); i++) b.push_back(*i);

    GIVEN("A file that exists") {
        WHEN("we write into it") {
            fs.async_write(working_dir+"/multiplea", b, [](const ErrorCode &ec, size_t bytesRead) {
                if (ec) REQUIRE(false);
            });
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            io.run();
            THEN("The content of the file must be exactly the same of the written buffer") {
                REQUIRE(fs.readFile(working_dir+"/multiplea") == b);
            }
        }

        WHEN("We perform two concurrent write") {
            Buffer c;
            for(int i = 0; i < 10; i++) b.push_back((uint8_t) i);
            fs.async_write(working_dir+"/multiplea", b, [](const ErrorCode &ec, size_t bytesRead) {
                if(ec) REQUIRE(false);
            });
            fs.async_write(working_dir+"/multiplea", c, [](const ErrorCode &ec, size_t bytesRead) {
                if(ec) REQUIRE(false);
            });
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            io.run();
            THEN("The content of the file must correspond to one or the other (no interleaving)") {
                REQUIRE((fs.readFile(working_dir+"/multiplea") == c || fs.readFile(working_dir+"/multiplea") == b) == true);
            }
        }
    }

    GIVEN("A directory") {
        WHEN("We write into it") {
            fs.async_write(working_dir+"/dir", b, [](const ErrorCode &ec, size_t bytesRead) {
                THEN("There must be an error") {
                    if(!ec) REQUIRE(false);
                    else REQUIRE(true); //must return an error.
                }
            });
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            io.run();
            THEN("The directory must still exist") {
                REQUIRE(fs.exists(working_dir+"/dir"));
            } AND_THEN("It must be a directory") {
                REQUIRE_THROWS(fs.readFile(working_dir+"/dir"));
            }
        }
    }

    GIVEN("A special file") {
        mkfifo((working_dir+"/specialfile").c_str(), EACCES);
        WHEN("We perform a write"){
            fs.async_write(working_dir+"/specialfile", b, [](const ErrorCode &ec, size_t bytesRead) {
                if(!ec) REQUIRE(false);
                else REQUIRE(true);
            });
        }
    }

    GIVEN("A symlink") {
        int a = symlink((working_dir + "multiplea").c_str(), (working_dir+"/linktomultiplea").c_str());
        REQUIRE(a == 0);
        WHEN("We perform a write"){
            fs.async_write(working_dir+"/linktomultiplea", b, [](const ErrorCode &ec, size_t bytesRead) {
                if(!ec) REQUIRE(false);
                else REQUIRE(true);
            });
        }
    }

    GIVEN("A path for a new file") {
        WHEN("We perform a write") {
            fs.async_write(working_dir+"/newfile.txt", b, [](const ErrorCode &ec, size_t bytesRead){
                if(ec) REQUIRE(false);
                else REQUIRE(true);
            });
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            io.run();
            THEN("The file must exist") {
                REQUIRE(fs.exists(working_dir+"/newfile.txt"));
            } AND_THEN("Its content must correspond to the one used for creation") {
                REQUIRE(fs.readFile(working_dir+"/newfile.txt") == b);
            }
        }
    }

    GIVEN("A path for a new file comprising a directory that does not exist") {
        WHEN("We perform a write") {
            fs.async_write(working_dir+"/dashauishuiahdas/fshauishdas/newfile.txt", b, [](const ErrorCode &ec, size_t bytesRead) {
                if(ec) REQUIRE(true);
                else REQUIRE(false);
            });
        }
    }

    GIVEN("An empty buffer") {
        Buffer empty;
        WHEN("We perform a write on a existing file") {
            fs.async_write(working_dir+"/prova.txt", empty, [](const ErrorCode &ec, size_t bytesRead) {
                if(ec) REQUIRE(false);
                else REQUIRE(true);
            });
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            io.run();
            THEN("The file must exist") {
                REQUIRE(fs.exists(working_dir+"/prova.txt"));
            } AND_THEN("It must be empty") {
                REQUIRE(fs.readFile(working_dir+"/prova.txt").size() == 0);
            }
        }

        WHEN("We perform a write on a new file") {
            fs.async_write(working_dir+"/newfile2.txt", empty, [](const ErrorCode &ec, size_t bytesRead) {
                if(ec) REQUIRE(false);
                else
                    REQUIRE(true);
            });
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
           io.run();
            THEN("It must exist") {
                REQUIRE(fs.exists(working_dir+"/newfile2.txt"));
            } AND_THEN("It must be empty") {
                REQUIRE(fs.readFile(working_dir+"/newfile2.txt").size() == 0);
            }
        }
    }

    GIVEN("A buffer to be written to disk") {
        Buffer buf(1000);
        WHEN("I write two times the buffer to the same path") {
            auto path = working_dir + "/two_times.txt";
            THEN("the size of the written file is equal to the size of one single buffer") {
                fs.async_write(path, buf, [](auto ec, auto len){});
                fs.async_write(path, buf, [path, buf_size = buf.size()](auto ec, auto len){
                    auto size = boost::filesystem::file_size(path);
                    REQUIRE((size == buf_size));
                });
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        io.run();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    io.run();

    fs.removeDirectory(working_dir);

}

