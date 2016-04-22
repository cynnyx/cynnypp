#include <bloom_filters/bloom_filters.hpp>
#include <map>
#include "catch.hpp"
using namespace cynny::cynnypp::bloom_filters;
TEST_CASE("Creation of an empty bloom filter", "[bloom_filter][bf]") {

    GIVEN("A bloom filter representing a set of scalar types") {
        BloomFilter<1024, 2, int> bf(std::hash<int> { });
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
            int setLength = 200;
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
            AND_THEN("A reinsertion of the set does not change the serialization") {
                auto serialization = bf.serialize();
                for(auto &v : mySet) {
                    bf.set(v.first);
                }
                REQUIRE((bf.serialize() == serialization));
            }
            AND_THEN("The serialization creates an identical set"){
                auto serialization = bf.serialize();
                auto bf2 = BloomFilter<1024,2,int>(std::hash<int>{}, serialization);
                for (auto &v : mySet) {
                    REQUIRE(bf2.has(v.first));
                }
            }

            AND_THEN("A copy creates an identical set"){
                BloomFilter<1024, 2, int> bf2 = bf;
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


    GIVEN("A bloom filter representing a set of object types") {


        BloomFilter<1024, 2, std::vector<bool> > bf(std::hash<std::vector<bool>> { });
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
            AND_THEN("A reinsertion of the set does not change the serialization") {
                auto serialization = bf.serialize();
                for(auto &v : mySet) {
                    bf.set(v.first);
                }
                REQUIRE((bf.serialize() == serialization));
            }
            AND_THEN("The serialization creates an identical set"){
                auto serialization = bf.serialize();
                auto bf2 = BloomFilter<1024,2,std::vector<bool>>(std::hash<std::vector<bool>>{}, serialization);
                for (auto &v : mySet) {
                    REQUIRE(bf2.has(v.first));
                }
            }

            AND_THEN("A copy creates an identical set"){
                BloomFilter<1024, 2, std::vector<bool>> bf2 = bf;
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