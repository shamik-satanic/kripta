
#include "pbox.hpp"
#include <iostream>
#include <iomanip>
#include <bitset>
#include <string>

using namespace pbox;

static void printBytes(const std::string& label,
                       const std::vector<uint8_t>& data)
{
    std::cout << label << " [";
    for (std::size_t i = 0; i < data.size(); ++i) {
        if (i) std::cout << ", ";
        std::cout << "0x" << std::hex << std::uppercase
                  << std::setw(2) << std::setfill('0') << (int)data[i]
                  << " (" << std::bitset<8>(data[i]) << ")";
    }
    std::cout << "]\n" << std::dec;
}

static void separator(const std::string& title = "") {
    std::cout << "\n───────────────────────────────────────────────────\n";
    if (!title.empty())
        std::cout << "  " << title << "\n";
    std::cout << "───────────────────────────────────────────────────\n";
}

int main() {
    std::cout << "============================================================\n";
    std::cout << "        Демонстрация P-блока (перестановка битов)\n";
    std::cout << "============================================================\n";

    // ── Пример 1: тождественная перестановка ────────────────────────────────
    separator("1. Тождественная перестановка (LSB_FIRST, base 0)");
    {
        std::vector<uint8_t> data = {0xB1}; // 1011 0001
        std::vector<std::size_t> pbox = {0,1,2,3,4,5,6,7};
        IndexingRule r;
        r.bitOrder  = BitOrder::LSB_FIRST;
        r.indexBase = IndexBase::ZERO;

        auto result = permute(data, pbox, r);
        printBytes("Вход   ", data);
        std::cout << "P-блок : {0,1,2,3,4,5,6,7} (тождественный)\n";
        printBytes("Выход  ", result);
    }

    // ── Пример 2: разворот битов (LSB_FIRST, base 0) ────────────────────────
    separator("2. Разворот битов одного байта (LSB_FIRST, base 0)");
    {
        // 0xB1 = 1011 0001
        // LSB-порядок: бит0=1, бит1=0, бит2=0, бит3=0, бит4=1, бит5=1, бит6=0, бит7=1
        // После разворота: бит0=1, бит1=0, бит2=1, бит3=1, бит4=0, бит5=0, бит6=0, бит7=1
        //                = 1000 1101 = 0x8D
        std::vector<uint8_t> data = {0xB1};
        std::vector<std::size_t> pbox = {7,6,5,4,3,2,1,0};
        IndexingRule r;
        r.bitOrder  = BitOrder::LSB_FIRST;
        r.indexBase = IndexBase::ZERO;

        auto result = permute(data, pbox, r);
        printBytes("Вход   ", data);
        std::cout << "P-блок : {7,6,5,4,3,2,1,0} (разворот)\n";
        printBytes("Выход  ", result);
        std::cout << "Ожидается: 0x8D (10001101)\n";
    }

    // ── Пример 3: разворот битов (MSB_FIRST, base 1) ────────────────────────
    separator("3. Разворот битов одного байта (MSB_FIRST, base 1)");
    {
        std::vector<uint8_t> data = {0x01}; // 0000 0001
        std::vector<std::size_t> pbox = {8,7,6,5,4,3,2,1};
        IndexingRule r;
        r.bitOrder  = BitOrder::MSB_FIRST;
        r.indexBase = IndexBase::ONE;

        auto result = permute(data, pbox, r);
        printBytes("Вход   ", data);
        std::cout << "P-блок : {8,7,6,5,4,3,2,1} (разворот, MSB-first, нумерация с 1)\n";
        printBytes("Выход  ", result);
        std::cout << "Ожидается: 0x80 (10000000)\n";
    }

    // ── Пример 4: перестановка двух байтов (обмен) ──────────────────────────
    separator("4. Обмен байтов (16 бит, LSB_FIRST, base 0)");
    {
        std::vector<uint8_t> data = {0xFF, 0x00};
        // Результат: биты 0..7 берём из байта 1 (биты 8..15),
        //            биты 8..15 берём из байта 0 (биты 0..7)
        std::vector<std::size_t> pbox = {8,9,10,11,12,13,14,15,
                                          0, 1, 2, 3, 4, 5, 6, 7};
        IndexingRule r;
        r.bitOrder  = BitOrder::LSB_FIRST;
        r.indexBase = IndexBase::ZERO;

        auto result = permute(data, pbox, r);
        printBytes("Вход   ", data);
        std::cout << "P-блок : перестановка старшего и младшего байтов\n";
        printBytes("Выход  ", result);
        std::cout << "Ожидается: [0x00, 0xFF]\n";
    }

    // ── Пример 5: расширяющая перестановка 8 → 16 бит ───────────────────────
    separator("5. Расширение 8 → 16 бит (дублирование каждого бита, LSB_FIRST)");
    {
        std::vector<uint8_t> data = {0xAA}; // 1010 1010
        std::vector<std::size_t> pbox = {0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7};
        IndexingRule r;
        r.bitOrder  = BitOrder::LSB_FIRST;
        r.indexBase = IndexBase::ZERO;

        auto result = permute(data, pbox, r);
        printBytes("Вход   ", data);
        std::cout << "P-блок : {0,0,1,1,...,7,7} (дублирование битов)\n";
        printBytes("Выход  ", result);
        std::cout << "Ожидается: [0xCC, 0xCC]\n";
    }

    // ── Пример 6: сужающая перестановка 16 → 8 бит ──────────────────────────
    separator("6. Сужение 16 → 8 бит (нечётные биты, LSB_FIRST, base 0)");
    {
        std::vector<uint8_t> data = {0xFF, 0x00};
        std::vector<std::size_t> pbox = {1,3,5,7,8,10,12,14};
        IndexingRule r;
        r.bitOrder  = BitOrder::LSB_FIRST;
        r.indexBase = IndexBase::ZERO;

        auto result = permute(data, pbox, r);
        printBytes("Вход   ", data);
        std::cout << "P-блок : {1,3,5,7,8,10,12,14} (нечётные биты)\n";
        printBytes("Выход  ", result);
        std::cout << "Ожидается: 0x0F\n";
    }

    // ── Пример 7: обработка ошибок ───────────────────────────────────────────
    separator("7. Демонстрация обработки ошибок");
    {
        auto tryPermute = [](const std::string& desc, auto fn) {
            std::cout << "  " << desc << ":\n    ";
            try {
                fn();
                std::cout << "ERROR: исключение не было выброшено!\n";
            } catch (const std::invalid_argument& ex) {
                std::cout << "[invalid_argument] " << ex.what() << "\n";
            } catch (const std::out_of_range& ex) {
                std::cout << "[out_of_range] " << ex.what() << "\n";
            }
        };

        tryPermute("Пустой P-блок", []{
            permute({0x42}, {});
        });
        tryPermute("Размер P-блока не кратен 8", []{
            permute({0x42}, {0,1,2});
        });
        tryPermute("Индекс вне диапазона (base 0)", []{
            IndexingRule r; r.bitOrder=BitOrder::LSB_FIRST; r.indexBase=IndexBase::ZERO;
            permute({0x42}, {0,1,2,3,4,5,6,8}, r);
        });
        tryPermute("Индекс меньше базы (base 1)", []{
            IndexingRule r; r.bitOrder=BitOrder::LSB_FIRST; r.indexBase=IndexBase::ONE;
            permute({0x42}, {1,2,3,4,5,6,7,0}, r);
        });
    }

    return 0;
}