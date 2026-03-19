#include "../include/fermat_primality_test.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace crypto;

void separator(const std::string& title) {
    std::cout << "\n┌─────────────────────────────────────────────────┐\n";
    std::cout << "│  " << std::left << std::setw(47) << title << "│\n";
    std::cout << "└─────────────────────────────────────────────────┘\n";
}

void print_result(long long n, double min_p, const PrimalityResult& r) {
    std::cout << "  n = " << std::setw(12) << n
              << "  min_p = " << std::fixed << std::setprecision(4) << min_p
              << "  →  " << (r.is_probably_prime ? "ВЕРОЯТНО ПРОСТОЕ" : "СОСТАВНОЕ      ")
              << "  conf = " << std::setprecision(6) << r.confidence
              << "  iter = " << r.iterations_done
              << "\n";
}

int main() {
    std::cout << "╔═════════════════════════════════════════════════════╗\n";
    std::cout << "║   Вероятностный тест простоты Ферма  —  DEMO        ║\n";
    std::cout << "╚═════════════════════════════════════════════════════╝\n";

    FermatPrimalityTest fermat;

    std::cout << "\nИмя теста: " << fermat.name() << "\n";

    // ── 1. Известные простые ─────────────────────────────────────────────────
    separator("1. Известные простые числа (min_prob = 0.9999)");
    for (long long p : {2LL, 3LL, 5LL, 17LL, 97LL, 7919LL, 999983LL, 15485863LL})
        print_result(p, 0.9999, fermat.is_prime(p, 0.9999));

    // ── 2. Известные составные ───────────────────────────────────────────────
    separator("2. Составные числа (min_prob = 0.9999)");
    for (long long c : {4LL, 9LL, 15LL, 100LL, 1000LL, 99991LL * 3})
        print_result(c, 0.9999, fermat.is_prime(c, 0.9999));

    // ── 3. Числа Кармайкла (псевдопростые для теста Ферма) ──────────────────
    separator("3. Числа Кармайкла — известная слабость теста Ферма");
    std::cout << "   (могут ложно определяться как вероятно простые)\n";
    for (long long k : {561LL, 1105LL, 1729LL, 2465LL, 8911LL}) {
        auto r = fermat.is_prime(k, 0.75);
        std::cout << "  n = " << std::setw(6) << k
                  << "  →  " << (r.is_probably_prime ? "ВЕРОЯТНО ПРОСТОЕ ⚠" : "СОСТАВНОЕ       ✓")
                  << "\n";
    }

    // ── 4. Влияние min_probability на число итераций ─────────────────────────
    separator("4. Зависимость числа итераций от min_probability");
    long long n = 999983LL;
    std::cout << "  Число n = " << n << "\n\n";
    std::cout << "  " << std::left
              << std::setw(14) << "min_probability"
              << std::setw(10) << "итераций"
              << std::setw(14) << "достигнута p"
              << "вывод\n";
    std::cout << "  " << std::string(50, '-') << "\n";
    for (double mp : {0.5, 0.75, 0.9, 0.99, 0.999, 0.9999, 0.999999}) {
        auto r = fermat.is_prime(n, mp);
        std::cout << "  " << std::left
                  << std::setw(14) << std::fixed << std::setprecision(6) << mp
                  << std::setw(10) << r.iterations_done
                  << std::setw(14) << std::setprecision(8) << r.confidence
                  << (r.is_probably_prime ? "простое" : "составное")
                  << "\n";
    }

    // ── 5. Валидация входных данных ──────────────────────────────────────────
    separator("5. Обработка некорректных входных данных");
    auto try_call = [&](const std::string& label, auto fn) {
        try { fn(); std::cout << "  " << label << " → нет исключения\n"; }
        catch (const std::invalid_argument& e) {
            std::cout << "  " << label << " → invalid_argument: " << e.what() << "\n";
        }
    };
    try_call("is_prime(1, 0.99)",   [&]{ fermat.is_prime(1,   0.99); });
    try_call("is_prime(7, 0.3)",    [&]{ fermat.is_prime(7,   0.3);  });
    try_call("is_prime(7, 1.0)",    [&]{ fermat.is_prime(7,   1.0);  });
    try_call("is_prime(97, 0.75)",  [&]{ fermat.is_prime(97,  0.75); });

    std::cout << "\n✓ Demo complete.\n";
    return 0;
}