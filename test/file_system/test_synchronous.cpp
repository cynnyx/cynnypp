#include <iostream>
#include <boost/filesystem/operations.hpp>
#include <file_system/fs_manager_interface.h>
#include <boost/asio/io_service.hpp>
#include <file_system/fs_manager.h>
#include "catch.hpp"

using Buffer = std::vector<uint8_t>;

const std::string input_dir = "../test/file_system/data";
const std::string working_dir = "./data";


/* Vedere se aggregare in un unico test case con più section dentro!*/
TEST_CASE("Exist on different files", "[fs_exists][fs]") {

    boost::asio::io_service io;
    cynny::cynnypp::filesystem::FilesystemManager fsm(io);
    REQUIRE(fsm.exists(input_dir+"/exists") == true); //true
    REQUIRE(fsm.exists(input_dir+"/esixts") == false);//false
    REQUIRE(fsm.exists(input_dir+"/exists/not_exist") == false); //false
    REQUIRE(fsm.exists(input_dir+"/exists/exists") == true); //true
    REQUIRE(fsm.exists(input_dir+"/exists/normalfile") == true);
    REQUIRE_THROWS(fsm.exists(input_dir+"/exists/linkfile"));
    REQUIRE_THROWS(fsm.exists(input_dir+"/exists/specialfile"));
    REQUIRE(fsm.exists(input_dir+"/exists/*/sub") == false);
    REQUIRE(fsm.exists(input_dir+"/exists/file_with_no_read_permissions") == false);
    REQUIRE(fsm.exists(input_dir+"/exists/*ciao") == false);
    io.run();
}
//
//TEST_CASE("Remove on different files", "[fs_remove][fs]") {
//    using namespace atlas;
//    /* Simple file remove */
//    try{
//        fsm.copyDirectory(input_dir, working_dir);
//        std::cout << "copied some stuff into" << working_dir << std::endl;
//    } catch (components::filesystem::ErrorCode &exc) {
//
//        std::cout << "\033[1;31m====================== ERROR WHILE COPYING =====================\033[0m" << std::endl;
//        std::cout << "not properly an error. However you should check what happened:" << std::endl;
//        std::cout << exc.what() << std::endl;
//        std::cout << "\033[1;31m================================================================\033[0m " << std::endl;
//        //REQUIRE(false);
//    }
//
//        REQUIRE(fsm.exists(working_dir+"/remove/remove1") == true);
//        fsm.removeFile(working_dir+"/remove/remove1"); //apply side effect
//        REQUIRE(fsm.exists(working_dir+"/remove/remove1") == false);
//
//
//    fsm.removeDirectory(working_dir+"/remove/remove2"); //remove2 is an empty directory
//    REQUIRE(fsm.exists(working_dir+"/remove/remove2") == false);
//
//    REQUIRE_THROWS(fsm.removeFile(working_dir+"/remove/remove3")); // throws because remove3 is a directory
//    fsm.removeDirectory(working_dir+"/remove/remove3"); //remove3 is a directory containing some data
//    REQUIRE(fsm.exists(working_dir+"/remove/remove3") == false);
//
//    REQUIRE(fsm.exists(working_dir+"/remove/remove4") == false); //sanity check: remove4 doesn't exist.
//    REQUIRE(fsm.removeFile(working_dir+"/remove/remove4") == false); // remove4 does not exist, removeFile does nothing
//    REQUIRE(fsm.removeDirectory(working_dir+"/remove/remove4") == 0); // remove4 does not exist, removeDirectory does nothing
//
//    // TODO: i test sui permessi è bene rifarli e verificare se i files mantengono i loro permessi attraverso git
////    fsm.removeFile(working_dir+"/remove/remove5/file"); //file onto which we only have read access.
////    REQUIRE(fsm.exists(working_dir+"/remove/remove5/file") == true);
//
//    fsm.removeDirectory(working_dir);
//}
//
//
//
//TEST_CASE("Reading different files", "[fs_read][fs]")  {
//    using namespace atlas;
//    std::string s1 = "aaaaaaaaaaaaaaaaaaaa\n";
//    Buffer r1;
//    for(auto i = s1.begin(); i != s1.end(); i++) r1.push_back(*i);
//    REQUIRE(fsm.readFile(input_dir+"/read/multiplea") == r1); //normal chars
//    Buffer r2;
//    for(::atlas::Byte i = 0; i < 39; i++) {r2.push_back(i);}
//    REQUIRE(fsm.readFile(input_dir+"/read/invisiblechars") == r2);
//    REQUIRE_THROWS(fsm.readFile(input_dir+"/read/dir/"));
//    REQUIRE_THROWS(fsm.readFile(input_dir+"/read/linktomultiplea"));
//    REQUIRE_THROWS(fsm.readFile(input_dir+"/read/dir/specialfile"));
//    REQUIRE(fsm.readFile(input_dir+"/read/hugefile").size() == boost::filesystem::file_size(input_dir+"/read/hugefile"));
//    REQUIRE_THROWS(fsm.readFile(""));
////    REQUIRE_THROWS(fsm.readFile('\0')); // TODO: what this test was intended to do? it does not compile with clang
//}
//
//
//SCENARIO("File Move", "[fs_move][fs]") {
//    using namespace atlas;
//    fsm.removeDirectory(working_dir);
//    try{
//        fsm.copyDirectory(input_dir+"/move", working_dir);
//    } catch (components::filesystem::ErrorCode &exc) {
//
//        std::cout << "\033[1;31m====================== ERROR WHILE COPYING =====================\033[0m" << std::endl;
//        std::cout << "not properly an error. However you should check what happened:" << std::endl;
//        std::cout << exc.what() << std::endl;
//        std::cout << "\033[1;31m================================================================\033[0m " << std::endl;
//        //REQUIRE(false);
//    }
//    GIVEN("A FILE mv1 which exists") {
//        Buffer mv1 = fsm.readFile(working_dir+"/mv1");
//        WHEN("the file is moved into itself") {
//        /*Move file into itself*/
//
//            try {
//                fsm.move(working_dir + "/mv1",
//                     working_dir + "/mv1"); //file should remain in the same position and no modifications should happen
//            } catch(components::filesystem::ErrorCode &exc) {
//                REQUIRE(false);
//            }
//            THEN("its content must remain the same"){REQUIRE(fsm.readFile(working_dir+"/mv1") == mv1);}
//        }
//
//        WHEN("the file is moved to a different location") {
//            /*Move file into a different location*/
//            try {
//                fsm.move(working_dir + "/mv1", working_dir + "/mv2");
//            } catch (components::filesystem::ErrorCode &exc) {
//                FAIL("Move operation failed, with motivation: "+exc.what());
//            }
//            THEN("the origin file mustn't exist") {
//                REQUIRE(fsm.exists(working_dir + "/mv1") == false);
//            }
//            THEN("the destination file must exist") {
//                REQUIRE(fsm.exists(working_dir + "/mv2") == true);
//            }
//            THEN("the content of the destination must be the same of the origin") {
//                REQUIRE(fsm.readFile(working_dir+"/mv2") == mv1);
//            }
//        }
//
//        WHEN("the file is moved to an already existing destination") {
//            REQUIRE_THROWS(fsm.move(working_dir+"/mv2", working_dir+"/mv3"));
//        }
//
//        WHEN("Moving to not existing directories") {
//            REQUIRE_THROWS(fsm.move(working_dir + "/mv1", working_dir + "/dir/dir/dir/"));
//        }
//
//        WHEN("Moving to a directory without specifying the destination file") {
//            Buffer mv3 = fsm.readFile(working_dir+"/mv3");
//            fsm.move(working_dir+"/mv3", working_dir+"/dir/");//should place mv2 within the specified directory
//            THEN("the destination file must exist and have a valid content"){
//                REQUIRE(fsm.exists(working_dir+"/mv3") == false);
//                REQUIRE(fsm.exists(working_dir+"/dir/mv3") == true);
//                REQUIRE(fsm.readFile(working_dir+"/dir/mv3") == mv3);
//            }
//        }
//
//    }
//
//
//    GIVEN("A file that does not exist") {
//        WHEN("Moving it") {
//            REQUIRE_THROWS(fsm.move(working_dir + "/doesnotexist", working_dir + "/notexistingmove"));
//            THEN("The dest. path must not exist.") {
//                REQUIRE(fsm.exists(working_dir+"/notexistingmove") == false);
//            }
//        }
//    }
//    GIVEN("An ambiguous origin path and an invalid destination one") {
//        WHEN("Trying to move") {
//            REQUIRE_THROWS(fsm.move(working_dir+"/\0", ""));
//            THEN("The disambiguated origin must stille exist") {
//                REQUIRE(fsm.exists(working_dir+"/") == true);
//            }
//        }
//    }
//
//    fsm.removeDirectory(working_dir);
//}
//
//
//SCENARIO("File copy", "[fs_copy][fs]") {
//    using namespace atlas;
//    /* copy of normal file. */
//    fsm.removeDirectory(working_dir);
//    try{
//        fsm.copyDirectory(input_dir+"/copy", working_dir);
//    } catch (components::filesystem::ErrorCode &exc) {
//
//        std::cout << "\033[1;31m====================== ERROR WHILE COPYING =====================\033[0m" << std::endl;
//        std::cout << "not properly an error. However you should check what happened:" << std::endl;
//        std::cout << exc.what() << std::endl;
//        std::cout << "\033[1;31m================================================================\033[0m " << std::endl;
//        //REQUIRE(false);
//    }
//    GIVEN("An existing file") {
//        Buffer cp1 = fsm.readFile(working_dir+"/cp1");
//        WHEN("It is copied in a valid position, where there's nothing") {
//            REQUIRE_NOTHROW(fsm.copyFile(working_dir+"/cp1", working_dir+"/cp2"));
//            THEN("the original file must still exist") {
//                REQUIRE(fsm.exists( working_dir+"/cp1") == true);
//            }
//            AND_THEN("the destination file must exist") {
//                REQUIRE(fsm.exists( working_dir+"/cp2") == true);
//            }
//            AND_THEN("its content must correspond to the one of the origin") {
//                REQUIRE(fsm.readFile(working_dir+"/cp2") == cp1);
//            }
//        }
//
//        WHEN("It is copied over an already existing file") {
//            REQUIRE_NOTHROW(fsm.copyFile(working_dir+"/cp1", working_dir+"/cp4"));
//            THEN("The content of the destination must correspond to the origin") {
//                REQUIRE(fsm.readFile(working_dir+"/cp4") == cp1);
//            }
//        }
//
//        WHEN("It is copied into an existing directory") {
//            REQUIRE_NOTHROW(fsm.copyFile(working_dir+"/cp1", working_dir+"/dir/cp1"));
//            THEN("The content of the destination must correspond to the origin") {
//                REQUIRE(fsm.readFile(working_dir+"/dir/cp1") == cp1);
//            }
//        }
//
//        WHEN("It is copied into a not-existing directory") {
//            REQUIRE_THROWS(fsm.copyFile(working_dir+"/cp1", working_dir+"/dir2/cp2"));
//        }
//
//        WHEN("It is both origin and destination") {
//            REQUIRE_THROWS(fsm.copyFile(working_dir+"/cp1", working_dir+"/cp1"));
//        }
//
//
//    }
//
//    GIVEN("A file that does not exist") {
//        WHEN("copying it anywhere") {
//            for(int i = 0; i < 10; i++) {
//                REQUIRE_THROWS(fsm.copyFile(working_dir+"/cp2", working_dir+("/cp"+std::to_string(i))));
//            }
//        }
//
//        WHEN("copying it into an existing directory") {
//            REQUIRE_THROWS(fsm.copyFile(working_dir+"/cp2",working_dir+"/dir/"));
//        }
//    }
//
//    GIVEN("An existing directory containing other files") {
//        WHEN("The directory is copied in an empty location") {
//            REQUIRE_NOTHROW(fsm.copyDirectory(working_dir+"/cpdir", working_dir+"/cpdir2"));
//            THEN("The previous directory must still exist") {
//                REQUIRE(fsm.exists(working_dir+"/cpdir"));
//            } AND_THEN("Its content must still exist") {
//                REQUIRE(fsm.exists(working_dir+"/cpdir/1"));
//                REQUIRE(fsm.exists(working_dir+"/cpdir/1/1"));
//                REQUIRE(fsm.exists(working_dir+"/cpdir/2"));
//            } AND_THEN("The destination folder must exist") {
//                REQUIRE(fsm.exists(working_dir+"/cpdir2"));
//            } AND_THEN("It must contain the same files as the origin") {
//                REQUIRE(fsm.exists(working_dir+"/cpdir2/1"));
//                REQUIRE(fsm.exists(working_dir+"/cpdir2/1/1"));
//                REQUIRE(fsm.exists(working_dir+"/cpdir2/2"));
//            } AND_THEN("The content of the files must be the same") {
//                REQUIRE(fsm.readFile(working_dir+"/cpdir/1/1") == fsm.readFile(working_dir+"/cpdir2/1/1"));
//                REQUIRE(fsm.readFile(working_dir+"/cpdir/2") == fsm.readFile(working_dir+"/cpdir2/2"));
//            }
//        }
//        WHEN("A copy of a dir is performed using copyFile") {
//            REQUIRE_THROWS(fsm.copyFile(working_dir+"/cpdir", working_dir+"/cpdir3"));
//            THEN("the destination path must not exist") {
//                REQUIRE_FALSE(fsm.exists(working_dir+"/cpdir3"));
//            }
//        }
//
//        WHEN("A copy is performed over an existing directory") {
//            REQUIRE_NOTHROW(fsm.copyDirectory(working_dir+"/cpdir", working_dir+"/cpdir2"));
//            REQUIRE_NOTHROW(fsm.copyDirectory(working_dir+"/cpdir", working_dir+"/cpdir2")); //this resource must now exist!
//            THEN("The previous directory must still exist") {
//                REQUIRE(fsm.exists(working_dir+"/cpdir"));
//            } AND_THEN("Its content must still exist") {
//                REQUIRE(fsm.exists(working_dir+"/cpdir/1"));
//                REQUIRE(fsm.exists(working_dir+"/cpdir/1/1"));
//                REQUIRE(fsm.exists(working_dir+"/cpdir/2"));
//            } AND_THEN("The destination folder must exist") {
//                REQUIRE(fsm.exists(working_dir+"/cpdir2"));
//            } AND_THEN("It must contain the same stuff as before") {
//                REQUIRE(fsm.exists(working_dir+"/cpdir2/1"));
//                REQUIRE(fsm.exists(working_dir+"/cpdir2/1/1"));
//                REQUIRE(fsm.exists(working_dir+"/cpdir2/2"));
//            } AND_THEN("it must contain the copied directory") {
//                REQUIRE(fsm.exists(working_dir+"/cpdir2/cpdir"));
//                REQUIRE(fsm.exists(working_dir+"/cpdir2/cpdir/1"));
//                REQUIRE(fsm.exists(working_dir+"/cpdir2/cpdir/1/1"));
//                REQUIRE(fsm.exists(working_dir+"/cpdir2/cpdir/2"));
//            }
//        }
//    }
//
//    GIVEN("An origin directory that does not exist"){
//        WHEN("it is copied somewhere") {
//            REQUIRE_THROWS(fsm.copyDirectory(working_dir+"/cpdir23", working_dir+"/cpdir34"));
//            THEN("It mustn't exist") {
//                REQUIRE(fsm.exists(working_dir+"/cpdir23") == false);
//            } AND_THEN("the destination mustn't exist") {
//                REQUIRE(fsm.exists(working_dir+"/cpdir34") == false);
//            }
//        }
//
//        WHEN("it is copied over an existing directory") {
//            REQUIRE_THROWS(fsm.copyDirectory(working_dir+"/cpdir23", working_dir+"/cpdir1"));
//            THEN("the destination directory must still exist") {
//                REQUIRE(fsm.exists(working_dir+"/cpdir"));
//            } AND_THEN("it must be the same as before") {
//                REQUIRE(fsm.exists(working_dir+"/cpdir/1"));
//                REQUIRE(fsm.exists(working_dir+"/cpdir/1/1"));
//                REQUIRE(fsm.exists(working_dir+"/cpdir/2"));
//            }
//        }
//
//    }
//    fsm.removeDirectory(working_dir);
//}
//
//
//SCENARIO("File write", "[fs_write][fs]") {
//    using namespace atlas;
//    /* copy of normal file. */
//    fsm.removeDirectory(working_dir);
//    try{
//        //we use the data in read to work on it
//        fsm.copyDirectory(input_dir+"/read", working_dir);
//    } catch (components::filesystem::ErrorCode &exc) {
//
//        std::cout << "\033[1;31m====================== ERROR WHILE COPYING =====================\033[0m" << std::endl;
//        std::cout << "not properly an error. However you should check what happened:" << std::endl;
//        std::cout << exc.what() << std::endl;
//        std::cout << "\033[1;31m================================================================\033[0m " << std::endl;
//        //REQUIRE(false);
//    }
//    std::string content = "ciapooinasoinasoinnoasnoasnoasoiasnoinasoinasoinasoiasnoiasnasnoinasoiasoinasonas";
//    Buffer b;
//    for(auto i = content.begin(); i != content.end(); i++) b.push_back(*i);
//
//    GIVEN("A file that exists") {
//        WHEN("we write into it") {
//            REQUIRE_NOTHROW(fsm.writeFile(working_dir+"/multiplea", b));
//            THEN("The content of the file must be exactly the same of the written buffer") {
//                REQUIRE(fsm.readFile(working_dir+"/multiplea") == b);
//            }
//        }
//
//        WHEN("We perform two subsequent write") {
//            Buffer c;
//            for(int i = 0; i < 10; i++) b.push_back((uint8_t) i);
//
//            REQUIRE_NOTHROW(fsm.writeFile(working_dir+"/multiplea", b));
//            REQUIRE_NOTHROW(fsm.writeFile(working_dir+"/multiplea", c));
//            THEN("The content of the file must correspond to the second one") {
//                REQUIRE(fsm.readFile(working_dir+"/multiplea") == c);
//            }
//        }
//
//        WHEN("We perform a write with version") {
//            Buffer c{40, 0, 0, 0, 0, 0, 0, 0, 40, 0, 0, 0, 0, 0, 0, 0}; //version 40
//            REQUIRE_NOTHROW(fsm.writeFile(working_dir+"/multiplea", 40, b));
//            THEN("The file content must contain the version!") {
//                Buffer total = c;
//                total.insert(total.end(), b.begin(), b.end());
//                REQUIRE(fsm.readFile(working_dir+"/multiplea") == total);
//            }
//        }
//    }
//
//    GIVEN("A directory") {
//        WHEN("We write into it") {
//            REQUIRE_THROWS(fsm.writeFile(working_dir+"/dir", b));
//            THEN("The directory must still exist") {
//                REQUIRE(fsm.exists(working_dir+"/dir"));
//            } AND_THEN("It must be a directory") {
//                REQUIRE_THROWS(fsm.readFile(working_dir+"/dir"));
//            }
//        }
//    }
//
//    GIVEN("A special file") {
//        mkfifo((working_dir+"/specialfile").c_str(), EACCES);
//        WHEN("We perform a write"){
//            REQUIRE_THROWS(fsm.writeFile(working_dir+"/specialfile", b));
//        }
//    }
//
//    GIVEN("A symlink") {
//
//        auto a = symlink((working_dir + "multiplea").c_str(), (working_dir+"/linktomultiplea").c_str());
//        REQUIRE(a == 0);
//        WHEN("We perform a write"){
//            REQUIRE_THROWS(fsm.writeFile(working_dir+"/linktomultiplea", b));
//        }
//    }
//
//    GIVEN("A path for a new file") {
//        WHEN("We perform a write") {
//            REQUIRE_NOTHROW(fsm.writeFile(working_dir+"/newfile.txt", b));
//            THEN("The file must exist") {
//                REQUIRE(fsm.exists(working_dir+"/newfile.txt"));
//            } AND_THEN("Its content must correspond to the one used for creation") {
//                REQUIRE(fsm.readFile(working_dir+"/newfile.txt") == b);
//            }
//        }
//    }
//
//    GIVEN("A path for a new file comprising a directory that does not exist") {
//        WHEN("We perform a write") {
//            REQUIRE_THROWS(fsm.writeFile(working_dir+"/dashauishuiahdas/fshauishdas/newfile.txt", b));
//        }
//    }
//
//    GIVEN("An empty buffer") {
//        Buffer empty;
//        WHEN("We perform a write on a existing file") {
//            REQUIRE_NOTHROW(fsm.writeFile(working_dir+"/prova.txt", empty));
//            THEN("The file must exist") {
//                REQUIRE(fsm.exists(working_dir+"/prova.txt"));
//            } AND_THEN("It must be empty") {
//                REQUIRE(fsm.readFile(working_dir+"/prova.txt").size() == 0);
//            }
//        }
//
//        WHEN("We perform a write on a new file") {
//            REQUIRE_NOTHROW(fsm.writeFile(working_dir+"/newfile2.txt", empty));
//            THEN("It must exist") {
//                REQUIRE(fsm.exists(working_dir+"/newfile2.txt"));
//            } AND_THEN("It must be empty") {
//                REQUIRE(fsm.readFile(working_dir+"/newfile2.txt").size() == 0);
//            }
//        }
//    }
//
//    GIVEN("A buffer to be written to disk") {
//        Buffer buf(1000);
//        WHEN("I write two times the buffer to the same path") {
//            auto& fs = fsm;
//            auto path = working_dir + "/two_times.txt";
//            fs.writeFile(path, 5, buf);
//            fs.writeFile(path, 5, buf);
//            THEN("the size of the written file is equal to the size of one single buffer") {
//                auto size = boost::filesystem::file_size(path);
//                REQUIRE(size == buf.size() + 2*sizeof(FileVersion));
//            }
//        }
//    }
//
//    fsm.removeDirectory(working_dir);
//}
//
//SCENARIO("Append to file", "[fs_append][fs]") {
//    using namespace atlas;
//    /* copy of normal file. */
//    fsm.removeDirectory(working_dir);
//    try{
//        //we use the data in read to work on it
//        fsm.copyDirectory(input_dir+"/read", working_dir);
//    } catch (components::filesystem::ErrorCode &exc) {
//
//        std::cout << "\033[1;31m====================== ERROR WHILE COPYING =====================\033[0m" << std::endl;
//        std::cout << "not properly an error. However you should check what happened:" << std::endl;
//        std::cout << exc.what() << std::endl;
//        std::cout << "\033[1;31m================================================================\033[0m " << std::endl;
//        //REQUIRE(false);
//    }
//    std::string content = "ciapooinasoinasoinnoasnoasnoasoiasnoinasoinasoinasoiasnoiasnasnoinasoiasoinasonas";
//    Buffer b;
//    b.insert(b.begin(), content.begin(), content.end());
//    /* Test begin!*/
//
//    GIVEN("A file that exists") {
//        Buffer previous;
//        REQUIRE_NOTHROW(previous = fsm.readFile(working_dir+"/multiplea"));
//        WHEN("we append to it") {
//            REQUIRE_NOTHROW(fsm.appendToFile(working_dir+"/multiplea", b));
//            THEN("The content of the file must correspond to its previous one plus the appended string") {
//                previous.insert(previous.end(), b.begin(), b.end());
//                REQUIRE(fsm.readFile(working_dir+"/multiplea") == previous);
//            }
//        }
//
//        WHEN("We perform two subsequent append") {
//            Buffer c;
//            for(int i = 0; i < 10; i++) b.push_back((uint8_t) i);
//            REQUIRE_NOTHROW(fsm.appendToFile(working_dir+"/multiplea", b));
//            REQUIRE_NOTHROW(fsm.appendToFile(working_dir+"/multiplea", c));
//            THEN("The content of the file must correspond to the second one") {
//                previous.insert(previous.end(), b.begin(), b.end());
//                previous.insert(previous.end(), c.begin(), c.end());
//                REQUIRE(fsm.readFile(working_dir+"/multiplea") == previous);
//            }
//        }
//
//        WHEN("We perform an append with version") {
//            Buffer c;
//            for(int i = 0; i < 10; i++) b.push_back((uint8_t) i);
//            REQUIRE_NOTHROW(fsm.appendToFile(working_dir+"/multiplea", 2, b));
//            THEN("The version must be correct and the append must be correct") {
//                //manually set version to two in the control vector
//                std::array<uint8_t, 16> vers{2,0,0,0,0,0,0,0, 2,0,0,0,0,0,0,0};
//                unsigned int i = 0;
//                for(auto b = previous.begin(); b != previous.end() && i <sizeof(vers); ++b, ++i) *b = vers[i];
//                //insert in the control vector the appended contnet
//                previous.insert(previous.end(), b.begin(), b.end());
//                REQUIRE(fsm.readFile(working_dir+"/multiplea") == previous);
//            }
//        }
//
//
//        WHEN("We perform two append with version") {
//            Buffer c;
//            for(int i = 0; i < 1000; i++) c.push_back((uint8_t) i%256);
//            REQUIRE_NOTHROW(fsm.appendToFile(working_dir+"/multiplea", 2, b));
//            REQUIRE_NOTHROW(fsm.appendToFile(working_dir+"/multiplea", 40, c));
//            THEN("The version must be correct and the two append must be correct") {
//                //manually set version to two in the control vector
//                std::array<uint8_t, 16> vers{40,0,0,0,0,0,0,0,40,0,0,0,0,0,0,0};
//                unsigned int i = 0;
//                for(auto it = previous.begin(); it != previous.end() && i < sizeof(vers); ++it, ++i) *it = vers[i];
//                //insert in the control vector the appended contnet
//                previous.insert(previous.end(), b.begin(), b.end());
//                previous.insert(previous.end(), c.begin(), c.end());
//                REQUIRE(fsm.readFile(working_dir+"/multiplea") == previous);
//            }
//        }
//    }
//
//    GIVEN("A directory") {
//        WHEN("We append to it") {
//            REQUIRE_THROWS(fsm.appendToFile(working_dir+"/dir", b));
//            THEN("The directory must still exist") {
//                REQUIRE(fsm.exists(working_dir+"/dir"));
//            } AND_THEN("It must be a directory") {
//                REQUIRE_THROWS(fsm.readFile(working_dir+"/dir"));
//            }
//        }
//    }
//
//    GIVEN("A special file") {
//        mkfifo((working_dir+"/specialfile").c_str(), EACCES);
//        WHEN("We perform an append"){
//            REQUIRE_THROWS(fsm.appendToFile(working_dir+"/specialfile", b));
//        }
//    }
//
//    GIVEN("A symlink") {
//        auto a = symlink((working_dir + "multiplea").c_str(), (working_dir+"/linktomultiplea").c_str());
//        REQUIRE(a == 0);
//        WHEN("We perform an append"){
//            REQUIRE_THROWS(fsm.appendToFile(working_dir+"/linktomultiplea", b));
//        }
//    }
//
//    GIVEN("A path for a new file") {
//        WHEN("We perform an append") {
//            REQUIRE_NOTHROW(fsm.appendToFile(working_dir+"/newfile.txt", b));
//            THEN("The file must exist") {
//                REQUIRE(fsm.exists(working_dir+"/newfile.txt"));
//            } AND_THEN("Its content must correspond to the one used for creation") {
//                REQUIRE(fsm.readFile(working_dir+"/newfile.txt") == b);
//            }
//        }
//    }
//
//    GIVEN("A path for a new file comprising a directory that does not exist") {
//        WHEN("We perform a write") {
//            REQUIRE_THROWS(fsm.appendToFile(working_dir+"/dashauishuiahdas/fshauishdas/newfile.txt", b));
//        }
//    }
//
//    GIVEN("An empty buffer") {
//        Buffer empty;
//        Buffer prev;
//        REQUIRE_NOTHROW(prev = fsm.readFile(working_dir+"/prova.txt"));
//        WHEN("We perform an append on a existing file") {
//            REQUIRE_NOTHROW(fsm.appendToFile(working_dir+"/prova.txt", empty));
//            THEN("The file must exist") {
//                REQUIRE(fsm.exists(working_dir+"/prova.txt"));
//            } AND_THEN("Its content must remain the same") {
//                REQUIRE(fsm.readFile(working_dir+"/prova.txt") == prev);
//            }
//        }
//
//        WHEN("We perform an append on a new file") {
//            REQUIRE_NOTHROW(fsm.writeFile(working_dir+"/newfile2.txt", empty));
//            THEN("It must exist") {
//                REQUIRE(fsm.exists(working_dir+"/newfile2.txt"));
//            } AND_THEN("It must be empty") {
//                REQUIRE(fsm.readFile(working_dir+"/newfile2.txt").size() == 0);
//            }
//        }
//    }
//
//    fsm.removeDirectory(working_dir);
//}
