#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <array>

namespace rc4 {

namespace detail {

inline std::array<uint8_t, 256> ksa(const std::vector<uint8_t>& key) {
    if (key.empty())
        throw std::invalid_argument("rc4::ksa: key must not be empty");
    if (key.size() > 256)
        throw std::invalid_argument("rc4::ksa: key length must be <= 256 bytes");

    std::array<uint8_t, 256> S{};
    for (int i = 0; i < 256; ++i)
        S[static_cast<std::size_t>(i)] = static_cast<uint8_t>(i);

    std::size_t j = 0;
    for (std::size_t i = 0; i < 256; ++i) {
        j = (j + S[i] + key[i % key.size()]) % 256u;
        std::swap(S[i], S[j]);
    }
    return S;
}

inline void prga(std::array<uint8_t, 256>& S,
                 uint8_t* data, std::size_t len)
{
    std::size_t i = 0, j = 0;
    for (std::size_t k = 0; k < len; ++k) {
        i = (i + 1u) % 256u;
        j = (j + S[i]) % 256u;
        std::swap(S[i], S[j]);
        data[k] ^= S[(S[i] + S[j]) % 256u];
    }
}

} 

inline std::vector<uint8_t> encrypt(const std::vector<uint8_t>& key,
                                     const std::vector<uint8_t>& data)
{
    auto S = detail::ksa(key);
    std::vector<uint8_t> result = data;
    if (!result.empty())
        detail::prga(S, result.data(), result.size());
    return result;
}

inline void encrypt_inplace(const std::vector<uint8_t>& key,
                              std::vector<uint8_t>& data)
{
    auto S = detail::ksa(key);
    if (!data.empty())
        detail::prga(S, data.data(), data.size());
}

inline std::vector<uint8_t> encrypt(const std::string& key,
                                     const std::vector<uint8_t>& data)
{
    if (key.empty())
        throw std::invalid_argument("rc4::encrypt: key must not be empty");
    std::vector<uint8_t> key_bytes(key.begin(), key.end());
    return encrypt(key_bytes, data);
}

}