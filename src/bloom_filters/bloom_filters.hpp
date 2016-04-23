#include <functional>
#include <array>
#include <initializer_list>
#include <stdexcept>
#include <iostream>

namespace cynny {
namespace cynnypp {
namespace bloom_filters {



/** \brief BloomFilter class provides the minimal functionalities needed in order to represent set membership using bloom
 * filters.
 * \tparam m the number of bits of the bloom filter
 * \tparam k the number of hash functions used
 * \tparam ElementType the type of the elements that will be represented in this filter
 *
 */
template<size_t m, uint8_t k, class ElementType>
class BloomFilter {
public:

    static_assert(m > 0, "Number of bits used cannot be 0");
    static_assert(k > 0, "Number of hash functions used cannot be 0");


    /** \brief Creates a new bloom filter giving explicitly a list of std::function to use as hashes, through an initialzier list
     * \param hashes the hash functions passed in input (initializer list)
     * \param bf_serialized a starting bloom filter (default: empty)
     *
     */
    BloomFilter(std::initializer_list<std::function<size_t(const ElementType &)>> hashes, std::array<uint8_t, (m+7)/8> bf_serialized = std::array<uint8_t, (m+7)/8>{}) : bf(bf_serialized) {
            addHashes(hashes);
    }

    /** \brief Creates a new bloom filter giving only a seed hash function; the k hash functions actually used will be generated automatically
     *  \param h the seed hash function
     *  \param bf_serialized a starting bloom filter (default: empty)
     *  \param seed the initial seed for generation.
     *
     */
    BloomFilter(std::function<size_t(const ElementType &)> h, std::array<uint8_t, (m+7)/8> bf_serialized = std::array<uint8_t, (m+7)/8>{}, size_t seed = 468486423) : bf(bf_serialized){
            generateHashes(h, seed);
    }

    /** Copy constructor */
    BloomFilter(const BloomFilter &obj) {
        bf = obj.bf;
        hash_functions = obj.hash_functions;
    }



    /** Move constructor */
    BloomFilter(BloomFilter && obj) : BloomFilter() {
        swap(*this, obj);
    }

    /** Assignment operator */
    BloomFilter & operator=(BloomFilter r) {
        swap(*this, r);
        return *this;
    }

    /** Swap function, used to implemente copy and swap idiom */
    friend void swap(BloomFilter &first, BloomFilter &second) {
        using std::swap;
        swap(first.bf, second.bf);
        swap(first.hash_functions, second.hash_functions);
    }


    /** \brief Adds to the set the element passed as input parameter
     * \param element the element to add to the set representation
     */
    template<typename T>
    void set(T&& element) {
        static_assert(std::is_convertible<T, ElementType>::value, "Invalid type.");
        for(auto &current_hash: hash_functions) {
            auto val = current_hash(std::forward<T>(element))%m;
            auto bucket = val/8;
            auto bit = val%8;
            bf[bucket] |= 1 << bit;
        }
    }
    /** \brief Verifies if an element is in the set representation
     * \param element the element to check for existence
     *
     *  \return true in case it exists (with a certain error probability); false otherwise.
     */
    template<typename T>
    bool has(T&& element) {
        static_assert(std::is_convertible<T, ElementType>::value, "Invalid type.");
        for(auto &current_hash : hash_functions) {
            auto val = current_hash(std::forward<T>(element))%m;
            auto bucket = val/8;
            auto bit = val%8;
            if(!(bf[bucket] >> bit & 1)) return false;
        }
        return true;
    }
    /** \brief union assignment operator
     * \param other the bloom filter with which we want to perform the union
     *
     */
    BloomFilter& operator+=(const BloomFilter &other) {
        //perform bitwise or of the two sets.
        for(unsigned int i = 0; i < (m+7)/8; ++i) {
            this->bf[i] |= other.bf[i];
        }
        return *this;
    }

    /** \brief union operator
     *  \param other the bloom filter with which we want to perform the union.
     */
    const BloomFilter operator+(const BloomFilter &other) const{
        return BloomFilter(*this) += other;
    }


    /** \brief returns a serialized representation of the bloom filter.
     *
     * \return serialization of the bloom filter.
     */
    std::array<uint8_t, (m+7)/8> serialize() {
        return bf;
    }


    ~BloomFilter() = default;

private:

    BloomFilter() = default;

    /** \brief adds the hashes passed as parameter to the internal list of functions to use to represent the set.
     * \param l the list of hash functions to use
     */
    inline void addHashes(std::initializer_list<std::function<size_t(const ElementType &)>> l) {
        if(l.size() != k && l.size() != 1) throw std::invalid_argument("The number of hash function passed to the constructor is not the one required.");
        if(l.size() == 1) return generateHashes(*l.begin());
        int i = 0;
        for(auto &cur : l) {
            hash_functions[i++] = cur;
        }
    }
    /** generates h hash functions starting from one.
     * \param h the hash function
     * \param seed the seed for generation.
     */
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
    //bytes representing the bloom filter
    std::array<uint8_t, (m+7)/8> bf;
    //array of hash functions.
    std::array<std::function<size_t(const ElementType &)>, k> hash_functions;

};


}
}
}