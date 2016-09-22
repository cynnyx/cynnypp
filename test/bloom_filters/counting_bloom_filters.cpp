#include <bloom_filters/counting_bloom_filters.hpp>
#include <map>
#include "catch.hpp"
using namespace cynny::cynnypp::bloom_filters;
TEST_CASE("Creation of an empty counting bloom filter", "[counting_bloom_filter][cbf]") {

    GIVEN("A bloom filter representing a set of scalar types") {
        CountingBloomFilter<1024, 2, 4, int> bf({std::hash<int> { }});
        WHEN("The bloom filter has no elements") {
            THEN("its serialization corresponds to the empty lisit") {
                for (auto &i: bf.serialize()) {
                    REQUIRE(i == 0);
                }
            }
        }


        WHEN("It contains a single element") {
            bf.set(1000);
            THEN("It is the only one represented //since we have chosen wisely") {
                bool has = false;
                for (int i = 0; i < 100; ++i) has |= bf.has(i);
                REQUIRE(!has);
            }
        }

        WHEN("It represents a set") {
            std::map<int, int> mySet;
            int setLength = 2;
            while (setLength--) {
                int val = std::rand();
                mySet.insert({val, val});
                bf.set(val);
            }
            THEN("There are no false negatives") {
                for (auto &v : mySet) {
                    REQUIRE(bf.has(v.first));
                }
            }
            AND_THEN("A reinsertion in the set causes an increment") {
                CountingBloomFilter<1024,2,4, int> old_bf = bf;
                for(auto &v : mySet) {
                    bf.set(v.first);
                }

                for(auto &v : mySet) {
                    REQUIRE(bf.count(v.first) > old_bf.count(v.first));
                }
            }
            AND_THEN("The serialization creates an identical set"){
                auto serialization = bf.serialize();
                auto bf2 = CountingBloomFilter<1024,2,4, int>({std::hash<int>{}}, serialization);
                for (auto &v : mySet) {
                    REQUIRE(bf2.has(v.first));
                }
            }

            AND_THEN("A copy creates an identical set"){
                CountingBloomFilter<1024, 2, 4, int> bf2 = bf;
                for (auto &v : mySet) {
                    REQUIRE(bf2.has(v.first));
                }
            }

            AND_THEN("A copy constructor creates an identical set"){
                auto bf2(bf);
                for (auto &v : mySet) {
                    REQUIRE(bf2.has(v.first));
                }
            }

            AND_THEN("The move constructor has the right semantics") {
                auto bf2 = std::move(bf);
                for (auto &v : mySet) {
                    REQUIRE(bf2.has(v.first));
                }
            }

            AND_THEN("The move constructor has the right semantics") {
                auto bf2(std::move(bf));
                for (auto &v : mySet) {
                    REQUIRE(bf2.has(v.first));
                }

            }
        }

        WHEN("An union is performed with another set") {
            std::map<int, int> mySet1;
            std::map<int, int> mySet2;
            CountingBloomFilter<1024, 2, 4, int> bf2({std::hash<int>{}});
            int setLength = 200;
            while (setLength--) {
                int val = std::rand();
                mySet1.insert({val, val});
                bf.set(val);
                val = std::rand();
                mySet2.insert({val, val});
                bf2.set(val);
            }
            bf += bf2;
            THEN("The union works correctly.") {
                for(auto &v : mySet1) {
                    REQUIRE(bf.has(v.first));
                }
                for(auto &v : mySet2) {
                    REQUIRE(bf2.has(v.first));
                }
            }

        }

    }


    GIVEN("A counting bloom filter which gets modified dynamically") {
        CountingBloomFilter<1024, 2, 4, int> bf{{std::hash<int>()}};
        WHEN("An element is inserted and then removed") {
            bf.set(546);
            REQUIRE(bf.has(546));
            REQUIRE(bf.count(546));
            auto size = bf.count(546);
            bf.remove(546);
            REQUIRE(!bf.has(546));
            REQUIRE(bf.count(546) == 0);
        }

        AND_WHEN("Multiple elements are inserted") {
            for(int i = 0; i < 200; ++i) {
                bf.set(i);
            }

            for(int i = 0; i < 200; ++i) {
                REQUIRE(bf.count(i) == 1);
            }
            THEN("When they get removed, their count is 0") {
                for(int i = 0; i < 200; ++i) {
                    bf.remove(i);
                    REQUIRE(bf.count(i) == 0);
                }

            }
        }


    }


    GIVEN("A bloom filter representing a set of object types") {


        CountingBloomFilter<1024, 2, 4, std::vector<bool> > bf({std::hash<std::vector<bool>> { }});
        WHEN("The bloom filter has no elements") {
            THEN("its serialization corresponds to the empty lisit") {
                for (auto &i: bf.serialize()) {
                    REQUIRE(i == 0);
                }
            }
        }


        WHEN("It represents a set") {
            std::map<std::vector<bool>, int> mySet;
            int setLength = 200;
            while (setLength--) {
                std::vector<bool> val;
                for(auto i = 0; i < 100; ++i) {
                    bool v = std::rand()%2;
                    val.push_back(v);
                }
                mySet.insert({val, 0});
                bf.set(val);
            }
            THEN("There are no false negatives") {
                for (auto &v : mySet) {
                    REQUIRE(bf.has(v.first));
                }
            }

            AND_THEN("The serialization creates an identical set"){
                auto serialization = bf.serialize();
                auto bf2 = CountingBloomFilter<1024,2, 4, std::vector<bool>>({std::hash<std::vector<bool>>{}}, serialization);
                for (auto &v : mySet) {
                    REQUIRE(bf2.has(v.first));
                }
            }

            AND_THEN("A copy creates an identical set"){
                CountingBloomFilter<1024, 2, 4, std::vector<bool>> bf2 = bf;
                for (auto &v : mySet) {
                    REQUIRE(bf2.has(v.first));
                }
            }

            AND_THEN("A copy constructor creates an identical set"){
                auto bf2(bf);
                for (auto &v : mySet) {
                    REQUIRE(bf2.has(v.first));
                }
            }

            AND_THEN("The move constructor has the right semantics") {
                auto bf2 = std::move(bf);
                for (auto &v : mySet) {
                    REQUIRE(bf2.has(v.first));
                }
            }

            AND_THEN("The move constructor has the right semantics") {
                auto bf2(std::move(bf));
                for (auto &v : mySet) {
                    REQUIRE(bf2.has(v.first));
                }
            }

        }

    }
}