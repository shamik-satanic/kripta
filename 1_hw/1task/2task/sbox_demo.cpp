#include "sbox.hpp"

#include <iostream>
#include <iomanip>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

using namespace sbox;

static std::string byteHex(uint8_t b) {
    const char h[] = "0123456789ABCDEF";
    return std::string("0x") + h[b >> 4] + h[b & 0xF];
}

static void printBytes(const std::string& label, const std::vector<uint8_t>& data) {
    std::cout << label;
    for (std::size_t i = 0; i < data.size(); ++i) {
        if (i) std::cout << "  ";
        std::cout << byteHex(data[i]);
    }
    std::cout << "\n";
}

static void separator(const std::string& title) {
    std::cout << "\n───────────────────────────────────────────────────────────\n";
    std::cout << "  " << title << "\n";
    std::cout << "───────────────────────────────────────────────────────────\n";
}


// Сокращённый AES SubBytes — только 4 байта из примера FIPS 197
static const std::map<uint8_t, uint8_t> AES_SNIPPET = {
    {0x19, 0xd4}, {0x3d, 0x27}, {0xe3, 0x11}, {0xbe, 0xae}
};

// DES S1-блок (4 строки × 16 столбцов → 4-бит результат).
// Здесь представлен упрощённо: отображение nibble → nibble (0..15 → 0..15)
// строка 0 (биты 0,5 = 00)
static const std::map<uint8_t, uint8_t> DES_S1_ROW0 = {
    {0x0,0xE},{0x1,0x4},{0x2,0xD},{0x3,0x1},{0x4,0x2},{0x5,0xF},
    {0x6,0xB},{0x7,0x8},{0x8,0x3},{0x9,0xA},{0xA,0x6},{0xB,0xC},
    {0xC,0x5},{0xD,0x9},{0xE,0x0},{0xF,0x7}
};


int main() {
    std::cout << "============================================================\n";
    std::cout << "    Демонстрация S-блока (замена байтов)\n";
    std::cout << "============================================================\n";


    separator("ПЕРЕГРУЗКА 1: ассоциативный контейнер");

    // 1a. std::map — AES SubBytes (фрагмент)
    {
        std::cout << "\n[1a] std::map — фрагмент AES SubBytes (FIPS 197)\n";
        std::vector<uint8_t> data = {0x19, 0x3d, 0xe3, 0xbe};
        printBytes("  Вход  : ", data);
        auto result = substitute(data, AES_SNIPPET);
        printBytes("  Выход : ", result);
        std::cout << "  Ожидается: 0xD4  0x27  0x11  0xAE\n";
    }

    // 1b. std::map — инверсионная таблица
    {
        std::cout << "\n[1b] std::map — побайтовая инверсия (byte → ~byte)\n";
        std::map<uint8_t, uint8_t> inv_table;
        for (int i = 0; i < 256; ++i)
            inv_table[static_cast<uint8_t>(i)] = static_cast<uint8_t>(~i);

        std::vector<uint8_t> data = {0x00, 0xFF, 0xAA, 0x55};
        printBytes("  Вход  : ", data);
        auto result = substitute(data, inv_table);
        printBytes("  Выход : ", result);
        std::cout << "  Ожидается: 0xFF  0x00  0x55  0xAA\n";
    }

    // 1c. std::unordered_map — XOR-шифр (ключ 0x5A)
    {
        std::cout << "\n[1c] std::unordered_map — XOR-шифр, ключ = 0x5A\n";
        std::unordered_map<uint8_t, uint8_t> xor_table;
        constexpr uint8_t KEY = 0x5A;
        for (int i = 0; i < 256; ++i)
            xor_table[static_cast<uint8_t>(i)] = static_cast<uint8_t>(i ^ KEY);

        std::vector<uint8_t> plaintext  = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; // "Hello"
        printBytes("  Открытый текст : ", plaintext);
        auto ciphertext = substitute(plaintext, xor_table);
        printBytes("  Шифртекст      : ", ciphertext);
        auto decrypted  = substitute(ciphertext, xor_table); // XOR самообратен
        printBytes("  Расшифровано   : ", decrypted);
        std::cout << "  Вход == Расшифровано: "
                  << (plaintext == decrypted ? "ДА" : "НЕТ") << "\n";
    }

    // 1d. DES S1-блок (nibble → nibble)
    {
        std::cout << "\n[1d] std::map — DES S1 (строка 0): nibble → nibble\n";
        std::vector<uint8_t> nibbles = {0x0, 0x1, 0x5, 0xA, 0xF};
        printBytes("  Вход  : ", nibbles);
        auto result = substitute(nibbles, DES_S1_ROW0);
        printBytes("  Выход : ", result);
        std::cout << "  Ожидается согласно таблице DES S1 (строка 0): "
                     "0x0E  0x04  0x0F  0x06  0x07\n";
    }

    // 1e. Обработка ошибки — отсутствующий ключ
    {
        std::cout << "\n[1e] Обработка ошибки: байт отсутствует в таблице\n";
        std::map<uint8_t, uint8_t> partial = {{0x00, 0xFF}}; // нет 0xAB
        try {
            substitute({0x00, 0xAB}, partial);
            std::cout << "  ОШИБКА: исключение не было выброшено!\n";
        } catch (const std::out_of_range& ex) {
            std::cout << "  [out_of_range] " << ex.what() << "\n";
        }
    }

    // ──────────────────────────────────────────────────────────────────────────
    // ПЕРЕГРУЗКА 2: ФУНКЦИОНАЛЬНЫЙ ОБЪЕКТ
    // ──────────────────────────────────────────────────────────────────────────

    separator("ПЕРЕГРУЗКА 2: функциональный объект (std::function)");

    // 2a. Лямбда — циклический сдвиг влево на 1 (ROL)
    {
        std::cout << "\n[2a] Лямбда — ROL (циклический сдвиг влево на 1 бит)\n";
        SBoxFunc rol1 = [](uint8_t b) -> uint8_t {
            return static_cast<uint8_t>((b << 1) | (b >> 7));
        };
        std::vector<uint8_t> data = {0x80, 0x01, 0xFF, 0x55};
        printBytes("  Вход  : ", data);
        auto result = substitute(data, rol1);
        printBytes("  Выход : ", result);
        std::cout << "  Ожидается: 0x01  0x02  0xFF  0xAA\n";
    }

    // 2b. Функтор (объект с operator()) — аффинное преобразование
    {
        std::cout << "\n[2b] Функтор — аффинное преобразование: (b * a + c) mod 256\n";

        struct AffineTransform {
            uint8_t a, c;
            AffineTransform(uint8_t a_, uint8_t c_) : a(a_), c(c_) {}
            uint8_t operator()(uint8_t b) const {
                return static_cast<uint8_t>(a * b + c);
            }
        };

        SBoxFunc fn = AffineTransform(3, 7);  // f(b) = 3b + 7 mod 256
        std::vector<uint8_t> data = {0x00, 0x01, 0x02, 0x10};
        printBytes("  Вход  : ", data);
        auto result = substitute(data, fn);
        printBytes("  Выход : ", result);
        // f(0x00)=7, f(0x01)=10, f(0x02)=13, f(0x10)=3*16+7=55=0x37
        std::cout << "  Ожидается: 0x07  0x0A  0x0D  0x37\n";
    }

    // 2c. std::function захватывает переменную — параметрический S-блок
    {
        std::cout << "\n[2c] Лямбда с захватом — параметрический XOR-ключ\n";
        uint8_t key = 0xCC;
        SBoxFunc fn = [key](uint8_t b) -> uint8_t {
            return static_cast<uint8_t>(b ^ key);
        };
        std::vector<uint8_t> data = {0xAA, 0xBB, 0xCC, 0xDD};
        printBytes("  Вход  (key=0xCC): ", data);
        auto result = substitute(data, fn);
        printBytes("  Выход           : ", result);
    }

    // 2d. Цепочка S-блоков: двойное применение
    {
        std::cout << "\n[2d] Цепочка S-блоков: NOT → ROL1 → NOT\n";
        SBoxFunc inv  = [](uint8_t b) -> uint8_t { return ~b; };
        SBoxFunc rol1 = [](uint8_t b) -> uint8_t {
            return static_cast<uint8_t>((b << 1) | (b >> 7));
        };

        std::vector<uint8_t> data = {0x01, 0x80, 0xAA};
        printBytes("  Вход        : ", data);
        auto after_inv  = substitute(data,      inv);
        printBytes("  После NOT   : ", after_inv);
        auto after_rol  = substitute(after_inv, rol1);
        printBytes("  После ROL1  : ", after_rol);
        auto after_inv2 = substitute(after_rol, inv);
        printBytes("  После NOT   : ", after_inv2);
    }

    // 2e. Обработка ошибки — пустой std::function
    {
        std::cout << "\n[2e] Обработка ошибки: пустой std::function\n";
        SBoxFunc empty_fn; // не инициализирован
        try {
            substitute({0x42}, empty_fn);
            std::cout << "  ОШИБКА: исключение не было выброшено!\n";
        } catch (const std::bad_function_call& ex) {
            std::cout << "  [bad_function_call] " << ex.what() << "\n";
        }
    }

    separator("Сравнение обоих методов: XOR-замена одними и теми же данными");
    {
        constexpr uint8_t MASK = 0xF0;
        std::map<uint8_t, uint8_t> table;
        for (int i = 0; i < 256; ++i)
            table[static_cast<uint8_t>(i)] = static_cast<uint8_t>(i ^ MASK);

        SBoxFunc fn = [](uint8_t b) -> uint8_t {
            return static_cast<uint8_t>(b ^ 0xF0);
        };

        std::vector<uint8_t> data = {0x00, 0x0F, 0xAA, 0xFF};
        printBytes("  Вход                   : ", data);
        auto r1 = substitute(data, table);
        auto r2 = substitute(data, fn);
        printBytes("  Результат (map)         : ", r1);
        printBytes("  Результат (func)        : ", r2);
        std::cout << "  Результаты совпадают   : " << (r1 == r2 ? "ДА" : "НЕТ") << "\n";
    }


    return 0;
}