#include "sbox.hpp"

#include <iostream>
#include <functional>
#include <string>
#include <stdexcept>
#include <vector>
#include <map>
#include <unordered_map>
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
        auto fmt = [](const std::vector<uint8_t>& v) {
            std::string s = "[";
            const char hex[] = "0123456789ABCDEF";
            for (std::size_t i = 0; i < v.size(); ++i) {
                if (i) s += ", ";
                s += "0x";
                s += hex[v[i] >> 4];
                s += hex[v[i] & 0xF];
            }
            return s + "]";
        };
        throw std::runtime_error(err + "\n  expected: " + fmt(b) + "\n  got:      " + fmt(a));
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

using namespace sbox;

// ─────────────────────────────────────────────────────────────────────────────
// Общие тестовые данные
// ─────────────────────────────────────────────────────────────────────────────

// AES SubBytes S-box (полная таблица 256 → 256)
static const std::map<uint8_t, uint8_t> AES_SBOX = {
    {0x00,0x63},{0x01,0x7c},{0x02,0x77},{0x03,0x7b},{0x04,0xf2},{0x05,0x6b},
    {0x06,0x6f},{0x07,0xc5},{0x08,0x30},{0x09,0x01},{0x0a,0x67},{0x0b,0x2b},
    {0x0c,0xfe},{0x0d,0xd7},{0x0e,0xab},{0x0f,0x76},{0x10,0xca},{0x11,0x82},
    {0x12,0xc9},{0x13,0x7d},{0x14,0xfa},{0x15,0x59},{0x16,0x47},{0x17,0xf0},
    {0x18,0xad},{0x19,0xd4},{0x1a,0xa2},{0x1b,0xaf},{0x1c,0x9c},{0x1d,0xa4},
    {0x1e,0x72},{0x1f,0xc0},{0x20,0xb7},{0x21,0xfd},{0x22,0x93},{0x23,0x26},
    {0x24,0x36},{0x25,0x3f},{0x26,0xf7},{0x27,0xcc},{0x28,0x34},{0x29,0xa5},
    {0x2a,0xe5},{0x2b,0xf1},{0x2c,0x71},{0x2d,0xd8},{0x2e,0x31},{0x2f,0x15},
    {0x30,0x04},{0x31,0xc7},{0x32,0x23},{0x33,0xc3},{0x34,0x18},{0x35,0x96},
    {0x36,0x05},{0x37,0x9a},{0x38,0x07},{0x39,0x12},{0x3a,0x80},{0x3b,0xe2},
    {0x3c,0xeb},{0x3d,0x27},{0x3e,0xb2},{0x3f,0x75},{0x40,0x09},{0x41,0x83},
    {0x42,0x2c},{0x43,0x1a},{0x44,0x1b},{0x45,0x6e},{0x46,0x5a},{0x47,0xa0},
    {0x48,0x52},{0x49,0x3b},{0x4a,0xd6},{0x4b,0xb3},{0x4c,0x29},{0x4d,0xe3},
    {0x4e,0x2f},{0x4f,0x84},{0x50,0x53},{0x51,0xd1},{0x52,0x00},{0x53,0xed},
    {0x54,0x20},{0x55,0xfc},{0x56,0xb1},{0x57,0x5b},{0x58,0x6a},{0x59,0xcb},
    {0x5a,0xbe},{0x5b,0x39},{0x5c,0x4a},{0x5d,0x4c},{0x5e,0x58},{0x5f,0xcf},
    {0x60,0xd0},{0x61,0xef},{0x62,0xaa},{0x63,0xfb},{0x64,0x43},{0x65,0x4d},
    {0x66,0x33},{0x67,0x85},{0x68,0x45},{0x69,0xf9},{0x6a,0x02},{0x6b,0x7f},
    {0x6c,0x50},{0x6d,0x3c},{0x6e,0x9f},{0x6f,0xa8},{0x70,0x51},{0x71,0xa3},
    {0x72,0x40},{0x73,0x8f},{0x74,0x92},{0x75,0x9d},{0x76,0x38},{0x77,0xf5},
    {0x78,0xbc},{0x79,0xb6},{0x7a,0xda},{0x7b,0x21},{0x7c,0x10},{0x7d,0xff},
    {0x7e,0xf3},{0x7f,0xd2},{0x80,0xcd},{0x81,0x0c},{0x82,0x13},{0x83,0xec},
    {0x84,0x5f},{0x85,0x97},{0x86,0x44},{0x87,0x17},{0x88,0xc4},{0x89,0xa7},
    {0x8a,0x7e},{0x8b,0x3d},{0x8c,0x64},{0x8d,0x5d},{0x8e,0x19},{0x8f,0x73},
    {0x90,0x60},{0x91,0x81},{0x92,0x4f},{0x93,0xdc},{0x94,0x22},{0x95,0x2a},
    {0x96,0x90},{0x97,0x88},{0x98,0x46},{0x99,0xee},{0x9a,0xb8},{0x9b,0x14},
    {0x9c,0xde},{0x9d,0x5e},{0x9e,0x0b},{0x9f,0xdb},{0xa0,0xe0},{0xa1,0x32},
    {0xa2,0x3a},{0xa3,0x0a},{0xa4,0x49},{0xa5,0x06},{0xa6,0x24},{0xa7,0x5c},
    {0xa8,0xc2},{0xa9,0xd3},{0xaa,0xac},{0xab,0x62},{0xac,0x91},{0xad,0x95},
    {0xae,0xe4},{0xaf,0x79},{0xb0,0xe7},{0xb1,0xc8},{0xb2,0x37},{0xb3,0x6d},
    {0xb4,0x8d},{0xb5,0xd5},{0xb6,0x4e},{0xb7,0xa9},{0xb8,0x6c},{0xb9,0x56},
    {0xba,0xf4},{0xbb,0xea},{0xbc,0x65},{0xbd,0x7a},{0xbe,0xae},{0xbf,0x08},
    {0xc0,0xba},{0xc1,0x78},{0xc2,0x25},{0xc3,0x2e},{0xc4,0x1c},{0xc5,0xa6},
    {0xc6,0xb4},{0xc7,0xc6},{0xc8,0xe8},{0xc9,0xdd},{0xca,0x74},{0xcb,0x1f},
    {0xcc,0x4b},{0xcd,0xbd},{0xce,0x8b},{0xcf,0x8a},{0xd0,0x70},{0xd1,0x3e},
    {0xd2,0xb5},{0xd3,0x66},{0xd4,0x48},{0xd5,0x03},{0xd6,0xf6},{0xd7,0x0e},
    {0xd8,0x61},{0xd9,0x35},{0xda,0x57},{0xdb,0xb9},{0xdc,0x86},{0xdd,0xc1},
    {0xde,0x1d},{0xdf,0x9e},{0xe0,0xe1},{0xe1,0xf8},{0xe2,0x98},{0xe3,0x11},
    {0xe4,0x69},{0xe5,0xd9},{0xe6,0x8e},{0xe7,0x94},{0xe8,0x9b},{0xe9,0x1e},
    {0xea,0x87},{0xeb,0xe9},{0xec,0xce},{0xed,0x55},{0xee,0x28},{0xef,0xdf},
    {0xf0,0x8c},{0xf1,0xa1},{0xf2,0x89},{0xf3,0x0d},{0xf4,0xbf},{0xf5,0xe6},
    {0xf6,0x42},{0xf7,0x68},{0xf8,0x41},{0xf9,0x99},{0xfa,0x2d},{0xfb,0x0f},
    {0xfc,0xb0},{0xfd,0x54},{0xfe,0xbb},{0xff,0x16}
};

// ─────────────────────────────────────────────────────────────────────────────
// Тесты — Перегрузка 1: ассоциативный контейнер
// ─────────────────────────────────────────────────────────────────────────────

void tests_map_overload() {

    // ── 1. Простая замена через std::map ──────────────────────────────────────
    test::run("map_simple_substitution", [] {
        std::map<uint8_t, uint8_t> table = {{0x00, 0xFF}, {0x01, 0xAA}, {0xFF, 0x00}};
        std::vector<uint8_t> data = {0x00, 0x01, 0xFF};
        auto result = substitute(data, table);
        test::assertEqual(result, {0xFF, 0xAA, 0x00}, "simple 3-byte map substitution");
    });

    // ── 2. Тождественная замена (каждый байт → сам себе) ──────────────────────
    test::run("map_identity_substitution", [] {
        std::map<uint8_t, uint8_t> table;
        for (int i = 0; i < 256; ++i)
            table[static_cast<uint8_t>(i)] = static_cast<uint8_t>(i);
        std::vector<uint8_t> data = {0x00, 0x42, 0xAB, 0xFF};
        auto result = substitute(data, table);
        test::assertEqual(result, data, "identity substitution");
    });

    // ── 3. AES SubBytes — тестовый вектор из спецификации FIPS 197 ───────────
    // Первые 4 байта первого раунда: {0x19, 0x3d, 0xe3, 0xbe}
    // Ожидаемый результат:           {0xd4, 0x27, 0x11, 0xae}
    test::run("map_aes_subbytes_known_vector", [] {
        std::vector<uint8_t> data   = {0x19, 0x3d, 0xe3, 0xbe};
        std::vector<uint8_t> expect = {0xd4, 0x27, 0x11, 0xae};

        // Строим локальную выборку из AES_SBOX только для нужных байтов
        std::map<uint8_t, uint8_t> local_sbox = {
            {0x19, 0xd4}, {0x3d, 0x27}, {0xe3, 0x11}, {0xbe, 0xae}
        };
        auto result = substitute(data, local_sbox);
        test::assertEqual(result, expect, "AES SubBytes known vector");
    });

    // ── 4. Замена через std::unordered_map ────────────────────────────────────
    test::run("unordered_map_substitution", [] {
        std::unordered_map<uint8_t, uint8_t> table = {
            {0xAA, 0x55}, {0x55, 0xAA}, {0x0F, 0xF0}, {0xF0, 0x0F}
        };
        std::vector<uint8_t> data = {0xAA, 0x55, 0x0F, 0xF0};
        auto result = substitute(data, table);
        test::assertEqual(result, {0x55, 0xAA, 0xF0, 0x0F},
                          "unordered_map swap-pairs substitution");
    });

    // ── 5. Повторяющиеся входные байты ────────────────────────────────────────
    test::run("map_repeated_input_bytes", [] {
        std::map<uint8_t, uint8_t> table = {{0xAB, 0xCD}};
        std::vector<uint8_t> data = {0xAB, 0xAB, 0xAB};
        auto result = substitute(data, table);
        test::assertEqual(result, {0xCD, 0xCD, 0xCD}, "repeated input bytes");
    });

    // ── 6. Один байт ──────────────────────────────────────────────────────────
    test::run("map_single_byte", [] {
        std::map<uint8_t, uint8_t> table = {{0x00, 0x42}};
        auto result = substitute({0x00}, table);
        test::assertEqual(result, {0x42}, "single byte substitution");
    });

    // ── 7. Замена в «константный» результат (все байты → одно значение) ───────
    test::run("map_all_to_same_value", [] {
        std::map<uint8_t, uint8_t> table;
        for (int i = 0; i < 256; ++i)
            table[static_cast<uint8_t>(i)] = 0x00;
        std::vector<uint8_t> data = {0x12, 0x34, 0x56, 0xAB};
        auto result = substitute(data, table);
        test::assertEqual(result, {0x00, 0x00, 0x00, 0x00}, "all → 0x00");
    });

    // ── 8. Исходный массив не изменяется ──────────────────────────────────────
    test::run("map_input_not_modified", [] {
        std::map<uint8_t, uint8_t> table = {{0x01, 0xFF}, {0x02, 0xFE}};
        std::vector<uint8_t> data = {0x01, 0x02};
        std::vector<uint8_t> copy = data;
        substitute(data, table);
        test::assertEqual(data, copy, "input must not be modified");
    });

    // ── 9. Исключение: пустой входной массив ──────────────────────────────────
    test::run("map_exception_empty_data", [] {
        std::map<uint8_t, uint8_t> table = {{0x00, 0xFF}};
        test::expectThrow<std::invalid_argument>(
            [&]{ substitute({}, table); }, "empty data");
    });

    // ── 10. Исключение: пустая таблица ────────────────────────────────────────
    test::run("map_exception_empty_table", [] {
        std::map<uint8_t, uint8_t> table;
        test::expectThrow<std::invalid_argument>(
            [&]{ substitute({0x42}, table); }, "empty table");
    });

    // ── 11. Исключение: байт отсутствует в таблице ────────────────────────────
    test::run("map_exception_missing_key", [] {
        std::map<uint8_t, uint8_t> table = {{0x00, 0xFF}}; // нет 0x01
        test::expectThrow<std::out_of_range>(
            [&]{ substitute({0x00, 0x01}, table); }, "missing key 0x01");
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Тесты — Перегрузка 2: функциональный объект
// ─────────────────────────────────────────────────────────────────────────────

void tests_func_overload() {

    // ── 12. Лямбда: тождественная замена ─────────────────────────────────────
    test::run("func_identity_lambda", [] {
        auto identity = [](uint8_t b) -> uint8_t { return b; };
        std::vector<uint8_t> data = {0x00, 0x42, 0xAB, 0xFF};
        auto result = substitute(data, SBoxFunc(identity));
        test::assertEqual(result, data, "identity lambda");
    });

    // ── 13. Лямбда: инверсия всех битов (NOT) ────────────────────────────────
    test::run("func_bitwise_not", [] {
        SBoxFunc inv = [](uint8_t b) -> uint8_t { return ~b; };
        std::vector<uint8_t> data = {0x00, 0xFF, 0xAA, 0x55};
        auto result = substitute(data, inv);
        test::assertEqual(result, {0xFF, 0x00, 0x55, 0xAA}, "bitwise NOT");
    });

    // ── 14. Лямбда: сдвиг влево на 1 (ROL-like) ──────────────────────────────
    test::run("func_rotate_left_1", [] {
        SBoxFunc rol1 = [](uint8_t b) -> uint8_t {
            return static_cast<uint8_t>((b << 1) | (b >> 7));
        };
        std::vector<uint8_t> data = {0x80, 0x01, 0xAA};
        // 0x80 = 10000000 → 00000001 = 0x01
        // 0x01 = 00000001 → 00000010 = 0x02
        // 0xAA = 10101010 → 01010101 = 0x55
        auto result = substitute(data, rol1);
        test::assertEqual(result, {0x01, 0x02, 0x55}, "rotate left by 1");
    });

    // ── 15. Указатель на функцию через std::function ──────────────────────────
    test::run("func_function_pointer", [] {
        auto xor42 = [](uint8_t b) -> uint8_t {
            return static_cast<uint8_t>(b ^ 0x42);
        };
        SBoxFunc fn = xor42;
        std::vector<uint8_t> data = {0x00, 0x42, 0xFF};
        // 0x00 ^ 0x42 = 0x42
        // 0x42 ^ 0x42 = 0x00
        // 0xFF ^ 0x42 = 0xBD
        auto result = substitute(data, fn);
        test::assertEqual(result, {0x42, 0x00, 0xBD}, "XOR with 0x42");
    });

    // ── 16. Функтор (объект с operator()) ─────────────────────────────────────
    test::run("func_functor_object", [] {
        struct AddConstant {
            uint8_t c;
            explicit AddConstant(uint8_t val) : c(val) {}
            uint8_t operator()(uint8_t b) const {
                return static_cast<uint8_t>(b + c);
            }
        };
        SBoxFunc fn = AddConstant(0x01);
        std::vector<uint8_t> data = {0x00, 0xFE, 0xFF};
        // 0x00+1=0x01, 0xFE+1=0xFF, 0xFF+1=0x00 (overflow)
        auto result = substitute(data, fn);
        test::assertEqual(result, {0x01, 0xFF, 0x00}, "functor object add+1");
    });

    // ── 17. std::function захватывает внешнее состояние ─────────────────────
    test::run("func_captures_state", [] {
        uint8_t mask = 0xF0;
        SBoxFunc fn = [mask](uint8_t b) -> uint8_t {
            return static_cast<uint8_t>(b & mask);
        };
        std::vector<uint8_t> data = {0xFF, 0x0F, 0xAB};
        auto result = substitute(data, fn);
        test::assertEqual(result, {0xF0, 0x00, 0xA0}, "lambda captures mask");
    });

    // ── 18. Цепочка двух S-блоков (результат одного — вход другого) ──────────
    test::run("func_chained_sboxes", [] {
        SBoxFunc inv  = [](uint8_t b) -> uint8_t { return ~b; };
        SBoxFunc rol1 = [](uint8_t b) -> uint8_t {
            return static_cast<uint8_t>((b << 1) | (b >> 7));
        };
        std::vector<uint8_t> data = {0x01};
        // NOT(0x01) = 0xFE = 11111110
        // ROL1(0xFE) = 11111101 = 0xFD
        auto step1 = substitute(data, inv);
        auto step2 = substitute(step1, rol1);
        test::assertEqual(step2, {0xFD}, "chained: NOT then ROL1");
    });

    // ── 19. Один байт ────────────────────────────────────────────────────────
    test::run("func_single_byte", [] {
        SBoxFunc fn = [](uint8_t b) -> uint8_t {
            return static_cast<uint8_t>(b ^ 0xFF);
        };
        auto result = substitute({0xAB}, fn);
        test::assertEqual(result, {0x54}, "single byte XOR 0xFF");
    });

    // ── 20. Исходный массив не изменяется ────────────────────────────────────
    test::run("func_input_not_modified", [] {
        SBoxFunc fn = [](uint8_t b) -> uint8_t { return ~b; };
        std::vector<uint8_t> data = {0x01, 0x02, 0x03};
        std::vector<uint8_t> copy = data;
        substitute(data, fn);
        test::assertEqual(data, copy, "input must not be modified");
    });

    // ── 21. Исключение: пустой входной массив ────────────────────────────────
    test::run("func_exception_empty_data", [] {
        SBoxFunc fn = [](uint8_t b) -> uint8_t { return b; };
        test::expectThrow<std::invalid_argument>(
            [&]{ substitute({}, fn); }, "empty data");
    });

    // ── 22. Исключение: пустой std::function ─────────────────────────────────
    test::run("func_exception_null_function", [] {
        SBoxFunc fn; // пустой std::function
        test::expectThrow<std::bad_function_call>(
            [&]{ substitute({0x42}, fn); }, "null function");
    });

    // ── 23. Результирующий вектор имеет ту же длину, что и входной ───────────
    test::run("func_output_same_length", [] {
        SBoxFunc fn = [](uint8_t b) -> uint8_t { return b; };
        std::vector<uint8_t> data = {1,2,3,4,5,6,7,8};
        auto result = substitute(data, fn);
        test::assertTrue(result.size() == data.size(), "output size == input size");
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Cross-tests: оба метода дают одинаковый результат на одних и тех же данных
// ─────────────────────────────────────────────────────────────────────────────

void tests_cross() {

    // ── 24. std::map и лямбда дают одинаковый результат для XOR ─────────────
    test::run("cross_map_vs_func_xor", [] {
        constexpr uint8_t KEY = 0x5A;
        // Таблица для всех 256 значений
        std::map<uint8_t, uint8_t> table;
        for (int i = 0; i < 256; ++i)
            table[static_cast<uint8_t>(i)] = static_cast<uint8_t>(i ^ KEY);

        SBoxFunc fn = [](uint8_t b) -> uint8_t {
            return static_cast<uint8_t>(b ^ 0x5A);
        };

        std::vector<uint8_t> data = {0x00, 0x5A, 0xFF, 0xAB, 0x01};
        auto r_map  = substitute(data, table);
        auto r_func = substitute(data, fn);
        test::assertEqual(r_map, r_func, "map and func XOR must agree");
    });

    // ── 25. std::unordered_map и std::function — инверсия ────────────────────
    test::run("cross_unordered_map_vs_func_invert", [] {
        std::unordered_map<uint8_t, uint8_t> table;
        for (int i = 0; i < 256; ++i)
            table[static_cast<uint8_t>(i)] = static_cast<uint8_t>(~i);

        SBoxFunc fn = [](uint8_t b) -> uint8_t { return ~b; };

        std::vector<uint8_t> data = {0x00, 0xDE, 0xAD, 0xBE, 0xEF};
        auto r_map  = substitute(data, table);
        auto r_func = substitute(data, fn);
        test::assertEqual(r_map, r_func, "unordered_map and func invert must agree");
    });
}


int main() {
    std::cout << "=== S-Box Unit Tests ===\n\n";

    std::cout << "── Перегрузка 1: ассоциативный контейнер ──────────────────\n";
    tests_map_overload();

    std::cout << "\n── Перегрузка 2: функциональный объект ────────────────────\n";
    tests_func_overload();

    std::cout << "\n── Cross-тесты ─────────────────────────────────────────────\n";
    tests_cross();

    test::summary();
    return test::failed == 0 ? 0 : 1;
}