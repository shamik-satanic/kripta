#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <algorithm>

namespace bitops {

namespace detail {

/// Общее число битов в массиве байтов.
inline std::size_t total_bits(const std::vector<uint8_t>& data) noexcept {
    return data.size() * 8u;
}

/// Читает бит по логическому индексу idx (0 = LSB data[0]).
inline bool read_bit(const std::vector<uint8_t>& data, std::size_t idx) {
    // idx / 8 — номер байта; idx % 8 — позиция бита внутри байта (0 = LSB).
    return (data[idx / 8u] >> (idx % 8u)) & 1u;
}

/// Записывает бит value по логическому индексу idx.
inline void write_bit(std::vector<uint8_t>& data, std::size_t idx, bool value) {
    uint8_t mask = static_cast<uint8_t>(1u << (idx % 8u));
    if (value)
        data[idx / 8u] |=  mask;
    else
        data[idx / 8u] &= static_cast<uint8_t>(~mask);
}

/// Проверяет, что data не пуст, и возвращает число битов.
inline std::size_t checked_bits(const std::vector<uint8_t>& data,
                                 const char* fn_name) {
    if (data.empty())
        throw std::invalid_argument(std::string(fn_name) + ": data is empty");
    return total_bits(data);
}

/// Проверяет, что индекс idx < n; бросает std::out_of_range иначе.
inline void check_index(std::size_t idx, std::size_t n, const char* fn_name) {
    if (idx >= n)
        throw std::out_of_range(
            std::string(fn_name) + ": bit index " + std::to_string(idx) +
            " out of range [0, " + std::to_string(n) + ")");
}

} 

inline std::vector<uint8_t> rotate_left(const std::vector<uint8_t>& data,
                                         std::size_t k)
{
    const std::size_t n = detail::checked_bits(data, "rotate_left");
    k %= n; // приводим к [0, n)
    if (k == 0) return data;

    std::vector<uint8_t> result(data.size(), 0u);
    for (std::size_t i = 0; i < n; ++i) {
        bool bit = detail::read_bit(data, i);
        detail::write_bit(result, (i + k) % n, bit);
    }
    return result;
}

inline std::vector<uint8_t> rotate_right(const std::vector<uint8_t>& data,
                                          std::size_t k)
{
    const std::size_t n = detail::checked_bits(data, "rotate_right");
    k %= n;
    if (k == 0) return data;
    // Сдвиг вправо на k ≡ сдвиг влево на (n - k)
    return rotate_left(data, n - k);
}

inline std::vector<uint8_t> apply_mask(const std::vector<uint8_t>& data,
                                        const std::vector<uint8_t>& mask)
{
    if (data.empty())
        throw std::invalid_argument("apply_mask: data is empty");
    if (mask.empty())
        throw std::invalid_argument("apply_mask: mask is empty");

    std::vector<uint8_t> result(data.size(), 0u);
    const std::size_t common = std::min(data.size(), mask.size());
    for (std::size_t i = 0; i < common; ++i)
        result[i] = data[i] & mask[i];
    // Байты data за пределами mask остаются нулевыми (AND с 0x00).
    return result;
}

inline std::vector<uint8_t> extract_bits(const std::vector<uint8_t>& data,
                                          std::size_t i, std::size_t j)
{
    const std::size_t n = detail::checked_bits(data, "extract_bits");
    detail::check_index(i, n, "extract_bits");
    detail::check_index(j, n, "extract_bits");

    // Количество извлекаемых битов
    const std::size_t len = (i <= j) ? (j - i + 1u) : (i - j + 1u);
    const std::size_t out_bytes = (len + 7u) / 8u;
    std::vector<uint8_t> result(out_bytes, 0u);

    for (std::size_t k = 0; k < len; ++k) {
        std::size_t src = (i <= j) ? (i + k) : (i - k);
        bool bit = detail::read_bit(data, src);
        detail::write_bit(result, k, bit);
    }
    return result;
}

inline std::vector<uint8_t> swap_bits(const std::vector<uint8_t>& data,
                                       std::size_t i, std::size_t j)
{
    const std::size_t n = detail::checked_bits(data, "swap_bits");
    detail::check_index(i, n, "swap_bits");
    detail::check_index(j, n, "swap_bits");

    if (i == j) return data;

    std::vector<uint8_t> result = data; // копируем
    bool bi = detail::read_bit(result, i);
    bool bj = detail::read_bit(result, j);
    detail::write_bit(result, i, bj);
    detail::write_bit(result, j, bi);
    return result;
}

inline std::vector<uint8_t> set_bit(const std::vector<uint8_t>& data,
                                     std::size_t i, bool value)
{
    const std::size_t n = detail::checked_bits(data, "set_bit");
    detail::check_index(i, n, "set_bit");

    std::vector<uint8_t> result = data;
    detail::write_bit(result, i, value);
    return result;
}

} 