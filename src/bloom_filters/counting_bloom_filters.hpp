#ifndef CYNNYPP_COUNTING_BLOOM_FILTERS_H
#define CYNNYPP_COUNTING_BLOOM_FILTERS_H
#include <functional>
#include <array>
#include <initializer_list>
#include <stdexcept>
#include <iostream>
#include <cassert>

#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")

namespace cynny {
namespace cynnypp {
namespace bloom_filters {
template<size_t m, uint8_t k, uint8_t bits_per_element, class ElementType>
class CountingBloomFilter {
public:
	static_assert(m > 0, "Number of bits used cannot be 0");
	static_assert(k > 0, "Number of hash functions used cannot be 0");
	static_assert(bits_per_element == 2 || bits_per_element == 4 || bits_per_element == 8, "Number of bits per element must be a power of 2.");

    using local_array = std::array<uint8_t, (m*bits_per_element+7)/8>;
	/** \brief Creates a new counting bloom filter giving explicitly a list of std::function to use as hashes, through an initialzier list
	 * \param hashes the hash functions passed in input (initializer list)
	 * \param bf_serialized a starting bloom filter (default: empty)
	 *
	 */
	CountingBloomFilter(std::initializer_list<std::function<size_t(const ElementType &)>> hashes) : _memory{ new std::array<uint8_t, (m*bits_per_element+7)/8>()}, bf(*_memory){
		addHashes(hashes);
	};


    CountingBloomFilter(std::initializer_list<std::function<size_t(const ElementType &)>> hashes, std::array<uint8_t, (m*bits_per_element+7)/8> other)  : _memory{ new std::array<uint8_t, (m*bits_per_element+7)/8>()}, bf(*_memory) {
        std::copy(other.begin(), other.end(), bf.begin());
        addHashes(hashes);
    }

	template<typename T>
	void set(T&& element) {
		static_assert(std::is_convertible<T, ElementType>::value, "Invalid type.");
		for(auto &current_hash: hash_functions) {
			auto val = current_hash(std::forward<T>(element))%m;
			auto bucket = (val*bits_per_element)/8;
			auto part = val%(8/bits_per_element);
			bf[bucket] += (1 << (part * bits_per_element));
		}
	}

	template<typename T>
	bool has(T&& element) {
		static_assert(std::is_convertible<T, ElementType>::value, "Invalid type.");
		for(auto &current_hash : hash_functions) {
			auto val = current_hash(std::forward<T>(element))%m;
			auto bucket = (val*bits_per_element)/8;
			auto part = val%(8/bits_per_element);
			if(((bf[bucket] >> (part * bits_per_element)) & 0xF) == 0) return false;
		}
		return true;
	}

	template<typename T>
	void remove(T&& element) {
		static_assert(std::is_convertible<T, ElementType>::value, "Invalid type.");
		for(auto &current_hash: hash_functions) {
			auto val = current_hash(std::forward<T>(element))%m;
			auto bucket = (val*bits_per_element)/8;
			auto part = val%(8/bits_per_element);
			if(((bf[bucket] >> (part * bits_per_element)) & 0xF) > 0) bf[bucket] -= (1 << (part * bits_per_element));
		}
	}



	template<typename T>
	int count(T &&element) {
		static_assert(std::is_convertible<T, ElementType>::value, "Invalid type.");
		int count = 15;
		for(auto &current_hash : hash_functions) {
			auto val = current_hash(std::forward<T>(element))%m;
			auto bucket = (val*bits_per_element)/8;
			uint8_t part = val%(8/bits_per_element);
			uint8_t bucket_count = (bf[bucket] >> (part * bits_per_element)) &0xF;
			assert(bucket_count <= 15);
			count = (count < bucket_count) ? count : bucket_count;
		}

		return count;
	}


    CountingBloomFilter(const CountingBloomFilter &obj) : _memory{new local_array}, bf(*_memory)
    {
        std::copy(obj.bf.begin(), obj.bf.end(), bf.begin());
		hash_functions = obj.hash_functions;
	}



	/** Move constructor */
	CountingBloomFilter(CountingBloomFilter && obj) : bf(obj.bf) {
		swap(*this, obj);
	}

	/** Assignment operator */
	CountingBloomFilter & operator=(CountingBloomFilter r) {
		swap(*this, r);
		return *this;
	}

	/** Swap function, used to implemente copy and swap idiom */
	friend void swap(CountingBloomFilter &first, CountingBloomFilter &second) {
		using std::swap;
		swap(first.bf, second.bf);
		swap(first.hash_functions, second.hash_functions);
	}

	/** \brief union assignment operator
     * \param other the counting  bloom filter with which we want to perform the union
     *
     */
	CountingBloomFilter& operator+=(const CountingBloomFilter &other) {
		for(unsigned int i = 0; i < sizeof(bf); ++i) {
			//operate on bytes
			uint8_t tmpbyte = 0;
			for(int part = 0; part < 8/bits_per_element; ++part) {
				uint8_t tcount = (this->bf[i] >> (bits_per_element * part)) & 0xF;
				uint8_t ocount = (other.bf[i] >> (bits_per_element * part)) & 0xF;
				tmpbyte += ((tcount + ocount) & 0xF) << (bits_per_element * part);
			}
			this->bf[i] = tmpbyte;
		}
		return *this;
	}

	/** \brief union operator
	 *  \param other the counting bloom filter with which we want to perform the union.
	 */
	const CountingBloomFilter operator+(const CountingBloomFilter &other) const{
		return CoutingBloomFilter(*this) += other;
	}


	/** \brief returns a serialized representation of the counting bloom filter.
	 *
	 * \return serialization of the bloom filter.
	 */
	std::array<uint8_t, (m*bits_per_element+7)/8> serialize() {
		return bf;
	}


	~CountingBloomFilter() = default;

private:

	CountingBloomFilter() = default;

	inline void addHashes(std::initializer_list<std::function<size_t(const ElementType &)>> l) {
		if(l.size() != k && l.size() != 1) throw std::invalid_argument("The number of hash function passed to the constructor is not the one required.");
		if(l.size() == 1) return generateHashes(*l.begin());
		int i = 0;
		for(auto &cur : l) {
			hash_functions[i++] = cur;
		}
	}


	inline void generateHashes(std::function<size_t(const ElementType &)> h, size_t seed = 468486423) {
		std::srand(seed);
		//generate k hash function
		for(uint8_t i = 0; i < k; ++i) {
			size_t val = std::rand();
			hash_functions[i] = [val, h](const ElementType &element){
				return (h(element) ^ val); //xor function
			};
		}
	}

	std::array<uint8_t, (m*bits_per_element+7)/8> *_memory;
	//reference to array taking the count of the stuff.
	std::array<uint8_t, (m*bits_per_element+7)/8> &bf;

	//array of hash functions.
	std::array<std::function<size_t(const ElementType &)>, k> hash_functions;

};

}
}
}



#endif //CYNNYPP_COUNTING_BLOOM_FILTERS_H
