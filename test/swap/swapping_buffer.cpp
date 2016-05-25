#include "catch.hpp"
#include "mocks/mock_filesystem.h"
#include "io/async/swap/swapping_buffer_append.h"
#include "io/async/swap/swapping_buffer_overwrite.h"
#include "boost/asio.hpp"

using cynny::cynnypp::filesystem::ErrorCode;

struct ChunkVerifier {
    ChunkVerifier(int &numReads, int expectedReads, std::shared_ptr<ChunkedFstreamInterface> chk, std::function<void()> onEnd,
                  Buffer::iterator &begin, Buffer::iterator &end, boost::asio::io_service& io) :
                    numReads(numReads), expectedReads(expectedReads), chk(chk), onEnd(onEnd), begin(begin), end(end), io{io}{

    }

    int &numReads, expectedReads;
    std::shared_ptr<ChunkedFstreamInterface> chk;
    std::function<void()> onEnd;
    Buffer::iterator begin, end;
    boost::asio::io_service& io;


    void readHandler(ErrorCode ec, Buffer &data) {
        numReads++;
        auto i = data.begin();
        bool equalChunk = true;
        for(; i != data.end() && begin != end; ++i, ++begin) {
            equalChunk = equalChunk && *i == *begin;
        }
        REQUIRE(equalChunk);
        REQUIRE((i == data.end())); //consume all data without failing.
        //then call nex-chunkk.
        if(!ec) {
            io.post([this](){chk->next_chunk([this](const ErrorCode& ec, Buffer data){ this->readHandler(ec, data);});});
        } else {
            REQUIRE((begin == end)); //consumed.
            if(begin == end) {
                onEnd();
            }
        }
    }

};


using namespace cynny::cynnypp::swapping;

int numOps = 0;
int prevNumOps = 0;

int numReads = 0;
int expectedReads = 0;

static boost::asio::io_service ioService;
static MockFilesystem mockFilesystem{ioService};
static const auto rootDir = "./tmp/";

TEST_CASE("Append Buffer", "[swapping_buffer][sb]") {
    ioService.reset();
    mockFilesystem.clear();

    Buffer b{'c', 'o', 'n', 't', 'e', 'n', 'u', 't', 'o'};
    mockFilesystem.writeFile("/prova/prova/provafileWithAppend.txt", b); //write to file a certain content.
    REQUIRE(mockFilesystem.exists("/prova/prova/provafileWithAppend.txt"));
    Buffer appendContent;
    GIVEN("A buffer whose size is less than the swapping limit on which we perform an append") {

        for (auto i = 0; i < 13812; i++) { appendContent.push_back((uint8_t) (i % 256)); } //initialize buffer
        auto ta = SwappingBufferAppend::make_shared(ioService, mockFilesystem, rootDir, "/prova/prova/provafileWithAppend.txt");
        REQUIRE((mockFilesystem.readFile("/prova/prova/provafileWithAppend.txt") == Buffer{'c', 'o', 'n', 't', 'e', 'n', 'u', 't', 'o'}));
        int numReads = 0;
        int expectedReads = 0;

        Buffer expected(b.begin(), b.end());
        expected.insert(expected.end(), appendContent.begin(), appendContent.end());
        auto keepAlive = new boost::asio::io_service::work(ioService);
        ta->append(appendContent, [keepAlive, &b, &appendContent, &expected, &numReads, &expectedReads, ta](size_t size) {
            WHEN("The content is retrieved with a readAll, it is correct") {
                ta->readAll([keepAlive, &b, &appendContent, &expected](Buffer totalFile) {
                    REQUIRE((expected == totalFile));
                    ++numOps;
                    delete keepAlive;
                }, [](const ErrorCode& er) {
                    std::cout << er.what() << std::endl;
                    FAIL("Read operation should not fail on append buffer.");
                });
            }

            AND_WHEN("The content is saved") {
                ta->saveAllContents("/prova/prova/provafileWithAppend.txt", [keepAlive,&expected](){
                    auto buf = mockFilesystem.readFile("/prova/prova/provafileWithAppend.txt");
                    REQUIRE((expected == buf));
                    ++numOps;
                    delete keepAlive;
                }, [](ErrorCode){ FAIL("The transactionappendbuffer should be able to write down the data."); });
            }

            AND_WHEN("The content is cleared") {
                ta->clear([ta, keepAlive, &b](){
                        ta->size([ta, keepAlive, &b](size_t size){
                            REQUIRE((size == 0));
                            ta->saveAllContents("/prova/prova/provafileWithAppend.txt", [keepAlive, &b]() {
                                auto buf = mockFilesystem.readFile("/prova/prova/provafileWithAppend.txt");
                                Buffer expected{b};
                                REQUIRE((expected == buf));
                                ++numOps;
                                delete keepAlive;
                            }, [](ErrorCode te){ FAIL("The transactionappendbuffer should be able to avoid to write down the data :D"); });

                        });

                });
            }
            AND_WHEN("We create a chunked reader with chunk size dividing swap size") {
                size_t chunkSize = 0;
                chunkSize = 4096;
                expectedReads = (expected.size() + chunkSize/2)/chunkSize;
                auto shd = new sharedinfo(ta);
                ta->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [&numReads, &expectedReads, keepAlive, &expected](std::shared_ptr<ChunkedFstreamInterface> chk){
                    auto expectedBegin = expected.begin();
                    auto expectedEnd = expected.end();
                    ChunkVerifier *verifier = nullptr;
                    verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; ++numOps; delete keepAlive;  },  expectedBegin, expectedEnd, ioService);
                    chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
                }, [](ErrorCode){FAIL("Make chunked stream is not expectetd to fail.");});

            }

            AND_WHEN("We create a chunked reader with chunk size not dividing swap size") {
            size_t chunkSize = 0;
            chunkSize = 17*13*19;
            expectedReads = (expected.size() + chunkSize/2)/chunkSize;
            auto shd = new sharedinfo(ta);
            ta->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [&numReads, &expectedReads, keepAlive, &expected](std::shared_ptr<ChunkedFstreamInterface> chk){
            auto expectedBegin = expected.begin();
            auto expectedEnd = expected.end();
            ChunkVerifier *verifier = nullptr;
            verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; ++numOps; delete keepAlive;},  expectedBegin, expectedEnd, ioService);
            chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
            }, [](ErrorCode){FAIL("Make chunked stream is not expectetd to fail.");});

            }
        }, [](ErrorCode){});

        ioService.run();
        REQUIRE(numOps == prevNumOps+1); //ensure one operation is always performed.
        prevNumOps++;
    }


    GIVEN("A buffer whose size is slightly greather than the swapping limit of the buffer") {
        size_t appendSize = SwappingBufferAppend::maxBufferSize * 1.1; //slightly greather :D
        for(size_t i = 0; i < appendSize; i++) {
            appendContent.push_back((uint8_t) (i%256) );
        }
        auto shrd = SwappingBufferAppend::make_shared(ioService, mockFilesystem, rootDir, "/prova/prova/provafileWithAppend.txt");
        REQUIRE((mockFilesystem.readFile("/prova/prova/provafileWithAppend.txt") == Buffer{'c', 'o', 'n', 't', 'e', 'n', 'u', 't', 'o'}));
        Buffer expected(b.begin(), b.end());
        expected.insert(expected.end(), appendContent.begin(), appendContent.end());
        auto keepAlive = new boost::asio::io_service::work(ioService);
        int numReads = 0;
        int expectedReads = 0;

        shrd->append(appendContent, [&](size_t size){


        WHEN("The content is retrieved with a readAll, it is correct") {
            shrd->readAll([keepAlive, &b, &appendContent, &expected](Buffer totalFile) {

                REQUIRE((expected.size() == totalFile.size()));
                REQUIRE((expected == totalFile));
                numOps++;
                delete keepAlive;
            }, [](ErrorCode er) {
                std::cout << er.what() << std::endl;
                FAIL("Read operation should not fail on append buffer.");
            });
        }

        AND_WHEN("The content is saved with version 1") {
            shrd->saveAllContents("/prova/prova/provafileWithAppend.txt", [keepAlive,&expected](){
                auto buf = mockFilesystem.readFile("/prova/prova/provafileWithAppend.txt");


                REQUIRE((expected.size() == buf.size()));
                REQUIRE((expected == buf));
                ++numOps;
                delete keepAlive;
            }, [](ErrorCode){ FAIL("The transactionappendbuffer should be able to write down the data."); });

        }

        AND_WHEN("The content is saved with version 80") {
            shrd->saveAllContents("/prova/prova/provafileWithAppend.txt",  [keepAlive,&expected](){
                auto buf = mockFilesystem.readFile("/prova/prova/provafileWithAppend.txt");


                REQUIRE((expected == buf));
                ++numOps;
                delete keepAlive;
            }, [](ErrorCode){ FAIL("The transactionappendbuffer should be able to write down the data."); });
        }

        AND_WHEN("The content is cleared") {
            shrd->clear([&](){
            //need to post otherwise we won't be sure that it will not be executed before clear.
            shrd->size([&shrd, &b, keepAlive](size_t size) {
                REQUIRE(((size == 0)));
                shrd->saveAllContents("/prova/prova/provafileWithAppend.txt", [keepAlive, &b]() {
                    auto buf = mockFilesystem.readFile(
                            "/prova/prova/provafileWithAppend.txt");
                    Buffer expected{b};

                    REQUIRE( (expected == buf) );
                    ++numOps;
                    delete keepAlive;
                }, [](ErrorCode te){ FAIL("The transactionappendbuffer should be able to avoid to write down the data :D"); });
            });
            });
        }


        AND_WHEN("We create a chunked reader with chunk size dividing swap size") {
            size_t chunkSize = 0;
            chunkSize = 4096;
            expectedReads = (expected.size() + chunkSize/2)/chunkSize;
            auto shd = new sharedinfo(shrd);
            shrd->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [&](std::shared_ptr<ChunkedFstreamInterface> chk){
                auto expectedBegin = expected.begin();
                auto expectedEnd = expected.end();
                ChunkVerifier *verifier = nullptr;
                verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; ++numOps; delete keepAlive;},  expectedBegin, expectedEnd, ioService);
                chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
            }, [](ErrorCode){});
        }

        AND_WHEN("We create a chunked reader with chunk size dividing swap size") {
            size_t chunkSize = 0;
            chunkSize = 17*13*19;
            expectedReads = (expected.size() + chunkSize/2)/chunkSize;
            auto shd = new sharedinfo(shrd);
            shrd->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [&](std::shared_ptr<ChunkedFstreamInterface> chk){
            auto expectedBegin = expected.begin();
            auto expectedEnd = expected.end();
            ChunkVerifier *verifier = nullptr;
            verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; ++numOps; delete keepAlive;},  expectedBegin, expectedEnd, ioService);
            chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
            }, [](ErrorCode){});
            }
        
        }, [](const ErrorCode& te){}); //end of append

        ioService.run();
        REQUIRE(numOps == prevNumOps+1); //ensure one operation is always performed.
        prevNumOps++;
    }


    GIVEN("A buffer whose size is a multiple of the swapping limit") {
        size_t appendSize = SwappingBufferAppend::maxBufferSize * 4;
        for(size_t i = 0; i < appendSize; i++) {
            appendContent.push_back((uint8_t) (i%256) );
        }
        std::shared_ptr<SwappingBufferAppend> shrd = SwappingBufferAppend::make_shared(ioService, mockFilesystem, rootDir, "/prova/prova/provafileWithAppend.txt");
        auto &ta = *shrd;
        REQUIRE((mockFilesystem.readFile("/prova/prova/provafileWithAppend.txt") == Buffer{'c', 'o', 'n', 't', 'e', 'n', 'u', 't', 'o'}));
        int numReads = 0;
        int expectedReads = 0;    
        Buffer expected(b.begin(), b.end());
        expected.insert(expected.end(), appendContent.begin(), appendContent.end());
        auto keepAlive = new boost::asio::io_service::work(ioService);
        ta.append(appendContent, [&](size_t size){
        WHEN("The content is retrieved with a readAll, it is correct") {
            ta.readAll([keepAlive, &b, &appendContent, &expected](Buffer totalFile) {
                REQUIRE((expected.size() == totalFile.size()));
                REQUIRE((expected == totalFile));
                ++numOps;
                delete keepAlive;
            }, [](ErrorCode er) {
                std::cout << er.what() << std::endl;
                FAIL("Read operation should not fail on append buffer.");
            });
        }

        AND_WHEN("The content is saved with version 1") {
            ta.saveAllContents("/prova/prova/provafileWithAppend.txt", [keepAlive,&expected](){
                auto buf = mockFilesystem.readFile("/prova/prova/provafileWithAppend.txt");


                REQUIRE((expected == buf));++numOps;
                delete keepAlive;
            }, [](ErrorCode){ FAIL("The transactionappendbuffer should be able to write down the data."); });

        }

        AND_WHEN("The content is saved with version 80") {
            ta.saveAllContents("/prova/prova/provafileWithAppend.txt",  [keepAlive,&expected](){
                auto buf = mockFilesystem.readFile("/prova/prova/provafileWithAppend.txt");


                REQUIRE((expected == buf));++numOps;
                delete keepAlive;
            }, [](ErrorCode){ FAIL("The transactionappendbuffer should be able to write down the data."); });
        }

        AND_WHEN("The content is cleared") {
            ta.clear([&](){;
            //need to post otherwise we won't be sure that it will not be executed before clear.
           ta.size([&ta, &b, keepAlive](size_t size) {
                REQUIRE((size == 0));
                ta.saveAllContents("/prova/prova/provafileWithAppend.txt", [keepAlive, &b]() {
                    auto buf = mockFilesystem.readFile(
                            "/prova/prova/provafileWithAppend.txt");
                    Buffer expected{b};

                    REQUIRE( (expected == buf ));
                    ++numOps;
                    delete keepAlive;
                }, [](ErrorCode te){ FAIL("The transactionappendbuffer should be able to avoid to write down the data :D"); });
            });
            });
        }



        AND_WHEN("We create a chunked reader with chunk size dividing swap size") {
            size_t chunkSize = 0;
            chunkSize = 4096;
            expectedReads = (expected.size() + chunkSize/2)/chunkSize;
            auto shd = new sharedinfo(shrd);
            shrd->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [&](std::shared_ptr<ChunkedFstreamInterface> chk){
                auto expectedBegin = expected.begin();
                auto expectedEnd = expected.end();
                ChunkVerifier *verifier = nullptr;
                verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; ++numOps;delete keepAlive;},  expectedBegin, expectedEnd, ioService);
                chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
            }, [](const ErrorCode& re){});
            
        }


         AND_WHEN("We create a chunked reader with chunk size dividing swap size") {
            size_t chunkSize = 0;
            chunkSize = 13*17*19;
            expectedReads = (expected.size() + chunkSize/2)/chunkSize;
            auto shd = new sharedinfo(shrd);

            shrd->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [&](std::shared_ptr<ChunkedFstreamInterface> chk){

                auto expectedBegin = expected.begin();
                auto expectedEnd = expected.end();
                ChunkVerifier *verifier = nullptr;
                verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; ++numOps;delete keepAlive;},  expectedBegin, expectedEnd, ioService);
                chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
            }, [](ErrorCode re){});
            
        }
        }, [](ErrorCode te){}); 

        ioService.run();
        REQUIRE(numOps == prevNumOps +1 );
        prevNumOps++; 
    }

    GIVEN("A very, very long buffer") {
        bool nothing = true;
        size_t appendSize = SwappingBufferAppend::maxBufferSize * 16 + 13; //slightly greather :D
        for(size_t i = 0; i < SwappingBufferAppend::maxBufferSize; ++i) appendContent.push_back((uint8_t) (i % 256));
        auto shrd = SwappingBufferAppend::make_shared(ioService, mockFilesystem, rootDir, "/prova/prova/provafileWithAppend.txt");
        auto &ta = *shrd;
        Buffer &expected = appendContent;
        auto testLambda = [&](size_t size){
            appendContent.reserve(appendSize+b.size());
            appendContent.insert(appendContent.begin(), b.begin(), b.end());
            while (appendContent.size()-b.size() < appendSize/2) {
                auto orig_end = appendContent.end();
                auto orig_begin = appendContent.begin()+b.size();
                appendContent.insert(orig_end, orig_begin, orig_end);   
            }
            for(size_t size = appendContent.size()-b.size(); size < appendSize; ++size) appendContent.push_back(size%256);
            std::cout << "creating keepalive" << std::endl;


            WHEN("The content is retrieved with a readAll") {
                auto keepAlive = new boost::asio::io_service::work(ioService); nothing = false;
                REQUIRE(true);
                ta.readAll([keepAlive, &b, &appendContent, &expected](Buffer totalFile) {
                    THEN("It is correct") {
                        REQUIRE((expected.size() == totalFile.size()));
                        REQUIRE((expected == totalFile));  
                    }
                    delete keepAlive;
                    numOps++;
                }, [](ErrorCode er) {
                    std::cout << er.what() << std::endl;
                    FAIL("Read operation should not fail on append buffer.");
                });
            }

            AND_WHEN("The content is saved with version 1") {
                auto keepAlive = new boost::asio::io_service::work(ioService);nothing = false;
                REQUIRE(true);
                ta.size([=](size_t sz){ std::cout << appendSize << " vs " << sz << std::endl;});
                ta.saveAllContents("/prova/prova/provafileWithAppend.txt", [keepAlive,&expected](){
                    auto buf = mockFilesystem.readFile("/prova/prova/provafileWithAppend.txt");


                    REQUIRE(expected.size() == buf.size());
                    REQUIRE((expected == buf)); numOps++;
                    delete keepAlive;
                }, [](ErrorCode){ FAIL("The transactionappendbuffer should be able to write down the data."); });

            }

            AND_WHEN("The content is saved with version 80") {
                auto keepAlive = new boost::asio::io_service::work(ioService);nothing = false;
                REQUIRE(true);
                ta.saveAllContents("/prova/prova/provafileWithAppend.txt",  [keepAlive,&expected](){
                    auto buf = mockFilesystem.readFile("/prova/prova/provafileWithAppend.txt");


                    REQUIRE((expected == buf)); numOps++;
                    delete keepAlive;
                }, [](ErrorCode){ FAIL("The transactionappendbuffer should be able to write down the data."); });
            }

            AND_WHEN("The content is cleared") {
                auto keepAlive = new boost::asio::io_service::work(ioService);nothing = false;
                REQUIRE(true);
                ta.clear([&](){
                    std::cout << "cleared" << std::endl;
                //need to post otherwise we won't be sure that it will not be executed before clear.
                    ta.size([&ta, &b, keepAlive](size_t sz) {
                        std::cout << "found size" << sz <<  std::endl;
                        REQUIRE(((sz == 0)));
                        ta.saveAllContents("/prova/prova/provafileWithAppend.txt", [keepAlive, &b]() {
                            auto buf = mockFilesystem.readFile("/prova/prova/provafileWithAppend.txt");
                            Buffer expected{b};
                            REQUIRE((expected.size() == buf.size())); 
                            REQUIRE( (expected == buf) ); numOps++;
                            delete keepAlive;
                        }, [](ErrorCode te){ FAIL("The transactionappendbuffer should be able to avoid to write down the data :D"); });
                    });
                }); 
            }
        };
        std::function<void(size_t size)> appendFunction;
        appendFunction = [&](size_t size){

                if(size >= appendSize-13) { //reached end!
                    Buffer b{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
                    ta.append(b, testLambda, [](const ErrorCode& te){ FAIL("Append should not fail!");});
                } else {

                    ta.append(appendContent, appendFunction, [](const ErrorCode& te){});
                }
        };

        ta.append(appendContent, appendFunction, [](const ErrorCode&){}); //appendi ogni volta

        ioService.run();
        REQUIRE((numOps == prevNumOps+1 || nothing));
        prevNumOps++; 
    }


};

TEST_CASE("Overwrite Buffer on existing file", "[swapping_buffer][sb]"){

    ioService.reset();
    mockFilesystem.clear();

    Buffer b{'c', 'o', 'n', 't', 'e', 'n', 'u', 't', 'o'};
    mockFilesystem.writeFile("/prova/prova/provaOverwrite.txt", b); //write to file a certain content.
    REQUIRE(mockFilesystem.exists("/prova/prova/provaOverwrite.txt"));
    Buffer appendContent;
                int numReads = 0;
            int expectedReads = 0;

    GIVEN("An append of size less than the swapping treshold") {
        for (auto i = 0; i < 139812; i++) { appendContent.push_back((uint8_t) (i % 256)); } //initialize buffer
        auto shrd = SwappingBufferOverwrite::make_shared(ioService, mockFilesystem, rootDir);
        REQUIRE((mockFilesystem.readFile("/prova/prova/provaOverwrite.txt") == Buffer{'c', 'o', 'n', 't', 'e', 'n', 'u', 't', 'o'}));
        shrd->append(appendContent, [&](size_t size){
            Buffer &expected = appendContent;
            
            WHEN("A readall is performed the content is correct") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                shrd->readAll([keepAlive, &b, &appendContent, &expected](Buffer totalFile) {

                    REQUIRE((expected.size() == totalFile.size()));
                    REQUIRE((expected == totalFile));
                    delete keepAlive;
                }, [](const ErrorCode& er) {
                    std::cout << er.what() << std::endl;
                    FAIL("Read operation should not fail on overwrite buffer.");
                });
            }

            AND_WHEN("The content is saved with version 1") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                shrd->saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive,&expected](){
                    auto buf = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");


                    REQUIRE((expected == buf));
                    delete keepAlive;
                }, [](const ErrorCode&){ FAIL("The transactionappendbuffer should be able to write down the data."); });

            }

            AND_WHEN("The content is saved with version 80") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                shrd->saveAllContents("/prova/prova/provaOverwrite.txt",  [keepAlive,&expected](){
                    auto buf = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");


                    REQUIRE((expected == buf));
                    delete keepAlive;
                }, [](const ErrorCode&){ FAIL("The transactionappendbuffer should be able to write down the data."); });
            }

            AND_WHEN("The content is cleared") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                shrd->clear([keepAlive, &shrd, &expected, &b](){
                //need to post otherwise we won't be sure that it will not be executed before clear.
                    shrd->size([&shrd, &b, keepAlive](size_t size) {
                        REQUIRE((size == 0));
                        shrd->saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive, &b]() {
                            auto buf = mockFilesystem.readFile(
                                    "/prova/prova/provaOverwrite.txt");
                            Buffer expected{};
                            REQUIRE((expected == buf));
                            delete keepAlive;
                        }, [](const ErrorCode& te){ FAIL("The transactionappendbuffer should be able to avoid to write down the data :D"); });
                    });
                });
            }
            
            AND_WHEN("We create a chunked reader with chunk size dividing swap size") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                size_t chunkSize = 0;
                chunkSize = 4096;
                expectedReads = (expected.size() + chunkSize/2)/chunkSize;
                auto shd = new sharedinfo(shrd);

                shrd->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [keepAlive, &shrd, &expected, &numReads, &expectedReads](std::shared_ptr<ChunkedFstreamInterface> chk){;

                    auto expectedBegin = expected.begin();
                    auto expectedEnd = expected.end();
                    ChunkVerifier *verifier = nullptr;
                    verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; delete keepAlive;},  expectedBegin, expectedEnd, ioService);
                    chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
                }, [](const ErrorCode&){ FAIL("NOT OK."); });
            }


            AND_WHEN("We create a chunked reader with chunk size not dividing swap size") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                size_t chunkSize = 0;
                chunkSize = 17*13*19;
                expectedReads = (expected.size() + chunkSize/2)/chunkSize;
                auto shd = new sharedinfo(shrd);

                shrd->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [&shrd, &expected, keepAlive, &numReads, &expectedReads](std::shared_ptr<ChunkedFstreamInterface> chk){;

                    auto expectedBegin = expected.begin();
                    auto expectedEnd = expected.end();
                    ChunkVerifier *verifier = nullptr;
                    verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; delete keepAlive;},  expectedBegin, expectedEnd, ioService);
                    chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
                }, [](const ErrorCode&){ FAIL("Chunked stream creation should not fail.");});
            }
        }, [](const ErrorCode&){ FAIL("Append should not fail!");});

        ioService.run();
    }


    GIVEN("A buffer whose size is slightly greather than the swapping limit of the buffer") {
        size_t appendSize = SwappingBufferAppend::maxBufferSize * 1.1; //slightly greather :D
        for(size_t i = 0; i < appendSize; i++) {
            appendContent.push_back((uint8_t) (i%256) );
        }
        auto shrd = SwappingBufferOverwrite::make_shared(ioService, mockFilesystem, rootDir);
        auto &ta = *shrd;
        ta.append(appendContent, [&](size_t size){;
        Buffer &expected = appendContent;

        int numReads = 0;
        int expectedReads = 0;

        WHEN("The content is retrieved with a readAll, it is correct") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
            ta.readAll([keepAlive, &b, &appendContent, &expected](Buffer totalFile) {
                REQUIRE((expected.size() == totalFile.size()));
                REQUIRE((expected == totalFile));
                delete keepAlive;
            }, [](ErrorCode er) {
                std::cout << er.what() << std::endl;
                FAIL("Read operation should not fail on append buffer.");
            });
        }

        AND_WHEN("The content is saved with version 1") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
            ta.saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive,&expected](){
                auto buf = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");


                REQUIRE((expected == buf));
                delete keepAlive;
            }, [](const ErrorCode&){ FAIL("The transactionappendbuffer should be able to write down the data."); });
        }

        AND_WHEN("The content is saved with version 80") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
            ta.saveAllContents("/prova/prova/provaOverwrite.txt",  [keepAlive,&expected](){
                auto buf = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");


                REQUIRE((expected == buf));
                delete keepAlive;
            }, [](const ErrorCode&){ FAIL("The transactionappendbuffer should be able to write down the data."); });
        }

        AND_WHEN("The content is cleared") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
            ta.clear([&](){
                ta.size([&ta, &b, keepAlive](size_t sz) {
                    REQUIRE((sz == 0));
                    ta.saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive, &b]() {
                        auto buf = mockFilesystem.readFile(
                                "/prova/prova/provaOverwrite.txt");
                        Buffer expected{};
                        REQUIRE((expected == buf));
                        delete keepAlive;
                    }, [](const ErrorCode& te){ FAIL("The transactionappendbuffer should be able to avoid to write down the data :D"); });
                });
            });
        }

        AND_WHEN("We create a chunked reader with chunk size dividing swap size") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
            size_t chunkSize = 0;
            chunkSize = 4096;
            expectedReads = (expected.size() + chunkSize/2)/chunkSize;
            auto shd = new sharedinfo(shrd);

            shrd->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [&shrd, keepAlive, &numReads, &expectedReads, &expected](std::shared_ptr<ChunkedFstreamInterface> chk){

                auto expectedBegin = expected.begin();
                auto expectedEnd = expected.end();
                ChunkVerifier *verifier = nullptr;
                verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; delete keepAlive;},  expectedBegin, expectedEnd, ioService);
                chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
            }, [](const ErrorCode&){ FAIL("Make chunked reader should not fail");});

        }


        AND_WHEN("We create a chunked reader with chunk size not dividing swap size") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
            size_t chunkSize = 0;
            chunkSize = 17*13*19;
            expectedReads = (expected.size() + chunkSize/2)/chunkSize;
            auto shd = new sharedinfo(shrd);

            shrd->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [&shrd, keepAlive, &numReads, &expectedReads, &expected](std::shared_ptr<ChunkedFstreamInterface> chk){

                auto expectedBegin = expected.begin();
                auto expectedEnd = expected.end();
                ChunkVerifier *verifier = nullptr;
                verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; delete keepAlive;},  expectedBegin, expectedEnd, ioService);
                chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
            }, [](const ErrorCode&){ FAIL("Make chunked reader should not fail");});
        }

        }, [](const ErrorCode&){ FAIL("Append should not fail.");});
        ioService.run();
    }

    GIVEN("A buffer whose size is multiple of the swapping limit of the buffer") {
        int numReads, expectedReads;
        numReads = expectedReads = 0;
        size_t appendSize = SwappingBufferAppend::maxBufferSize * 10;
        for(size_t i = 0; i < appendSize; i++) {
            appendContent.push_back((uint8_t) (i%256) );
        }
        auto shrd = SwappingBufferOverwrite::make_shared(ioService, mockFilesystem, rootDir);
        auto &ta = *shrd;
        try {
            auto a = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");
            REQUIRE((a == Buffer{'c', 'o', 'n', 't', 'e', 'n', 'u', 't', 'o'}));

        } catch (const ErrorCode &ec) {
            std::cout << ec.what() << std::endl;
        }
        shrd->append(appendContent, [&](uint32_t){
            Buffer &expected = appendContent;

            WHEN("The content is retrieved with a readAll, it is correct") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                ta.readAll([keepAlive, &b, &appendContent, &expected](Buffer totalFile) {

                    REQUIRE((expected.size() == totalFile.size()));
                    REQUIRE((expected == totalFile));
                    delete keepAlive;
                }, [](const ErrorCode& er) {
                    std::cout << er.what() << std::endl;
                    FAIL("Read operation should not fail on append buffer.");
                });
            }

            AND_WHEN("The content is saved with version 1") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                ta.saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive,&expected](){
                    auto buf = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");


                    REQUIRE((expected == buf));
                    delete keepAlive;
                }, [](const ErrorCode&){ FAIL("The transactionappendbuffer should be able to write down the data."); });
            }

            AND_WHEN("The content is saved with version 80") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                ta.saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive,&expected](){
                    auto buf = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");


                    REQUIRE((expected == buf));
                    delete keepAlive;
                }, [](const ErrorCode&){ FAIL("The transactionappendbuffer should be able to write down the data."); });
            }

            AND_WHEN("The content is cleared") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                ta.clear([&]() {
                    //need to post otherwise we won't be sure that it will not be executed before clear.
                    ta.size([&ta, &b, keepAlive](size_t size) {
                        REQUIRE((size == 0));
                        ta.saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive, &b]() {
                            auto buf = mockFilesystem.readFile(
                                    "/prova/prova/provaOverwrite.txt");
                            Buffer expected{};
                            REQUIRE((expected == buf));
                            delete keepAlive;
                        }, [](const ErrorCode& te) {
                            FAIL("The transactionappendbuffer should be able to avoid to write down the data :D");
                        });
                    });
                });
            }

            AND_WHEN("We create a chunked reader with chunk size dividing swap size") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                size_t chunkSize = 0;
                chunkSize = 4096;
                expectedReads = (expected.size() + chunkSize/2)/chunkSize;
                auto shd = new sharedinfo(shrd);

                ta.make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [&shrd, keepAlive, &numReads, &expectedReads, &expected](std::shared_ptr<ChunkedFstreamInterface> chk){

                    auto expectedBegin = expected.begin();
                    auto expectedEnd = expected.end();
                    ChunkVerifier *verifier = nullptr;
                    verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; delete keepAlive;},  expectedBegin, expectedEnd, ioService);
                    chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
                }, [](const ErrorCode&){ FAIL("Make chunked reader should not fail.");});
            }


            AND_WHEN("We create a chunked reader with chunk size not dividing swap size") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                size_t chunkSize = 0;
                chunkSize = 17*13*19;
                expectedReads = (expected.size() + chunkSize/2)/chunkSize;
                auto shd = new sharedinfo(shrd);

                ta.make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [&shrd, keepAlive, &numReads, &expectedReads, &expected](std::shared_ptr<ChunkedFstreamInterface> chk){

                    auto expectedBegin = expected.begin();
                    auto expectedEnd = expected.end();
                    ChunkVerifier *verifier = nullptr;
                    verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; delete keepAlive;},  expectedBegin, expectedEnd, ioService);
                    chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
                }, [](const ErrorCode&){ FAIL("Make chunked reader should not fail");});
            }

        }, [](const ErrorCode&){ FAIL("Append should not fail");});

        ioService.run();
    }

    GIVEN("A very very large overwrite buffer") {
        int numReads = 0, expectedReads = 0;
        size_t appendSize = SwappingBufferAppend::maxBufferSize * 256 + 13; //slightly greather :D
        appendContent.reserve(appendSize);
        for(size_t i = 0; i < appendSize; i++) {
            appendContent.push_back('a');
        }
        auto shrd = SwappingBufferOverwrite::make_shared(ioService, mockFilesystem, rootDir);
        auto &ta = *shrd;
        try {
            auto a = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");
            REQUIRE((a == Buffer{'c', 'o', 'n', 't', 'e', 'n', 'u', 't', 'o'}));

        } catch (const ErrorCode &ec) {
            std::cout << ec.what() << std::endl;
        }
        shrd->append(appendContent, [&](size_t size){

            Buffer &expected = appendContent;


            WHEN("The content is retrieved with a readAll, it is correct") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                ta.readAll([keepAlive, &b, &appendContent, &expected](Buffer totalFile) {

                    REQUIRE((expected.size() == totalFile.size()));
                    REQUIRE((expected == totalFile));
                    delete keepAlive;
                }, [](const ErrorCode& er) {
                    std::cout << er.what() << std::endl;
                    FAIL("Read operation should not fail on append buffer.");
                });
            }

            AND_WHEN("The content is saved with version 1") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                ta.saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive,&expected](){
                    auto buf = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");


                    REQUIRE((expected == buf));
                    delete keepAlive;
                }, [](const ErrorCode&){ FAIL("The transactionappendbuffer should be able to write down the data."); });
            }

            AND_WHEN("The content is saved with version 80") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                ta.saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive,&expected](){
                    auto buf = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");


                    REQUIRE((expected == buf));
                    delete keepAlive;
                }, [](const ErrorCode&){ FAIL("The transactionappendbuffer should be able to write down the data."); });
            }

            AND_WHEN("The content is cleared") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                ta.clear([&]() {
                    //need to post otherwise we won't be sure that it will not be executed before clear.
                    ta.size([&ta, &b, keepAlive](size_t size) {
                        REQUIRE((size == 0));
                        ta.saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive, &b]() {
                            auto buf = mockFilesystem.readFile(
                                    "/prova/prova/provaOverwrite.txt");
                            Buffer expected{};
                            REQUIRE((expected == buf));
                            delete keepAlive;
                        }, [](const ErrorCode& te) {
                            FAIL("The transactionappendbuffer should be able to avoid to write down the data :D");
                        });
                    });
                });
            }


            int numReads = 0;
            int expectedReads = 0;

            AND_WHEN("We create a chunked reader with chunk size dividing swap size") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                size_t chunkSize = 4096;
                expectedReads = (expected.size() + chunkSize/2)/chunkSize;
                auto shd = new sharedinfo(shrd);

                ta.make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [&ta, keepAlive, &numReads, &expectedReads, &expected](std::shared_ptr<ChunkedFstreamInterface> chk){

                    auto expectedBegin = expected.begin();
                    auto expectedEnd = expected.end();
                    ChunkVerifier *verifier = nullptr;
                    verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; delete keepAlive;},  expectedBegin, expectedEnd, ioService);
                    chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
                }, [](const ErrorCode&){ FAIL("chunked reader creation");});
            }


            AND_WHEN("We create a chunked reader with chunk size not dividing swap size") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                size_t chunkSize = 0;
                chunkSize = 17*13*19;
                expectedReads = (expected.size() + chunkSize/2)/chunkSize;
                auto shd = new sharedinfo(shrd);

                ta.make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [&ta, keepAlive, &numReads, &expectedReads, &expected](std::shared_ptr<ChunkedFstreamInterface> chk) {

                    auto expectedBegin = expected.begin();
                    auto expectedEnd = expected.end();
                    ChunkVerifier *verifier = nullptr;
                    verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier]() {
                        delete verifier;
                        delete keepAlive;
                    }, expectedBegin, expectedEnd, ioService);
                    chk->next_chunk([verifier](const ErrorCode& ec, Buffer data) { verifier->readHandler(ec, data); });
                }, [](const ErrorCode&){ FAIL("Should not fail while creating");});
            }
        }, [](const ErrorCode&){ FAIL("Append should not fail "); });
        ioService.run();
    }

}


TEST_CASE("Overwrite Buffer on file that does not exist", "[swapping_buffer][tbo][tbo2]"){

    ioService.reset();
    mockFilesystem.clear();

    Buffer appendContent;

    GIVEN("An append of size less than the swapping treshold") {
        for (auto i = 0; i < 139812; i++) { appendContent.push_back((uint8_t) (i % 256)); } //initialize buffer
        auto shrd = SwappingBufferOverwrite::make_shared(ioService, mockFilesystem, rootDir);
        int numReads = 0;
        int expectedReads = 0;
        shrd->append(appendContent, [&](uint32_t){
        Buffer &expected = appendContent;


            WHEN("A readall is performed the content is correct") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                shrd->readAll([keepAlive, &appendContent, &expected](Buffer totalFile) {
                    REQUIRE((expected.size() == totalFile.size()));
                    REQUIRE((expected == totalFile));
                    delete keepAlive;
                }, [](const ErrorCode& er) {
                    std::cout << er.what() << std::endl;
                    FAIL("Read operation should not fail on overwrite buffer.");
                });
            }

            AND_WHEN("The content is saved with version 1") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                shrd->saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive,&expected](){
                    auto buf = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");


                    REQUIRE((expected == buf));
                    delete keepAlive;
                }, [](const ErrorCode&){ FAIL("The transactionappendbuffer should be able to write down the data."); });

            }

            AND_WHEN("The content is saved with version 80") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                shrd->saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive,&expected](){
                    auto buf = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");


                    REQUIRE((expected == buf));
                    delete keepAlive;
                }, [](const ErrorCode&){ FAIL("The transactionappendbuffer should be able to write down the data."); });
            }

            AND_WHEN("The content is cleared") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                shrd->clear([keepAlive, shrd](){
                    //need to post otherwise we won't be sure that it will not be executed before clear.
                    shrd->size([shrd, keepAlive](size_t sz) {
                        REQUIRE((sz == 0));
                        shrd->saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive]() {
                            auto buf = mockFilesystem.readFile(
                                    "/prova/prova/provaOverwrite.txt");
                            Buffer expected{};
                            REQUIRE((expected == buf));
                            delete keepAlive;
                        }, [](const ErrorCode& te){ FAIL("The transactionappendbuffer should be able to avoid to write down the data :D"); });
                    });
                });
            }


            AND_WHEN("We create a chunked reader with chunk size dividing swap size") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                size_t chunkSize = 0;
                chunkSize = 4096;
                expectedReads = (expected.size() + chunkSize/2)/chunkSize;
                auto shd = new sharedinfo(shrd);

                shrd->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [keepAlive, &numReads, &expectedReads, &expected](std::shared_ptr<ChunkedFstreamInterface> chk){

                    auto expectedBegin = expected.begin();
                    auto expectedEnd = expected.end();
                    ChunkVerifier *verifier = nullptr;
                    verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; delete keepAlive;},  expectedBegin, expectedEnd, ioService);
                    chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
                }, [&](ErrorCode){ FAIL("Faialed to make chunked reader");});
            }


            AND_WHEN("We create a chunked reader with chunk size not dividing swap size") {
                auto keepAlive = new boost::asio::io_service::work(ioService);
                size_t chunkSize = 0;
                chunkSize = 17*13*19;
                expectedReads = (expected.size() + chunkSize/2)/chunkSize;
                auto shd = new sharedinfo(shrd);

                shrd->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [keepAlive, &numReads, &expectedReads, &expected](std::shared_ptr<ChunkedFstreamInterface> chk){

                    auto expectedBegin = expected.begin();
                    auto expectedEnd = expected.end();
                    ChunkVerifier *verifier = nullptr;
                    verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; delete keepAlive;},  expectedBegin, expectedEnd, ioService);
                    chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
                }, [&](ErrorCode){FAIL("Failure to make chunked reader");});
            }

        }, [](const ErrorCode&){ FAIL("Should not fail appending");});
        ioService.run();
    }


    GIVEN("A buffer whose size is slightly greather than the swapping limit of the buffer") {
        size_t appendSize = SwappingBufferAppend::maxBufferSize * 1.1; //slightly greather :D
        for(size_t i = 0; i < appendSize; i++) {
            appendContent.push_back((uint8_t) (i%256) );
        }
        auto shrd = SwappingBufferOverwrite::make_shared(ioService, mockFilesystem, rootDir);
//        auto &ta = *shrd;
        shrd->append(appendContent, [&, shrd](uint32_t){
        Buffer &expected = appendContent;

        WHEN("The content is retrieved with a readAll, it is correct") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
            shrd->readAll([keepAlive, &appendContent, &expected](Buffer totalFile) {

                REQUIRE((expected.size() == totalFile.size()));
                REQUIRE((expected == totalFile));
                delete keepAlive;
            }, [](const ErrorCode& er) {
                std::cout << er.what() << std::endl;
                FAIL("Read operation should not fail on append buffer.");
            });
        }

        AND_WHEN("The content is saved with version 1") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
            shrd->saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive,&expected](){
                auto buf = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");


                REQUIRE((expected == buf));
                delete keepAlive;
            }, [](const ErrorCode&){ FAIL("The transactionappendbuffer should be able to write down the data."); });
        }

        AND_WHEN("The content is saved with version 80") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
            shrd->saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive,&expected](){
                auto buf = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");


                REQUIRE((expected == buf));
                delete keepAlive;
            }, [](const ErrorCode&){ FAIL("The transactionappendbuffer should be able to write down the data."); });
        }

        AND_WHEN("The content is cleared") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
            shrd->clear([shrd, keepAlive](){
                //need to post otherwise we won't be sure that it will not be executed before clear.
                shrd->size([shrd, keepAlive](size_t size) {
                    REQUIRE((size == 0));
                    shrd->saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive]() {
                        auto buf = mockFilesystem.readFile(
                                "/prova/prova/provaOverwrite.txt");
                        Buffer expected{};
                        REQUIRE((expected == buf));
                        delete keepAlive;
                    }, [](const ErrorCode& te){ FAIL("The transactionappendbuffer should be able to avoid to write down the data :D"); });
                });
            });
        }

        AND_WHEN("We create a chunked reader with chunk size dividing swap size") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
            size_t chunkSize = 0;
            chunkSize = 4096;
            expectedReads = (expected.size() + chunkSize/2)/chunkSize;
            auto shd = new sharedinfo(shrd);

            shrd->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [keepAlive, &expected](std::shared_ptr<ChunkedFstreamInterface> chk){

                auto expectedBegin = expected.begin();
                auto expectedEnd = expected.end();
                ChunkVerifier *verifier = nullptr;
                verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; delete keepAlive;},  expectedBegin, expectedEnd, ioService);
                chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
            }, [](const ErrorCode&){ FAIL("Chunked reader should not fail");});
        }


        AND_WHEN("We create a chunked reader with chunk size not dividing swap size") {
            size_t chunkSize = 0;
            chunkSize = 17*13*19;
            expectedReads = (expected.size() + chunkSize/2)/chunkSize;
            auto shd = new sharedinfo(shrd);

            auto keepAlive = new boost::asio::io_service::work(ioService);
            shrd->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [keepAlive, &expected](std::shared_ptr<ChunkedFstreamInterface> chk){

                auto expectedBegin = expected.begin();
                auto expectedEnd = expected.end();
                ChunkVerifier *verifier = nullptr;
                verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier](){ delete verifier; delete keepAlive;},  expectedBegin, expectedEnd, ioService);
                chk->next_chunk([verifier](const ErrorCode& ec, Buffer data){ verifier->readHandler(ec, data);});
            }, [](ErrorCode){
                FAIL("Make chunked reader should not fail");
            });
        }
        }, [](const ErrorCode&){ FAIL("Append should not fail");});

        ioService.run();
    }



    GIVEN("A buffer whose size is multiple of the swapping limit of the buffer") {
        size_t appendSize = SwappingBufferAppend::maxBufferSize * 10;
        for(size_t i = 0; i < appendSize; i++) {
            appendContent.push_back((uint8_t) (i%256) );
        }
        int numReads = 0;
        int expectedReads = 0;

        auto shrd = SwappingBufferOverwrite::make_shared(ioService, mockFilesystem, rootDir);
        shrd->append(appendContent, [&](uint32_t) {
            Buffer &expected = appendContent;


            WHEN("The content is retrieved with a readAll, it is correct") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
                shrd->readAll([keepAlive, &expected](Buffer totalFile) {

                    REQUIRE((expected.size() == totalFile.size()));
                    REQUIRE((expected == totalFile));
                    delete keepAlive;
                }, [](const ErrorCode& er) {
                    std::cout << er.what() << std::endl;
                    FAIL("Read operation should not fail on append buffer.");
                });
            }

            AND_WHEN("The content is saved with version 1") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
                shrd->saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive, &expected]() {
                    auto buf = mockFilesystem.readFile(
                            "/prova/prova/provaOverwrite.txt");


                    REQUIRE((expected == buf));
                    delete keepAlive;
                }, [](ErrorCode) { FAIL("The transactionappendbuffer should be able to write down the data."); });
            }

            AND_WHEN("The content is saved with version 80") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
                shrd->saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive, &expected]() {
                    auto buf = mockFilesystem.readFile(
                            "/prova/prova/provaOverwrite.txt");


                    REQUIRE((expected == buf));
                    delete keepAlive;
                }, [](ErrorCode) { FAIL("The transactionappendbuffer should be able to write down the data."); });
            }

            AND_WHEN("The content is cleared") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
                shrd->clear([&shrd, keepAlive](){
                //need to post otherwise we won't be sure that it will not be executed before clear.
                shrd->size([&shrd, keepAlive](size_t size) {
                    REQUIRE((size == 0));
                    shrd->saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive]() {
                        auto buf = mockFilesystem.readFile(
                                "/prova/prova/provaOverwrite.txt");
                        Buffer expected{};
                        REQUIRE((expected == buf));
                        delete keepAlive;
                    }, [](const ErrorCode& te) {
                        FAIL("The transactionappendbuffer should be able to avoid to write down the data :D");
                    });
                });
                });
            }


            AND_WHEN("We create a chunked reader with chunk size dividing swap size") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
                size_t chunkSize = 0;
                chunkSize = 4096;
                expectedReads = (expected.size() + chunkSize / 2) / chunkSize;
                auto shd = new sharedinfo(shrd);

                shrd->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [&shrd, keepAlive, &numReads, &expectedReads, &expected](std::shared_ptr<ChunkedFstreamInterface> chk){

                    auto expectedBegin = expected.begin();
                    auto expectedEnd = expected.end();
                    ChunkVerifier *verifier = nullptr;
                    verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier]() {
                        delete verifier;
                        delete keepAlive;
                    }, expectedBegin, expectedEnd, ioService);
                    chk->next_chunk([verifier](const ErrorCode& ec, Buffer data) { verifier->readHandler(ec, data); });
                }, [](const ErrorCode&){ FAIL("Should not fail while making chunked reader");});
            }


            AND_WHEN("We create a chunked reader with chunk size not dividing swap size") {
            auto keepAlive = new boost::asio::io_service::work(ioService);
                size_t chunkSize = 0;
                chunkSize = 17 * 13 * 19;
                expectedReads = (expected.size() + chunkSize / 2) / chunkSize;
                auto shd = new sharedinfo(shrd);

                shrd->make_chunked_stream(std::shared_ptr<sharedinfo>(shd), chunkSize, [&shrd, keepAlive, &numReads, &expectedReads, &expected](std::shared_ptr<ChunkedFstreamInterface> chk){

                auto expectedBegin = expected.begin();
                auto expectedEnd = expected.end();
                ChunkVerifier *verifier = nullptr;
                verifier = new ChunkVerifier(numReads, expectedReads, chk, [keepAlive, verifier]() {
                    delete verifier;
                    delete keepAlive;
                }, expectedBegin, expectedEnd, ioService);
                chk->next_chunk([verifier](const ErrorCode& ec, Buffer data) { verifier->readHandler(ec, data); });
                }, [](const ErrorCode&){ FAIL("Should not fail while making chunked reader");});
            }
        }, [&](ErrorCode){ FAIL("Should not fail while appending"); });
        ioService.run();
    }


    GIVEN("A very very large append buffer, added chunk by chunk. ") {
        int numReads = 0;
        int expectedReads = 0;
        size_t appendSize = SwappingBufferAppend::maxBufferSize * 64 + 13; //slightly greather :D
        appendContent.reserve(appendSize);
        for(size_t i = 0; i < SwappingBufferOverwrite::maxBufferSize; ++i) {
            appendContent.push_back((uint8_t) (i % 256));
        }
        auto shrd = SwappingBufferOverwrite::make_shared(ioService, mockFilesystem, rootDir);

        auto testLambda = [shrd, appendSize, &appendContent](size_t size) {
                while(appendContent.size() < appendSize/2) {
                    appendContent.insert(appendContent.end(), appendContent.begin(), appendContent.end());
                }
                for(auto i = appendContent.size(); i < appendSize; ++i) {
                    appendContent.push_back((uint8_t) i % 256);
                }
            Buffer &expected = appendContent;
            auto keepAlive = new boost::asio::io_service::work(ioService);

            WHEN("The content is retrieved with a readAll, it is correct") {
                shrd->readAll([keepAlive, &expected](Buffer totalFile) {
                    REQUIRE((expected.size() == totalFile.size()));
                    REQUIRE((expected == totalFile));
                    delete keepAlive;
                }, [](const ErrorCode& er) {
                    std::cout << er.what() << std::endl;
                    FAIL("Read operation should not fail on append buffer.");
                });
            }

            AND_WHEN("The content is saved with version 1") {
                shrd->saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive,&expected](){
                    auto buf = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");


                    REQUIRE((expected == buf));
                    delete keepAlive;
                }, [](const ErrorCode&){ FAIL("The transactionappendbuffer should be able to write down the data."); });
            }

            AND_WHEN("The content is saved with version 80") {
                shrd->saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive,&expected](){
                    auto buf = mockFilesystem.readFile("/prova/prova/provaOverwrite.txt");


                    REQUIRE((expected == buf));
                    delete keepAlive;
                }, [](const ErrorCode&){ FAIL("The transactionappendbuffer should be able to write down the data."); });
            }
            AND_WHEN("The content is cleared") {
                shrd->clear([shrd, keepAlive](){
                    //need to post otherwise we won't be sure that it will not be executed before clear.
                    shrd->size([shrd, keepAlive](size_t size) {
                        REQUIRE(size == 0);
                        std::cout << "wtf are you reading??" << std::endl;
                        shrd->saveAllContents("/prova/prova/provaOverwrite.txt", [keepAlive]() {
                            std::cout << "i have written all contents... maybe they are too much?" << std::endl;
                            auto buf = mockFilesystem.readFile(
                                    "/prova/prova/provaOverwrite.txt");
                            std::cout << "buffer size is" << buf.size() << std::endl;
                            Buffer expected{};
                            REQUIRE( (expected == buf) );
                            delete keepAlive;
                        }, [](const ErrorCode& te){ FAIL("The transactionappendbuffer should be able to avoid to write down the data :D"); });
                    });
                });
            }
        };
        std::function<void(size_t size)> appendFunction;
        appendFunction = [&](size_t size){
            if(size >= appendSize-13) { //reached end!
                Buffer b{0, 1, 2, 3, 4, 5, 6,7, 8, 9, 10, 11, 12};
                shrd->append(b, testLambda, [](uint32_t){});
            } else {
                shrd->append(appendContent, appendFunction, [](const ErrorCode& te){});
            }
        };

        shrd->append(appendContent, appendFunction, [](ErrorCode){}); //appendi ogni volta
        ioService.run();
    }

}
