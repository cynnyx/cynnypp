#include <functional>
#include <array>
#include <initializer_list>
#include <stdexcept>

template<size_t m, uint8_t k, class ElementType, typename Hash = std::hash<ElementType>>
class BloomFilter {
public:


    BloomFilter(std::initializer_list<std::function<size_t(const ElementType &)>> hashes, std::array<uint8_t, (m+7)/8> bf_serialized = std::array<uint8_t, (m+7)/8>{}) : bf(bf_serialized) {
            addHashes(hashes);
    }

    BloomFilter(std::function<size_t(const ElementType &)> h, std::array<uint8_t, (m+7)/8> bf_serialized = std::array<uint8_t, (m+7)/8>{}, size_t seed = 468486423) : bf(bf_serialized){
            generateHashes(h);
    }

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

    std::array<uint8_t, (m+7)/8> serialize() {
        return bf;
    }

private:

    inline void addHashes(std::initializer_list<std::function<size_t(const ElementType &)>> l) {
        if(l.size() != k && l.size() != 1) throw std::invalid_argument("The number of hash function passed to the constructor is not the one required.");
        if(l.size() == 1) return generateHashes((*l.begin()));
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

    std::array<uint8_t, (m+7)/8> bf;
    std::array<std::function<size_t(const ElementType &)>, k> hash_functions;

};