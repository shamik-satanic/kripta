#pragma once


#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace sbox {

using SBoxMap     = std::map<uint8_t, uint8_t>;

using SBoxHashMap = std::unordered_map<uint8_t, uint8_t>;

using SBoxFunc    = std::function<uint8_t(uint8_t)>;

template<typename AssocContainer>
std::vector<uint8_t> substitute(const std::vector<uint8_t>& data,
                                 const AssocContainer&        table)
{
    if (data.empty())
        throw std::invalid_argument("substitute: input data is empty");
    if (table.empty())
        throw std::invalid_argument("substitute: substitution table is empty");

    std::vector<uint8_t> result;
    result.reserve(data.size());

    for (std::size_t i = 0; i < data.size(); ++i) {
        auto it = table.find(data[i]);
        if (it == table.end())
            throw std::out_of_range(
                "substitute: no mapping for byte 0x" +
                [](uint8_t v) {
                    const char hex[] = "0123456789ABCDEF";
                    return std::string{hex[v >> 4], hex[v & 0xF]};
                }(data[i]) +
                " at position " + std::to_string(i));
        result.push_back(it->second);
    }

    return result;
}

inline std::vector<uint8_t> substitute(const std::vector<uint8_t>& data,
                                        const SBoxFunc&              func)
{
    if (data.empty())
        throw std::invalid_argument("substitute: input data is empty");
    if (!func)
        throw std::bad_function_call();

    std::vector<uint8_t> result;
    result.reserve(data.size());

    for (const uint8_t byte : data)
        result.push_back(func(byte));

    return result;
}

} 