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
        }

    }
}