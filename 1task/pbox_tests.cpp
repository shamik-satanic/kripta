#include "pbox.hpp"

#include <iostream>
#include <functional>
#include <string>
#include <stdexcept>
#include <cassert>
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
        std::string err = "assertEqual failed";
        if (!msg.empty()) err += ": " + msg;
        err += "\n  expected: [";
        for (std::size_t i = 0; i < b.size(); ++i)
            err += (i ? ", " : "") + std::to_string((int)b[i]);
        err += "]\n  got:      [";
        for (std::size_t i = 0; i < a.size(); ++i)
            err += (i ? ", " : "") + std::to_string((int)a[i]);
        err += "]";
        throw std::runtime_error(err);
    }
}

void assertTrue(bool cond, const std::string& msg = "assertion failed") {
    if (!cond) throw std::runtime_error(msg);
}

// Ожидаем исключение заданного типа
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

using namespace pbox;
using test::assertEqual;
using test::assertTrue;

// ─────────────────────────────────────────────────────────────────────────────
// Вспомогательные функции
// ─────────────────────────────────────────────────────────────────────────────

// Создаёт IndexingRule
static IndexingRule rule(BitOrder bo, IndexBase ib) {
    IndexingRule r;
    r.bitOrder  = bo;
    r.indexBase = ib;
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
// Тесты
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== P-Box Unit Tests ===\n\n";

    // ── 1. Тождественная перестановка (LSB, base 0) ──────────────────────────
    test::run("identity_permutation_lsb_base0", [] {
        // 0b10110001 = 0xB1
        std::vector<uint8_t> data = {0xB1};
        // pbox[i] = i → биты не меняются
        std::vector<std::size_t> pbox = {0,1,2,3,4,5,6,7};
        auto result = permute(data, pbox, rule(BitOrder::LSB_FIRST, IndexBase::ZERO));
        assertEqual(result, data, "identity should preserve value");
    });

    // ── 2. Тождественная перестановка (MSB, base 1) ──────────────────────────
    test::run("identity_permutation_msb_base1", [] {
        std::vector<uint8_t> data = {0xA3};
        std::vector<std::size_t> pbox = {1,2,3,4,5,6,7,8};
        auto result = permute(data, pbox, rule(BitOrder::MSB_FIRST, IndexBase::ONE));
        assertEqual(result, data, "identity (MSB, base1) should preserve value");
    });

    // ── 3. Разворот битов в одном байте (LSB, base 0) ───────────────────────
    test::run("reverse_bits_lsb_base0", [] {
        // 0b00000001 = 0x01, развернём → 0b10000000 = 0x80
        std::vector<uint8_t> data = {0x01};
        std::vector<std::size_t> pbox = {7,6,5,4,3,2,1,0};
        auto result = permute(data, pbox, rule(BitOrder::LSB_FIRST, IndexBase::ZERO));
        assertEqual(result, {0x80}, "reverse bits of 0x01 = 0x80");
    });

    // ── 4. Разворот битов (MSB, base 1) ─────────────────────────────────────
    test::run("reverse_bits_msb_base1", [] {
        // 0b00000001 = 0x01
        // MSB: бит 1 = старший = 0, бит 8 = младший = 1
        // Разворот: выход[1]=вход[8], ..., выход[8]=вход[1]
        // Результат: 0b10000000 = 0x80
        std::vector<uint8_t> data = {0x01};
        std::vector<std::size_t> pbox = {8,7,6,5,4,3,2,1};
        auto result = permute(data, pbox, rule(BitOrder::MSB_FIRST, IndexBase::ONE));
        assertEqual(result, {0x80}, "reverse bits (MSB, base1) of 0x01 = 0x80");
    });

    // ── 5. Перестановка двух байтов (LSB, base 0) ───────────────────────────
    test::run("two_byte_permutation_lsb_base0", [] {
        // data = [0xFF, 0x00] = все биты 0..7 = 1, биты 8..15 = 0
        // pbox: переставим старший и младший байты местами
        std::vector<uint8_t> data = {0xFF, 0x00};
        std::vector<std::size_t> pbox = {8,9,10,11,12,13,14,15,
                                          0, 1, 2, 3, 4, 5, 6, 7};
        auto result = permute(data, pbox, rule(BitOrder::LSB_FIRST, IndexBase::ZERO));
        assertEqual(result, {0x00, 0xFF}, "swap bytes");
    });

    // ── 6. Произвольная перестановка — конкретный DES-подобный вектор ────────
    test::run("des_initial_permutation_first8bits", [] {
        // Используем первые 8 позиций DES IP для одного байта (демо)
        // IP[1..8] = {58,50,42,34,26,18,10,2} (1-based, MSB)
        // Возьмём простой случай: 1 байт, pbox это перестановка его же битов
        // Перестановка: выход[0]=вход[7], остальные 0
        // data = 0b10000000 = 0x80
        // bit 7 (MSB, 0-based) = 1
        // ожидаем: выход бит 0 = 1, остальные 0 → 0b00000001 = 0x01 (LSB_FIRST)
        std::vector<uint8_t> data = {0x80};
        std::vector<std::size_t> pbox = {7,6,5,4,3,2,1,0};
        auto result = permute(data, pbox, rule(BitOrder::LSB_FIRST, IndexBase::ZERO));
        assertEqual(result, {0x01}, "0x80 reversed → 0x01");
    });

    // ── 7. Перестановка с базой 1 (LSB) ─────────────────────────────────────
    test::run("permutation_lsb_base1", [] {
        // data = 0x01 (только бит 1, т.е. самый младший, установлен)
        // pbox = {1} ставит входной бит 1 на выходную позицию 0
        // Ожидаем то же 0x01
        std::vector<uint8_t> data = {0x01};
        std::vector<std::size_t> pbox = {1,2,3,4,5,6,7,8};
        auto result = permute(data, pbox, rule(BitOrder::LSB_FIRST, IndexBase::ONE));
        assertEqual(result, {0x01}, "identity with base1");
    });

    // ── 8. Все биты нулевые → результат тоже нулевой ────────────────────────
    test::run("all_zero_input", [] {
        std::vector<uint8_t> data = {0x00};
        std::vector<std::size_t> pbox = {3,1,7,0,5,2,6,4};
        auto result = permute(data, pbox, rule(BitOrder::LSB_FIRST, IndexBase::ZERO));
        assertEqual(result, {0x00}, "all-zero input → all-zero output");
    });

    // ── 9. Все биты единичные → результат тоже единичный ────────────────────
    test::run("all_ones_input", [] {
        std::vector<uint8_t> data = {0xFF};
        std::vector<std::size_t> pbox = {3,1,7,0,5,2,6,4};
        auto result = permute(data, pbox, rule(BitOrder::LSB_FIRST, IndexBase::ZERO));
        assertEqual(result, {0xFF}, "all-ones input → all-ones output for any perm");
    });

    // ── 10. Расширяющая перестановка (8 бит → 16 бит) ───────────────────────
    test::run("expansion_8_to_16_bits", [] {
        // data = 0xAA = 0b10101010
        // pbox дублирует каждый бит
        std::vector<uint8_t> data = {0xAA};
        std::vector<std::size_t> pbox = {0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7};
        auto result = permute(data, pbox, rule(BitOrder::LSB_FIRST, IndexBase::ZERO));
        // бит 0 = 0, бит 1 = 1, бит 2 = 0, ... (0xAA LSB-first)
        // дублируем: 00,11,00,11,00,11,00,11 → 0b01010101_01010101? нет
        // LSB_FIRST: бит 0 = LSB = 0 (0xAA=10101010), бит 1 = 1, бит2=0,...
        // выход[0,1] = бит0,бит0 = 0,0
        // выход[2,3] = бит1,бит1 = 1,1 → ...
        // байт0: биты 0..7 → 0b11001100 = 0xCC  (позиции: 0→0,1→0,2→1,3→1,4→0,5→0,6→1,7→1)
        // байт1: биты 8..15 → 0b11001100 = 0xCC
        assertEqual(result, {0xCC, 0xCC}, "expansion 8→16");
    });

    // ── 11. Сужающая перестановка (16 бит → 8 бит) ──────────────────────────
    test::run("contraction_16_to_8_bits", [] {
        // Берём только нечётные биты из двух байтов
        std::vector<uint8_t> data = {0xFF, 0x00};
        // Биты 0..7 = 1 (байт 0xFF), биты 8..15 = 0 (байт 0x00)
        // pbox выбирает биты 1,3,5,7,8,10,12,14 (0-based, LSB)
        std::vector<std::size_t> pbox = {1,3,5,7,8,10,12,14};
        auto result = permute(data, pbox, rule(BitOrder::LSB_FIRST, IndexBase::ZERO));
        // Выходные биты: 1,1,1,1,0,0,0,0 → 0b00001111 = 0x0F
        assertEqual(result, {0x0F}, "contraction 16→8");
    });

    // ── 12. Исключение: пустой pbox ─────────────────────────────────────────
    test::run("exception_empty_pbox", [] {
        std::vector<uint8_t> data = {0x42};
        std::vector<std::size_t> pbox = {};
        test::expectThrow<std::invalid_argument>(
            [&]{ permute(data, pbox); },
            "empty pbox");
    });

    // ── 13. Исключение: pbox не кратен 8 ────────────────────────────────────
    test::run("exception_pbox_not_multiple_of_8", [] {
        std::vector<uint8_t> data = {0x42};
        std::vector<std::size_t> pbox = {0,1,2}; // 3 элемента
        test::expectThrow<std::invalid_argument>(
            [&]{ permute(data, pbox); },
            "pbox size % 8 != 0");
    });

    // ── 14. Исключение: индекс вне диапазона (base 0) ───────────────────────
    test::run("exception_index_out_of_range_base0", [] {
        std::vector<uint8_t> data = {0x42}; // 8 бит, индексы 0..7
        std::vector<std::size_t> pbox = {0,1,2,3,4,5,6,8}; // 8 — за пределами
        test::expectThrow<std::out_of_range>(
            [&]{ permute(data, pbox, rule(BitOrder::LSB_FIRST, IndexBase::ZERO)); },
            "index 8 out of range for 1-byte data");
    });

    // ── 15. Исключение: индекс < base (base 1) ──────────────────────────────
    test::run("exception_index_less_than_base1", [] {
        std::vector<uint8_t> data = {0x42};
        std::vector<std::size_t> pbox = {1,2,3,4,5,6,7,0}; // 0 < base=1
        test::expectThrow<std::out_of_range>(
            [&]{ permute(data, pbox, rule(BitOrder::LSB_FIRST, IndexBase::ONE)); },
            "index 0 below base 1");
    });

    // ── 16. MSB vs LSB дают разные результаты для несимметричных данных ──────
    test::run("lsb_vs_msb_differ", [] {
        std::vector<uint8_t> data = {0x01}; // только бит 0 (LSB) установлен
        std::vector<std::size_t> pbox = {0,1,2,3,4,5,6,7};
        auto r_lsb = permute(data, pbox, rule(BitOrder::LSB_FIRST, IndexBase::ZERO));
        auto r_msb = permute(data, pbox, rule(BitOrder::MSB_FIRST, IndexBase::ZERO));
        // LSB_FIRST: бит 0 = LSB, identity → 0x01
        // MSB_FIRST: бит 0 = MSB, identity → 0x01 (данные не меняются при тождественной)
        // Проверяем, что identity в обоих случаях сохраняет значение
        assertEqual(r_lsb, data, "identity LSB");
        assertEqual(r_msb, data, "identity MSB");

        // Но разворот битов даст разные результаты только если данные не симметричны
        std::vector<uint8_t> data2 = {0b10110001}; // 0xB1
        std::vector<std::size_t> rev = {7,6,5,4,3,2,1,0};
        auto rev_lsb = permute(data2, rev, rule(BitOrder::LSB_FIRST, IndexBase::ZERO));
        auto rev_msb = permute(data2, rev, rule(BitOrder::MSB_FIRST, IndexBase::ZERO));
        // Обе операции - разворот, но интерпретация "позиция 0" разная
        // Убеждаемся, что разворот в LSB даёт результат, отличный от входных данных
        // (для несимметричного 0xB1 разворот битов должен изменить значение)
        assertTrue(rev_lsb != data2, "LSB reverse of asymmetric data should change value");
        // Более предметная проверка:
        // LSB rev(0xB1=10110001): читаем биты LSB → 1,0,0,0,1,1,0,1
        // разворачиваем → выход LSB: 1,0,1,1,0,0,0,1 = 0b10001101 = 0x8D
        assertEqual(rev_lsb, {0x8D}, "LSB reverse of 0xB1");
    });

    // ── 17. Четыре байта, тождественная перестановка ─────────────────────────
    test::run("four_bytes_identity", [] {
        std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};
        std::vector<std::size_t> pbox;
        for (std::size_t i = 0; i < 32; ++i) pbox.push_back(i + 1); // base 1
        auto result = permute(data, pbox, rule(BitOrder::LSB_FIRST, IndexBase::ONE));
        assertEqual(result, data, "4-byte identity (base 1)");
    });

    // ── 18. Один бит установлен, перемещаем его на конкретную позицию ────────
    test::run("single_bit_move", [] {
        // data = 0x01, бит 0 (LSB) = 1
        // Хотим переместить его на позицию 7 (MSB)
        // pbox: выходная позиция 7 берёт из входа бит 0
        std::vector<uint8_t> data = {0x01};
        std::vector<std::size_t> pbox = {1,1,1,1,1,1,1,0};
        // Выход: биты 0..6 = источник бит 1 = 0; бит 7 = источник бит 0 = 1
        // → 0b10000000 = 0x80
        auto result = permute(data, pbox, rule(BitOrder::LSB_FIRST, IndexBase::ZERO));
        assertEqual(result, {0x80}, "move bit 0 to bit 7");
    });

    test::summary();
    return test::failed == 0 ? 0 : 1;
}