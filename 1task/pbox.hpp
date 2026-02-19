#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace pbox {

enum class BitOrder {
    LSB_FIRST, 
    MSB_FIRST  
};


enum class IndexBase {
    ZERO, 
    ONE   
};


struct IndexingRule {
    BitOrder  bitOrder  = BitOrder::LSB_FIRST;
    IndexBase indexBase = IndexBase::ZERO;
};


namespace detail {

inline std::size_t totalBits(const std::vector<uint8_t>& data) {
    return data.size() * 8u;
}


inline bool readBit(const std::vector<uint8_t>& data,
                    std::size_t                  idx,
                    BitOrder                     order)
{
    std::size_t n = totalBits(data);
    if (idx >= n)
        throw std::out_of_range("readBit: index " + std::to_string(idx) +
                                " out of range [0, " + std::to_string(n) + ")");

    // Переводим логический индекс в (byteIndex, bitWithinByte)
    // LSB_FIRST: idx=0 → самый младший бит самого первого байта
    //   byteIndex = idx / 8,  bitPos = idx % 8  (0 = LSB)
    // MSB_FIRST: idx=0 → самый старший бит самого первого байта
    //   byteIndex = idx / 8,  bitPos = 7 - (idx % 8)  (7 = MSB)

    std::size_t byteIndex = idx / 8u;
    std::size_t bitInByte = idx % 8u;
    if (order == BitOrder::MSB_FIRST) {
        bitInByte = 7u - bitInByte;
    }
    return (data[byteIndex] >> bitInByte) & 1u;
}

/// Записывает один бит value в data по «логическому» индексу idx
inline void writeBit(std::vector<uint8_t>& data,
                     std::size_t           idx,
                     bool                  value,
                     BitOrder              order)
{
    std::size_t n = totalBits(data);
    if (idx >= n)
        throw std::out_of_range("writeBit: index " + std::to_string(idx) +
                                " out of range [0, " + std::to_string(n) + ")");

    std::size_t byteIndex = idx / 8u;
    std::size_t bitInByte = idx % 8u;
    if (order == BitOrder::MSB_FIRST) {
        bitInByte = 7u - bitInByte;
    }
    if (value)
        data[byteIndex] |=  static_cast<uint8_t>(1u << bitInByte);
    else
        data[byteIndex] &= ~static_cast<uint8_t>(1u << bitInByte);
}


} 
inline std::vector<uint8_t> permute(const std::vector<uint8_t>& data,
                                    const std::vector<std::size_t>& pbox,
                                    const IndexingRule& rule = IndexingRule{})
{
    if (pbox.empty())
        throw std::invalid_argument("permute: pbox is empty");
    if (pbox.size() % 8u != 0u)
        throw std::invalid_argument("permute: pbox size must be a multiple of 8, got " +
                                    std::to_string(pbox.size()));

    std::size_t base = (rule.indexBase == IndexBase::ONE) ? 1u : 0u;

    std::size_t n = detail::totalBits(data);
    for (std::size_t i = 0; i < pbox.size(); ++i) {
        if (pbox[i] < base)
            throw std::out_of_range("permute: pbox[" + std::to_string(i) +
                                    "] = " + std::to_string(pbox[i]) +
                                    " is less than index base " + std::to_string(base));
        std::size_t zeroBasedIdx = pbox[i] - base;
        if (zeroBasedIdx >= n)
            throw std::out_of_range("permute: pbox[" + std::to_string(i) +
                                    "] = " + std::to_string(pbox[i]) +
                                    " out of range (data has " + std::to_string(n) + " bits)");
    }

    std::vector<uint8_t> result(pbox.size() / 8u, 0u);

    for (std::size_t i = 0; i < pbox.size(); ++i) {
        std::size_t srcIdx = pbox[i] - base; 
        bool bit = detail::readBit(data, srcIdx, rule.bitOrder);
        detail::writeBit(result, i, bit, rule.bitOrder);
    }

    return result;
}

} 