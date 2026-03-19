#include "bitops.hpp"

#include <iostream>
#include <iomanip>
#include <bitset>
#include <string>
#include <vector>
#include <cstdint>

using namespace bitops;

static std::string toHex(uint8_t b) {
    const char h[] = "0123456789ABCDEF";
    return std::string("0x") + h[b >> 4] + h[b & 0xF];
}

// Выводит байты в виде цепочки: 0xXX(bbbbbbbb) 0xXX(bbbbbbbb) ...
static void printBits(const std::string& label, const std::vector<uint8_t>& data) {
    std::cout << label;
    for (std::size_t i = 0; i < data.size(); ++i) {
        if (i) std::cout << "  ";
        std::cout << toHex(data[i])
                  << " (" << std::bitset<8>(data[i]) << ")";
    }
    std::cout << "\n";
}

static void separator(char letter, const std::string& title) {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  " << letter << ". " << std::left << std::setw(54) << title << "║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
}

static void subsection(const std::string& title) {
    std::cout << "\n  ┌─ " << title << "\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "══════════════════════════════════════════════════════════════\n";
    std::cout << "          Демонстрация побитовых операций (bitops)\n";
    std::cout << "  Соглашение: бит 0 = LSB первого байта, бит n-1 = MSB последнего\n";
    std::cout << "══════════════════════════════════════════════════════════════\n";

    // ──────────────────────────────────────────────────────────────────────────
    // a. Циклический сдвиг влево
    // ──────────────────────────────────────────────────────────────────────────
    separator('a', "Циклический сдвиг ВЛЕВО (rotate_left)");

    subsection("Один байт, сдвиг на 1");
    {
        std::vector<uint8_t> data = {0xB3}; // 10110011
        printBits("  Вход     : ", data);
        auto r = rotate_left(data, 1);
        printBits("  ROL 1    : ", r);
        // Бит 7 (=1) → бит 0; остальные +1: 01100111 = 0x67
        std::cout << "  Ожидается: 0x67 (01100111)\n";
    }

    subsection("Один байт, сдвиг на 4 (обмен ниббл)");
    {
        std::vector<uint8_t> data = {0xF0}; // 11110000
        printBits("  Вход     : ", data);
        auto r = rotate_left(data, 4);
        printBits("  ROL 4    : ", r);
        std::cout << "  Ожидается: 0x0F (00001111)\n";
    }

    subsection("Два байта, сдвиг на 1 (перенос между байтами)");
    {
        std::vector<uint8_t> data = {0x80, 0x00}; // 10000000 00000000
        printBits("  Вход     : ", data);
        auto r = rotate_left(data, 1);
        printBits("  ROL 1    : ", r);
        std::cout << "  Ожидается: 0x00 (00000000)  0x01 (00000001)\n";
    }

    subsection("Два байта, сдвиг на 8 (обмен байтами)");
    {
        std::vector<uint8_t> data = {0xFF, 0x00};
        printBits("  Вход     : ", data);
        auto r = rotate_left(data, 8);
        printBits("  ROL 8    : ", r);
        std::cout << "  Ожидается: 0x00 (00000000)  0xFF (11111111)\n";
    }

    subsection("Сдвиг k > n (k=17 ≡ k=1 для 16-битного значения)");
    {
        std::vector<uint8_t> data = {0xAB, 0xCD};
        auto r17 = rotate_left(data, 17);
        auto r1  = rotate_left(data, 1);
        printBits("  ROL 17 : ", r17);
        printBits("  ROL  1 : ", r1);
        std::cout << "  Совпадают: " << (r17 == r1 ? "ДА" : "НЕТ") << "\n";
    }

    // ──────────────────────────────────────────────────────────────────────────
    // b. Циклический сдвиг вправо
    // ──────────────────────────────────────────────────────────────────────────
    separator('b', "Циклический сдвиг ВПРАВО (rotate_right)");

    subsection("Один байт, сдвиг на 1 (LSB оборачивается в MSB)");
    {
        std::vector<uint8_t> data = {0x01}; // 00000001
        printBits("  Вход     : ", data);
        auto r = rotate_right(data, 1);
        printBits("  ROR 1    : ", r);
        std::cout << "  Ожидается: 0x80 (10000000)\n";
    }

    subsection("Один байт 0xB3, сдвиг на 1");
    {
        std::vector<uint8_t> data = {0xB3}; // 10110011
        printBits("  Вход     : ", data);
        auto r = rotate_right(data, 1);
        printBits("  ROR 1    : ", r);
        std::cout << "  Ожидается: 0xD9 (11011001)\n";
    }

    subsection("ROL(k) затем ROR(k) = тождественное преобразование");
    {
        std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};
        printBits("  Исходное  : ", data);
        for (std::size_t k : {1u, 5u, 13u, 31u}) {
            auto roundtrip = rotate_right(rotate_left(data, k), k);
            std::cout << "  ROL(" << std::setw(2) << k << ")→ROR("
                      << std::setw(2) << k << ") = исходное? "
                      << (roundtrip == data ? "ДА" : "НЕТ") << "\n";
        }
    }

    // ──────────────────────────────────────────────────────────────────────────
    // c. Применение маски
    // ──────────────────────────────────────────────────────────────────────────
    separator('c', "Применение маски (apply_mask)");

    subsection("Маска той же длины (побайтовое AND)");
    {
        std::vector<uint8_t> data = {0xFF, 0xFF};
        std::vector<uint8_t> mask = {0xF0, 0x0F};
        printBits("  Данные   : ", data);
        printBits("  Маска    : ", mask);
        printBits("  AND      : ", apply_mask(data, mask));
        std::cout << "  Ожидается: 0xF0  0x0F\n";
    }

    subsection("Маска КОРОЧЕ данных (лишние байты обнуляются)");
    {
        std::vector<uint8_t> data = {0xFF, 0xFF, 0xFF};
        std::vector<uint8_t> mask = {0xAA};
        printBits("  Данные (3 байта): ", data);
        printBits("  Маска  (1 байт) : ", mask);
        printBits("  AND             : ", apply_mask(data, mask));
        std::cout << "  Ожидается: 0xAA  0x00  0x00\n";
    }

    subsection("Маска ДЛИННЕЕ данных (лишние байты маски игнорируются)");
    {
        std::vector<uint8_t> data = {0xFF};
        std::vector<uint8_t> mask = {0x0F, 0xFF, 0xFF};
        printBits("  Данные (1 байт) : ", data);
        printBits("  Маска  (3 байта): ", mask);
        printBits("  AND             : ", apply_mask(data, mask));
        std::cout << "  Ожидается: 0x0F\n";
    }

    subsection("Практический пример: выделение старшего ниббла");
    {
        std::vector<uint8_t> data = {0xB3}; // 10110011
        std::vector<uint8_t> mask = {0xF0}; // 11110000
        printBits("  Данные: ", data);
        printBits("  Маска : ", mask);
        printBits("  Выход : ", apply_mask(data, mask));
        std::cout << "  Ожидается: 0xB0 (старший ниббл)\n";
    }

    // ──────────────────────────────────────────────────────────────────────────
    // d. Извлечение битов [i, j]
    // ──────────────────────────────────────────────────────────────────────────
    separator('d', "Извлечение битов [i, j] (extract_bits)");

    subsection("Младший ниббл байта 0xB3 (биты [0, 3])");
    {
        std::vector<uint8_t> data = {0xB3}; // 10110011
        printBits("  Вход          : ", data);
        auto r = extract_bits(data, 0, 3);
        printBits("  extract[0,3]  : ", r);
        std::cout << "  Ожидается: 0x03 (биты 0-3 = 0011)\n";
    }

    subsection("Старший ниббл байта 0xB3 (биты [4, 7])");
    {
        std::vector<uint8_t> data = {0xB3}; // 10110011
        printBits("  Вход          : ", data);
        auto r = extract_bits(data, 4, 7);
        printBits("  extract[4,7]  : ", r);
        std::cout << "  Ожидается: 0x0B (биты 4-7 = 1011)\n";
    }

    subsection("Кросс-байтовое извлечение из двух байтов (биты [4, 11])");
    {
        std::vector<uint8_t> data = {0xF0, 0x0F};
        // LSB-first: 0xF0 = bits 0-7: 0,0,0,0,1,1,1,1
        //            0x0F = bits 8-15: 1,1,1,1,0,0,0,0
        // Биты [4,11] = 1,1,1,1,1,1,1,1 = 0xFF
        printBits("  Вход            : ", data);
        auto r = extract_bits(data, 4, 11);
        printBits("  extract[4,11]   : ", r);
        std::cout << "  Ожидается: 0xFF\n";
    }

    subsection("Один бит (бит 5 из 0xB3)");
    {
        std::vector<uint8_t> data = {0xB3}; // 10110011, бит 5 = 1
        printBits("  Вход          : ", data);
        auto r = extract_bits(data, 5, 5);
        printBits("  extract[5,5]  : ", r);
        std::cout << "  Ожидается: 0x01 (бит 5 = 1)\n";
    }

    subsection("Обратный порядок (i > j): биты [7..0] = разворот байта");
    {
        std::vector<uint8_t> data = {0xB3}; // 10110011
        printBits("  Вход          : ", data);
        auto r = extract_bits(data, 7, 0);
        printBits("  extract[7,0]  : ", r);
        // Бит7=1,бит6=0,бит5=1,бит4=1,бит3=0,бит2=0,бит1=1,бит0=1
        // упакованы: 0b11001101 = 0xCD
        std::cout << "  Ожидается: 0xCD (биты в обратном порядке)\n";
    }

    // ──────────────────────────────────────────────────────────────────────────
    // e. Обмен битов
    // ──────────────────────────────────────────────────────────────────────────
    separator('e', "Обмен битов i и j (swap_bits)");

    subsection("Обмен бита 0 (=1) и бита 7 (=0) в 0x01");
    {
        std::vector<uint8_t> data = {0x01}; // 00000001
        printBits("  Вход       : ", data);
        auto r = swap_bits(data, 0, 7);
        printBits("  swap(0,7)  : ", r);
        std::cout << "  Ожидается: 0x80 (10000000)\n";
    }

    subsection("Кросс-байтовый обмен: бит 7 ↔ бит 8 в {0x80, 0x00}");
    {
        std::vector<uint8_t> data = {0x80, 0x00};
        printBits("  Вход       : ", data);
        auto r = swap_bits(data, 7, 8);
        printBits("  swap(7,8)  : ", r);
        std::cout << "  Ожидается: 0x00 (00000000)  0x01 (00000001)\n";
    }

    subsection("Двойной обмен = тождественное преобразование");
    {
        std::vector<uint8_t> data = {0xDE, 0xAD};
        auto once  = swap_bits(data, 3, 12);
        auto twice = swap_bits(once, 3, 12);
        printBits("  Исходное  : ", data);
        printBits("  Swap×1    : ", once);
        printBits("  Swap×2    : ", twice);
        std::cout << "  Исходное == Swap×2: " << (data == twice ? "ДА" : "НЕТ") << "\n";
    }

    subsection("Обмен двух единичных битов — без изменений");
    {
        // 0xFF: все биты = 1, обмен любых двух единичных битов не меняет значение
        std::vector<uint8_t> data = {0xFF};
        auto r = swap_bits(data, 2, 5); // оба бита = 1
        printBits("  Вход      : ", data);
        printBits("  swap(2,5) : ", r);
        std::cout << "  Ожидается: без изменений (оба бита = 1)\n";
    }

    // ──────────────────────────────────────────────────────────────────────────
    // f. Установка бита
    // ──────────────────────────────────────────────────────────────────────────
    separator('f', "Установка/сброс бита i (set_bit)");

    subsection("Установить бит 0 в 1 (из 0x00)");
    {
        auto r = set_bit({0x00}, 0, true);
        printBits("  set_bit(0x00, 0, 1) = ", r);
        std::cout << "  Ожидается: 0x01\n";
    }

    subsection("Установить бит 7 в 1 (из 0x00)");
    {
        auto r = set_bit({0x00}, 7, true);
        printBits("  set_bit(0x00, 7, 1) = ", r);
        std::cout << "  Ожидается: 0x80\n";
    }

    subsection("Сбросить бит 0 в 0 (из 0xFF)");
    {
        auto r = set_bit({0xFF}, 0, false);
        printBits("  set_bit(0xFF, 0, 0) = ", r);
        std::cout << "  Ожидается: 0xFE\n";
    }

    subsection("Установить все биты по одному (0x00 → 0xFF)");
    {
        std::vector<uint8_t> data = {0x00};
        std::cout << "  Начало: ";
        printBits("", data);
        for (std::size_t i = 0; i < 8; ++i) {
            data = set_bit(data, i, true);
            std::cout << "  +bit " << i << ": ";
            printBits("", data);
        }
    }

    subsection("Кросс-байтовая установка: бит 15 (MSB байта 1) из {0x00, 0x00}");
    {
        auto r = set_bit({0x00, 0x00}, 15, true);
        printBits("  set_bit({0,0}, 15, 1) = ", r);
        std::cout << "  Ожидается: 0x00 (00000000)  0x80 (10000000)\n";
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Обработка ошибок
    // ──────────────────────────────────────────────────────────────────────────
    separator('!', "Обработка ошибок");

    auto tryOp = [](const std::string& desc, auto fn) {
        std::cout << "\n  [" << desc << "]\n    ";
        try {
            fn();
            std::cout << "ERROR: исключение не было выброшено!\n";
        } catch (const std::invalid_argument& ex) {
            std::cout << "[invalid_argument] " << ex.what() << "\n";
        } catch (const std::out_of_range& ex) {
            std::cout << "[out_of_range] " << ex.what() << "\n";
        }
    };

    tryOp("rotate_left: пустой массив",
          []{ rotate_left({}, 1); });
    tryOp("rotate_right: пустой массив",
          []{ rotate_right({}, 2); });
    tryOp("apply_mask: пустая маска",
          []{ apply_mask({0xFF}, {}); });
    tryOp("extract_bits: индекс j за пределами",
          []{ extract_bits({0xFF}, 0, 8); });
    tryOp("swap_bits: индекс i за пределами",
          []{ swap_bits({0xFF}, 0, 9); });
    tryOp("set_bit: индекс за пределами",
          []{ set_bit({0xFF}, 8, true); });

    return 0;
}