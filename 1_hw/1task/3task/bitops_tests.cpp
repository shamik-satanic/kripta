#include "bitops.hpp"

#include <iostream>
#include <functional>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstdint>


namespace test {

static int passed = 0;
static int failed = 0;

void run(const std::string& name, std::function<void()> fn) {
    try {
        fn();
        std::cout << "[PASS] " << name << "\n";
        ++passed;
    } catch (const std::exception& ex) {
        std::cout << "[FAIL] " << name << "\n       " << ex.what() << "\n";
        ++failed;
    } catch (...) {
        std::cout << "[FAIL] " << name << "\n       unknown exception\n";
        ++failed;
    }
}

void assertEqual(const std::vector<uint8_t>& a,
                 const std::vector<uint8_t>& b,
                 const std::string& msg = "")
{
    if (a != b) {
        auto fmt = [](const std::vector<uint8_t>& v) {
            const char h[] = "0123456789ABCDEF";
            std::string s = "[";
            for (std::size_t i = 0; i < v.size(); ++i) {
                if (i) s += ", ";
                s += "0x"; s += h[v[i]>>4]; s += h[v[i]&0xF];
            }
            return s + "]";
        };
        std::string err = "assertEqual failed";
        if (!msg.empty()) err += ": " + msg;
        throw std::runtime_error(
            err + "\n  expected: " + fmt(b) + "\n  got:      " + fmt(a));
    }
}

void assertTrue(bool cond, const std::string& msg = "assertion failed") {
    if (!cond) throw std::runtime_error(msg);
}

template<typename ExType, typename Fn>
void expectThrow(Fn fn, const std::string& desc = "") {
    bool caught = false;
    try { fn(); }
    catch (const ExType&) { caught = true; }
    catch (...) { throw std::runtime_error("wrong exception type: " + desc); }
    if (!caught) throw std::runtime_error("expected exception not thrown: " + desc);
}

void summary() {
    std::cout << "\n─────────────────────────────────\n";
    std::cout << "Tests passed: " << passed << "\n";
    std::cout << "Tests failed: " << failed << "\n";
    std::cout << "─────────────────────────────────\n";
}

} // namespace test

using namespace bitops;

// ─────────────────────────────────────────────────────────────────────────────
// a. rotate_left
// ─────────────────────────────────────────────────────────────────────────────

void test_rotate_left() {

    // 1. Сдвиг на 0 — тождественное преобразование
    test::run("rotate_left_by_0", [] {
        test::assertEqual(rotate_left({0xB3}, 0), {0xB3}, "k=0 → identity");
    });

    // 2. Одиночный байт, сдвиг на 1
    // 0xB3 = 1011 0011; ROL1 → 0110 0111 = 0x67 (бит 7 → бит 0; бит 6 → бит 7)
    test::run("rotate_left_1byte_by_1", [] {
        // 0xB3 = 10110011
        // Бит i → позиция (i+1)%8
        // LSB: бит0=1→1, бит1=1→2, бит2=0→3, бит3=0→4,
        //       бит4=1→5, бит5=1→6, бит6=0→7, бит7=1→0
        // результат: бит0=1(из7), бит1=1(из0), бит2=1(из1), бит3=0(из2),
        //            бит4=0(из3), бит5=1(из4), бит6=1(из5), бит7=0(из6)
        //          = 0b01100111 = 0x67
        test::assertEqual(rotate_left({0xB3}, 1), {0x67}, "0xB3 ROL1 = 0x67");
    });

    // 3. Сдвиг на n (= 8) — тождественное преобразование
    test::run("rotate_left_full_cycle", [] {
        test::assertEqual(rotate_left({0xAB}, 8), {0xAB}, "ROL n → identity");
    });

    // 4. Два байта, сдвиг на 8 — обмен байтами
    // {0xFF, 0x00} ROL 8 → {0x00, 0xFF}
    test::run("rotate_left_2bytes_by_8", [] {
        test::assertEqual(rotate_left({0xFF, 0x00}, 8), {0x00, 0xFF},
                          "2-byte ROL8 = byte swap");
    });

    // 5. Два байта, сдвиг на 1
    // {0x80, 0x00} = 10000000 00000000 (16 бит)
    // Бит 7 (MSB байта 0) → позиция 8 → байт 1, бит 0
    // Результат: {0x00, 0x01}
    test::run("rotate_left_2bytes_by_1_carry", [] {
        test::assertEqual(rotate_left({0x80, 0x00}, 1), {0x00, 0x01},
                          "carry bit from byte 0 to byte 1");
    });

    // 6. Один байт, сдвиг на 4
    // 0xF0 = 1111 0000; ROL4 → 0000 1111 = 0x0F
    test::run("rotate_left_nibble_swap", [] {
        test::assertEqual(rotate_left({0xF0}, 4), {0x0F}, "0xF0 ROL4 = 0x0F");
    });

    // 7. Сдвиг на k > n (автоматическое приведение по модулю)
    test::run("rotate_left_k_greater_than_n", [] {
        // ROL 9 ≡ ROL 1 для 8-битного значения
        auto r1  = rotate_left({0xB3}, 1);
        auto r9  = rotate_left({0xB3}, 9);
        test::assertEqual(r9, r1, "ROL 9 ≡ ROL 1 for 8-bit");
    });

    // 8. ROL(k) затем ROR(k) → исходное значение
    test::run("rotate_left_then_right_inverse", [] {
        std::vector<uint8_t> data = {0xDE, 0xAD};
        for (std::size_t k = 0; k < 16; ++k) {
            auto r = rotate_right(rotate_left(data, k), k);
            test::assertEqual(r, data, "ROL then ROR = identity for k=" + std::to_string(k));
        }
    });

    // 9. Исключение: пустой массив
    test::run("rotate_left_empty_throws", [] {
        test::expectThrow<std::invalid_argument>(
            []{ rotate_left({}, 1); }, "empty data");
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// b. rotate_right
// ─────────────────────────────────────────────────────────────────────────────

void test_rotate_right() {

    // 10. Сдвиг на 0 → тождественное преобразование
    test::run("rotate_right_by_0", [] {
        test::assertEqual(rotate_right({0xC5}, 0), {0xC5}, "k=0 → identity");
    });

    // 11. Одиночный байт, сдвиг на 1
    // 0x01 = 00000001; ROR1 → 10000000 = 0x80 (бит 0 → бит 7)
    test::run("rotate_right_1byte_by_1_lsb_wraps", [] {
        test::assertEqual(rotate_right({0x01}, 1), {0x80},
                          "0x01 ROR1: bit 0 wraps to bit 7 → 0x80");
    });

    // 12. 0xB3 ROR1
    // 0xB3 = 10110011; ROR1: бит i → позиция (i-1+8)%8 ≡ бит i вход → (i+7)%8 выход
    // бит0=1→7, бит1=1→0, бит2=0→1, бит3=0→2,
    // бит4=1→3, бит5=1→4, бит6=0→5, бит7=1→6
    // бит0=1(из бита1), бит1=0(из2), бит2=0(из3), бит3=1(из4),
    // бит4=1(из5), бит5=0(из6), бит6=1(из7), бит7=1(из0)
    // = 0b11011001 = 0xD9
    test::run("rotate_right_1byte_0xB3_by_1", [] {
        test::assertEqual(rotate_right({0xB3}, 1), {0xD9}, "0xB3 ROR1 = 0xD9");
    });

    // 13. ROR(8) для двух байтов → обмен байтами
    test::run("rotate_right_2bytes_by_8", [] {
        test::assertEqual(rotate_right({0xFF, 0x00}, 8), {0x00, 0xFF},
                          "2-byte ROR8 = byte swap");
    });

    // 14. 0xF0 ROR4 = 0x0F (симметрично ROL4)
    test::run("rotate_right_nibble_swap", [] {
        test::assertEqual(rotate_right({0xF0}, 4), {0x0F}, "0xF0 ROR4 = 0x0F");
    });

    // 15. Исключение: пустой массив
    test::run("rotate_right_empty_throws", [] {
        test::expectThrow<std::invalid_argument>(
            []{ rotate_right({}, 3); }, "empty data");
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// c. apply_mask
// ─────────────────────────────────────────────────────────────────────────────

void test_apply_mask() {

    // 16. Маска той же длины — побайтовое AND
    test::run("apply_mask_same_length", [] {
        test::assertEqual(apply_mask({0xFF, 0xFF}, {0xF0, 0x0F}),
                          {0xF0, 0x0F}, "same-length AND");
    });

    // 17. Маска короче — лишние байты data обнуляются
    test::run("apply_mask_shorter_mask", [] {
        // data=3 байта, mask=1 байт: байты 1,2 → 0x00
        test::assertEqual(apply_mask({0xFF, 0xFF, 0xFF}, {0x0F}),
                          {0x0F, 0x00, 0x00}, "mask shorter → trailing zeros");
    });

    // 18. Маска длиннее — лишние байты маски игнорируются
    test::run("apply_mask_longer_mask", [] {
        test::assertEqual(apply_mask({0xFF}, {0xAA, 0xFF, 0xFF}),
                          {0xAA}, "mask longer → only first byte used");
    });

    // 19. Нулевая маска → нулевой результат
    test::run("apply_mask_zero_mask", [] {
        test::assertEqual(apply_mask({0xDE, 0xAD}, {0x00, 0x00}),
                          {0x00, 0x00}, "zero mask → zero result");
    });

    // 20. Единичная маска → без изменений
    test::run("apply_mask_all_ones_mask", [] {
        std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE};
        test::assertEqual(apply_mask(data, {0xFF, 0xFF, 0xFF}),
                          data, "all-ones mask → identity");
    });

    // 21. Исключение: пустые данные
    test::run("apply_mask_empty_data_throws", [] {
        test::expectThrow<std::invalid_argument>(
            []{ apply_mask({}, {0xFF}); }, "empty data");
    });

    // 22. Исключение: пустая маска
    test::run("apply_mask_empty_mask_throws", [] {
        test::expectThrow<std::invalid_argument>(
            []{ apply_mask({0xFF}, {}); }, "empty mask");
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// d. extract_bits
// ─────────────────────────────────────────────────────────────────────────────

void test_extract_bits() {

    // 23. Извлечение всех битов одного байта — тождественно
    test::run("extract_bits_full_byte", [] {
        test::assertEqual(extract_bits({0xB3}, 0, 7), {0xB3},
                          "extract all 8 bits = identity");
    });

    // 24. Извлечение младших 4 битов (nibble)
    // 0xB3 = 10110011; биты 0..3 = 0011 → 0x03
    test::run("extract_bits_low_nibble", [] {
        test::assertEqual(extract_bits({0xB3}, 0, 3), {0x03},
                          "low nibble of 0xB3 = 0x03");
    });

    // 25. Извлечение старших 4 битов
    // 0xB3 = 10110011; биты 4..7 = 1011 → бит4=1,бит5=1,бит6=0,бит7=1 → 0x0B
    test::run("extract_bits_high_nibble", [] {
        test::assertEqual(extract_bits({0xB3}, 4, 7), {0x0B},
                          "high nibble of 0xB3 = 0x0B");
    });

    // 26. Извлечение одного бита (результат 1 байт, 1 бит)
    test::run("extract_bits_single_bit_1", [] {
        // 0x01: бит 0 = 1
        test::assertEqual(extract_bits({0x01}, 0, 0), {0x01}, "bit 0 of 0x01 = 1");
    });

    test::run("extract_bits_single_bit_0", [] {
        // 0xFE = 11111110: бит 0 = 0
        test::assertEqual(extract_bits({0xFE}, 0, 0), {0x00}, "bit 0 of 0xFE = 0");
    });

    // 27. Кросс-байтовое извлечение: биты 4..11 из двух байтов
    // {0xF0, 0x0F} = 11110000 00001111
    // биты 4..11: бит4=0,бит5=0,бит6=0,бит7=1, бит8=1,бит9=1,бит10=1,бит11=0
    //           = 0b00011110 = wait, пересчитаем:
    // 0xF0 = bits0-7: 0,0,0,0,1,1,1,1 (LSB-first)
    // 0x0F = bits8-15: 1,1,1,1,0,0,0,0 (LSB-first)
    // биты [4..11]: bit4=1,bit5=1,bit6=1,bit7=1, bit8=1,bit9=1,bit10=1,bit11=1 → 0xFF
    test::run("extract_bits_cross_byte", [] {
        // 0xF0 = 11110000 (LSB-first: 0,0,0,0,1,1,1,1)
        // 0x0F = 00001111 (LSB-first: 1,1,1,1,0,0,0,0)
        // биты 4..11: 1,1,1,1,1,1,1,1 → 0xFF
        test::assertEqual(extract_bits({0xF0, 0x0F}, 4, 11), {0xFF},
                          "cross-byte extract bits [4,11] of {0xF0,0x0F}");
    });

    // 28. i > j — извлечение в обратном порядке
    // 0xB3 = 10110011; биты 7..0 в обратном порядке
    // бит7=1,бит6=0,бит5=1,бит4=1,бит3=0,бит2=0,бит1=1,бит0=1
    // → упакованы как бит0=1,бит1=0,бит2=1,бит3=1,бит4=0,бит5=0,бит6=1,бит7=1 = 0xCD
    test::run("extract_bits_reverse_order", [] {
        test::assertEqual(extract_bits({0xB3}, 7, 0), {0xCD},
                          "reverse bit order of 0xB3 = 0xCD");
    });

    // 29. Исключение: пустой массив
    test::run("extract_bits_empty_throws", [] {
        test::expectThrow<std::invalid_argument>(
            []{ extract_bits({}, 0, 0); }, "empty data");
    });

    // 30. Исключение: индекс за пределами
    test::run("extract_bits_out_of_range_throws", [] {
        test::expectThrow<std::out_of_range>(
            []{ extract_bits({0xFF}, 0, 8); }, "j=8 out of range for 8-bit");
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// e. swap_bits
// ─────────────────────────────────────────────────────────────────────────────

void test_swap_bits() {

    // 31. Обмен бита 0 и бита 7
    // 0x80 = 10000000 → после обмена бит0↔бит7 → 00000001 = 0x01
    test::run("swap_bits_0_and_7", [] {
        test::assertEqual(swap_bits({0x80}, 0, 7), {0x01},
                          "swap bits 0&7 of 0x80 → 0x01");
    });

    // 32. Обмен одинаковых позиций (i == j) → без изменений
    test::run("swap_bits_same_index", [] {
        test::assertEqual(swap_bits({0xAB}, 3, 3), {0xAB},
                          "swap bit with itself → no change");
    });

    // 33. Обмен двух единичных битов → без изменений
    test::run("swap_bits_both_one", [] {
        test::assertEqual(swap_bits({0xFF}, 0, 7), {0xFF},
                          "swap two 1-bits → no change");
    });

    // 34. Обмен двух нулевых битов → без изменений
    test::run("swap_bits_both_zero", [] {
        test::assertEqual(swap_bits({0x00}, 2, 5), {0x00},
                          "swap two 0-bits → no change");
    });

    // 35. Кросс-байтовый обмен: бит 7 (старший бит байта 0) ↔ бит 8 (младший бит байта 1)
    // {0x80, 0x00} → обмен бит7 (=1) и бит8 (=0) → {0x00, 0x01}
    test::run("swap_bits_cross_byte", [] {
        test::assertEqual(swap_bits({0x80, 0x00}, 7, 8), {0x00, 0x01},
                          "cross-byte swap bits 7 and 8");
    });

    // 36. Двойной обмен возвращает исходное значение
    test::run("swap_bits_double_swap_inverse", [] {
        std::vector<uint8_t> data = {0xDE, 0xAD};
        auto r = swap_bits(swap_bits(data, 3, 12), 3, 12);
        test::assertEqual(r, data, "double swap = identity");
    });

    // 37. Входной массив не изменяется
    test::run("swap_bits_input_not_modified", [] {
        std::vector<uint8_t> data = {0xAB};
        auto copy = data;
        swap_bits(data, 0, 7);
        test::assertEqual(data, copy, "input must not be modified");
    });

    // 38. Исключение: пустой массив
    test::run("swap_bits_empty_throws", [] {
        test::expectThrow<std::invalid_argument>(
            []{ swap_bits({}, 0, 0); }, "empty data");
    });

    // 39. Исключение: индекс за пределами
    test::run("swap_bits_out_of_range_throws", [] {
        test::expectThrow<std::out_of_range>(
            []{ swap_bits({0xFF}, 0, 8); }, "j=8 out of range");
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// f. set_bit
// ─────────────────────────────────────────────────────────────────────────────

void test_set_bit() {

    // 40. Установить бит 0 в 1
    test::run("set_bit_0_to_1", [] {
        test::assertEqual(set_bit({0x00}, 0, true), {0x01},
                          "set bit 0 → 0x01");
    });

    // 41. Установить бит 7 в 1
    test::run("set_bit_7_to_1", [] {
        test::assertEqual(set_bit({0x00}, 7, true), {0x80},
                          "set bit 7 → 0x80");
    });

    // 42. Установить бит 0 в 0
    test::run("set_bit_0_to_0", [] {
        test::assertEqual(set_bit({0xFF}, 0, false), {0xFE},
                          "clear bit 0 of 0xFF → 0xFE");
    });

    // 43. Установить бит 7 в 0
    test::run("set_bit_7_to_0", [] {
        test::assertEqual(set_bit({0xFF}, 7, false), {0x7F},
                          "clear bit 7 of 0xFF → 0x7F");
    });

    // 44. Установить уже установленный бит в 1 → без изменений
    test::run("set_bit_already_set", [] {
        test::assertEqual(set_bit({0xFF}, 4, true), {0xFF},
                          "set already-set bit → no change");
    });

    // 45. Установить уже нулевой бит в 0 → без изменений
    test::run("set_bit_already_clear", [] {
        test::assertEqual(set_bit({0x00}, 4, false), {0x00},
                          "clear already-zero bit → no change");
    });

    // 46. Кросс-байтовая установка: бит 8 (LSB байта 1) двухбайтового значения
    test::run("set_bit_cross_byte", [] {
        // {0x00, 0x00}: установить бит 8 → {0x00, 0x01}
        test::assertEqual(set_bit({0x00, 0x00}, 8, true), {0x00, 0x01},
                          "set bit 8 of 2-byte value");
    });

    // 47. Последовательная установка всех битов
    test::run("set_bit_sequential_all_bits", [] {
        std::vector<uint8_t> data = {0x00};
        for (std::size_t i = 0; i < 8; ++i)
            data = set_bit(data, i, true);
        test::assertEqual(data, {0xFF}, "set all bits sequentially → 0xFF");
    });

    // 48. Последовательный сброс всех битов
    test::run("set_bit_sequential_clear_all", [] {
        std::vector<uint8_t> data = {0xFF};
        for (std::size_t i = 0; i < 8; ++i)
            data = set_bit(data, i, false);
        test::assertEqual(data, {0x00}, "clear all bits sequentially → 0x00");
    });

    // 49. Входной массив не изменяется
    test::run("set_bit_input_not_modified", [] {
        std::vector<uint8_t> data = {0xAB};
        auto copy = data;
        set_bit(data, 3, false);
        test::assertEqual(data, copy, "input must not be modified");
    });

    // 50. Исключение: пустой массив
    test::run("set_bit_empty_throws", [] {
        test::expectThrow<std::invalid_argument>(
            []{ set_bit({}, 0, true); }, "empty data");
    });

    // 51. Исключение: индекс за пределами
    test::run("set_bit_out_of_range_throws", [] {
        test::expectThrow<std::out_of_range>(
            []{ set_bit({0xFF}, 8, true); }, "index 8 out of range for 8-bit");
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Комплексные тесты: комбинирование операций
// ─────────────────────────────────────────────────────────────────────────────

void test_combined() {

    // 52. ROL + маска: сдвинуть и выделить старший ниббл
    test::run("combined_rotate_then_mask", [] {
        // 0x0F = 00001111; ROL4 → 11110000 = 0xF0; маска 0xF0 → 0xF0
        auto rotated = rotate_left({0x0F}, 4);
        auto masked  = apply_mask(rotated, {0xF0});
        test::assertEqual(masked, {0xF0}, "ROL4 then mask 0xF0");
    });

    // 53. set_bit + swap_bits: установить бит, затем переставить
    test::run("combined_set_then_swap", [] {
        // Установить бит 0, затем обменять 0 и 7
        auto after_set  = set_bit({0x00}, 0, true);   // 0x01
        auto after_swap = swap_bits(after_set, 0, 7); // 0x80
        test::assertEqual(after_swap, {0x80}, "set bit 0 then swap 0↔7 → 0x80");
    });

    // 54. extract_bits + rotate_right: извлечь ниббл, затем сдвинуть
    test::run("combined_extract_then_rotate", [] {
        // 0xB3 = 10110011; биты [4,7] = 1011 → 0x0B; ROR1 → ROL7
        // 0x0B = 00001011; ROR1 → бит 0 (=1) → бит 7 → 0b10000101 = 0x85
        auto extracted = extract_bits({0xB3}, 4, 7); // {0x0B}
        auto shifted   = rotate_right(extracted, 1);  // {0x85}
        test::assertEqual(shifted, {0x85}, "extract [4,7] then ROR1");
    });

    // 55. Полный цикл: ROL + ROR = тождественное преобразование для любого k
    test::run("combined_rol_ror_identity_multisize", [] {
        std::vector<uint8_t> data = {0xCA, 0xFE, 0xBA, 0xBE};
        for (std::size_t k = 0; k <= 32; ++k) {
            auto result = rotate_right(rotate_left(data, k), k);
            test::assertEqual(result, data,
                              "ROL+ROR identity, k=" + std::to_string(k));
        }
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== Bitops Unit Tests ===\n\n";

    std::cout << "── a. rotate_left ──────────────────────────────────────────\n";
    test_rotate_left();

    std::cout << "\n── b. rotate_right ─────────────────────────────────────────\n";
    test_rotate_right();

    std::cout << "\n── c. apply_mask ───────────────────────────────────────────\n";
    test_apply_mask();

    std::cout << "\n── d. extract_bits ─────────────────────────────────────────\n";
    test_extract_bits();

    std::cout << "\n── e. swap_bits ────────────────────────────────────────────\n";
    test_swap_bits();

    std::cout << "\n── f. set_bit ──────────────────────────────────────────────\n";
    test_set_bit();

    std::cout << "\n── Комплексные тесты ───────────────────────────────────────\n";
    test_combined();

    test::summary();
    return test::failed == 0 ? 0 : 1;
}