#include "rc4.hpp"

#include <iostream>
#include <functional>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>

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
            std::ostringstream ss;
            ss << "[";
            for (std::size_t i = 0; i < v.size(); ++i) {
                if (i) ss << ", ";
                ss << "0x" << std::hex << std::uppercase
                   << std::setw(2) << std::setfill('0')
                   << static_cast<int>(v[i]);
            }
            ss << "]";
            return ss.str();
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

using namespace test;

// Вспомогательная функция: строка → вектор байтов
static std::vector<uint8_t> s2v(const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

// Вспомогательная функция: hex-строка → вектор байтов
static std::vector<uint8_t> hex2v(const std::string& hex) {
    std::vector<uint8_t> result;
    for (std::size_t i = 0; i + 1 < hex.size(); i += 2) {
        result.push_back(static_cast<uint8_t>(
            std::stoul(hex.substr(i, 2), nullptr, 16)));
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Тесты по известным тестовым векторам RC4 (RFC 6229 и другие источники)
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== RC4 Unit Tests ===\n\n";

    // ── 1. Тестовый вектор: Key="Key", Plaintext="Plaintext" ─────────────────
    // Известный результат из RFC 6229 / публичных источников
    test::run("rfc_key_plaintext", [] {
        // Key = "Key" (0x4B 0x65 0x79)
        // Plaintext = "Plaintext" (0x50 0x6C 0x61 0x69 0x6E 0x74 0x65 0x78 0x74)
        // Expected ciphertext (из открытых источников):
        // BBF316E8 D940AF0A D3
        std::vector<uint8_t> key = s2v("Key");
        std::vector<uint8_t> plain = s2v("Plaintext");
        auto cipher = rc4::encrypt(key, plain);
        std::vector<uint8_t> expected = hex2v("BBF316E8D940AF0AD3");
        assertEqual(cipher, expected, "Key/Plaintext known vector");
    });

    // ── 2. Тестовый вектор: Key="Wiki", Plaintext="pedia" ────────────────────
    // Результат: 1021BF0420
    test::run("wiki_pedia_vector", [] {
        std::vector<uint8_t> key   = s2v("Wiki");
        std::vector<uint8_t> plain = s2v("pedia");
        auto cipher = rc4::encrypt(key, plain);
        std::vector<uint8_t> expected = hex2v("1021BF0420");
        assertEqual(cipher, expected, "Wiki/pedia known vector");
    });

    // ── 3. Тестовый вектор: Key="Secret", Plaintext="Attack at dawn" ─────────
    // Результат: 45A01F645FC35B383552544B9BF5
    test::run("secret_attack_at_dawn", [] {
        std::vector<uint8_t> key   = s2v("Secret");
        std::vector<uint8_t> plain = s2v("Attack at dawn");
        auto cipher = rc4::encrypt(key, plain);
        std::vector<uint8_t> expected = hex2v("45A01F645FC35B383552544B9BF5");
        assertEqual(cipher, expected, "Secret/Attack at dawn known vector");
    });

    // ── 4. Симметричность: шифрование и дешифрование одинаковы ──────────────
    test::run("encrypt_decrypt_symmetry", [] {
        std::vector<uint8_t> key   = s2v("MySecretKey");
        std::vector<uint8_t> plain = s2v("Hello, World! This is RC4 test.");
        auto cipher = rc4::encrypt(key, plain);
        auto decrypted = rc4::encrypt(key, cipher);
        assertEqual(decrypted, plain, "decrypt(encrypt(plain)) == plain");
    });

    // ── 5. Симметричность для бинарных данных ────────────────────────────────
    test::run("encrypt_decrypt_binary", [] {
        std::vector<uint8_t> key = {0xDE, 0xAD, 0xBE, 0xEF};
        std::vector<uint8_t> plain;
        for (int i = 0; i < 256; ++i)
            plain.push_back(static_cast<uint8_t>(i));
        auto cipher = rc4::encrypt(key, plain);
        assertTrue(cipher != plain, "cipher != plain for all-bytes data");
        auto decrypted = rc4::encrypt(key, cipher);
        assertEqual(decrypted, plain, "decrypt(encrypt(binary)) == binary");
    });

    // ── 6. Пустые данные → пустой результат ──────────────────────────────────
    test::run("empty_data", [] {
        std::vector<uint8_t> key = s2v("key");
        std::vector<uint8_t> empty;
        auto result = rc4::encrypt(key, empty);
        assertTrue(result.empty(), "empty data → empty result");
    });

    // ── 7. Длина результата = длина входа ────────────────────────────────────
    test::run("output_same_length", [] {
        std::vector<uint8_t> key = s2v("key");
        for (std::size_t len : {1u, 8u, 100u, 1000u, 65536u}) {
            std::vector<uint8_t> data(len, 0xAA);
            auto result = rc4::encrypt(key, data);
            assertTrue(result.size() == len,
                       "output size == input size for len=" + std::to_string(len));
        }
    });

    // ── 8. Различные ключи дают различные шифртексты ─────────────────────────
    test::run("different_keys_different_ciphertexts", [] {
        std::vector<uint8_t> plain = s2v("test data");
        auto c1 = rc4::encrypt(s2v("key1"), plain);
        auto c2 = rc4::encrypt(s2v("key2"), plain);
        assertTrue(c1 != c2, "different keys → different ciphertexts");
    });

    // ── 9. Один и тот же ключ всегда даёт одинаковый результат ──────────────
    test::run("deterministic_output", [] {
        std::vector<uint8_t> key   = s2v("deterministic");
        std::vector<uint8_t> plain = s2v("same plaintext");
        auto c1 = rc4::encrypt(key, plain);
        auto c2 = rc4::encrypt(key, plain);
        assertEqual(c1, c2, "same key+data → same result");
    });

    // ── 10. Шифрование XOR: encrypt(k, encrypt(k, x)) == x ──────────────────
    test::run("double_encrypt_is_identity", [] {
        std::vector<uint8_t> key = {0x01, 0x02, 0x03};
        std::vector<uint8_t> data = s2v("round-trip test");
        auto once  = rc4::encrypt(key, data);
        auto twice = rc4::encrypt(key, once);
        assertEqual(twice, data, "double encrypt = identity");
    });

    // ── 11. Однобайтовые данные ───────────────────────────────────────────────
    test::run("single_byte_data", [] {
        std::vector<uint8_t> key  = s2v("k");
        std::vector<uint8_t> data = {0x42};
        auto enc = rc4::encrypt(key, data);
        assertTrue(enc.size() == 1, "single byte output size = 1");
        auto dec = rc4::encrypt(key, enc);
        assertEqual(dec, data, "single byte round-trip");
    });

    // ── 12. Однобайтовый ключ ─────────────────────────────────────────────────
    test::run("single_byte_key", [] {
        std::vector<uint8_t> key  = {0xFF};
        std::vector<uint8_t> data = s2v("single byte key test");
        auto enc = rc4::encrypt(key, data);
        auto dec = rc4::encrypt(key, enc);
        assertEqual(dec, data, "single byte key round-trip");
    });

    // ── 13. Максимально длинный ключ (256 байт) ──────────────────────────────
    test::run("max_key_length", [] {
        std::vector<uint8_t> key(256);
        for (int i = 0; i < 256; ++i) key[i] = static_cast<uint8_t>(i);
        std::vector<uint8_t> data = s2v("max key length test");
        auto enc = rc4::encrypt(key, data);
        auto dec = rc4::encrypt(key, enc);
        assertEqual(dec, data, "256-byte key round-trip");
    });

    // ── 14. Ключ в виде строки (перегрузка) ──────────────────────────────────
    test::run("string_key_overload", [] {
        std::string key_str = "StringKey";
        std::vector<uint8_t> key_vec(key_str.begin(), key_str.end());
        std::vector<uint8_t> data = s2v("test string key overload");
        auto r1 = rc4::encrypt(key_str, data);
        auto r2 = rc4::encrypt(key_vec, data);
        assertEqual(r1, r2, "string key overload matches vector key");
    });

    // ── 15. encrypt_inplace эквивалентен encrypt ──────────────────────────────
    test::run("encrypt_inplace_equivalent", [] {
        std::vector<uint8_t> key  = s2v("inplace_key");
        std::vector<uint8_t> data = s2v("inplace test data");
        auto enc_copy = rc4::encrypt(key, data);
        rc4::encrypt_inplace(key, data);
        assertEqual(data, enc_copy, "encrypt_inplace == encrypt");
    });

    // ── 16. Большой объём данных (1 МБ) ──────────────────────────────────────
    test::run("large_data_roundtrip", [] {
        std::vector<uint8_t> key = s2v("large_data_key");
        std::vector<uint8_t> data(1024 * 1024);
        for (std::size_t i = 0; i < data.size(); ++i)
            data[i] = static_cast<uint8_t>(i & 0xFF);
        auto enc = rc4::encrypt(key, data);
        assertTrue(enc != data, "1MB: ciphertext != plaintext");
        auto dec = rc4::encrypt(key, enc);
        assertEqual(dec, data, "1MB round-trip");
    });

    // ── 17. Исключение: пустой ключ ──────────────────────────────────────────
    test::run("exception_empty_key", [] {
        test::expectThrow<std::invalid_argument>(
            []{ rc4::encrypt(std::vector<uint8_t>{}, {0x42}); },
            "empty key");
    });

    // ── 18. Исключение: ключ > 256 байт ──────────────────────────────────────
    test::run("exception_key_too_long", [] {
        std::vector<uint8_t> key(257, 0xAA);
        test::expectThrow<std::invalid_argument>(
            [&]{ rc4::encrypt(key, {0x42}); },
            "key > 256 bytes");
    });

    // ── 19. Различные данные с одним ключом → различные шифртексты ───────────
    test::run("different_plaintexts_different_ciphertexts", [] {
        std::vector<uint8_t> key = s2v("samekey");
        auto c1 = rc4::encrypt(key, s2v("plaintext1"));
        auto c2 = rc4::encrypt(key, s2v("plaintext2"));
        assertTrue(c1 != c2, "different plaintexts → different ciphertexts");
    });

    // ── 20. Ключ из бинарных нулей ────────────────────────────────────────────
    test::run("all_zero_key", [] {
        std::vector<uint8_t> key(16, 0x00);
        std::vector<uint8_t> data = s2v("zero key test");
        auto enc = rc4::encrypt(key, data);
        auto dec = rc4::encrypt(key, enc);
        assertEqual(dec, data, "all-zero key round-trip");
    });

    test::summary();
    return test::failed == 0 ? 0 : 1;
}