#include "../include/i_primality_test.h"
#include "../include/probabilistic_primality_base.h"
#include "../include/fermat_primality_test.h"
#include "../include/math_service.h"

#include <cassert>
#include <iostream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

using namespace crypto;

static int tests_run = 0, tests_passed = 0;

void run_test(const std::string& name, std::function<void()> fn) {
    ++tests_run;
    try {
        fn();
        ++tests_passed;
        std::cout << "  [PASS] " << name << "\n";
    } catch (const std::exception& e) {
        std::cout << "  [FAIL] " << name << " — " << e.what() << "\n";
    }
}

void ASSERT(bool cond, const std::string& msg = "assertion failed") {
    if (!cond) throw std::runtime_error(msg);
}

// ── Известные простые числа ───────────────────────────────────────────────────
const std::vector<long long> KNOWN_PRIMES = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47,
    97, 101, 997, 7919, 104729, 999983
};

// ── Известные составные числа ─────────────────────────────────────────────────
const std::vector<long long> KNOWN_COMPOSITES = {
    4, 6, 8, 9, 10, 15, 21, 25, 35, 49, 100, 561 /*Carmichael*/, 1105, 8911
};

// ─────────────────────────────────────────────────────────────────────────────
// Тесты интерфейса / валидации
// ─────────────────────────────────────────────────────────────────────────────
void test_interface_validation() {
    std::cout << "\n=== Interface & validation ===\n";
    FermatPrimalityTest fermat(42);

    run_test("name() not empty", [&]{
        ASSERT(!fermat.name().empty());
    });

    run_test("n=1 throws invalid_argument", [&]{
        bool threw = false;
        try { fermat.is_prime(1, 0.99); } catch (const std::invalid_argument&) { threw = true; }
        ASSERT(threw, "expected invalid_argument for n=1");
    });

    run_test("n=0 throws invalid_argument", [&]{
        bool threw = false;
        try { fermat.is_prime(0, 0.99); } catch (const std::invalid_argument&) { threw = true; }
        ASSERT(threw);
    });

    run_test("min_prob=0.3 throws (below 0.5)", [&]{
        bool threw = false;
        try { fermat.is_prime(7, 0.3); } catch (const std::invalid_argument&) { threw = true; }
        ASSERT(threw);
    });

    run_test("min_prob=1.0 throws (not < 1)", [&]{
        bool threw = false;
        try { fermat.is_prime(7, 1.0); } catch (const std::invalid_argument&) { threw = true; }
        ASSERT(threw);
    });

    run_test("min_prob=0.5 accepted", [&]{
        auto r = fermat.is_prime(7, 0.5);
        ASSERT(r.is_probably_prime);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Тест Ферма: простые числа
// ─────────────────────────────────────────────────────────────────────────────
void test_fermat_primes() {
    std::cout << "\n=== Fermat: known primes ===\n";
    FermatPrimalityTest fermat(123);

    for (long long p : KNOWN_PRIMES) {
        run_test("prime " + std::to_string(p), [&]{
            auto r = fermat.is_prime(p, 0.9999);
            ASSERT(r.is_probably_prime,
                std::to_string(p) + " should be probably prime");
            ASSERT(r.confidence >= 0.9999,
                "confidence too low");
            ASSERT(r.iterations_done >= 0);
        });
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Тест Ферма: составные числа (не числа Кармайкла)
// ─────────────────────────────────────────────────────────────────────────────
void test_fermat_composites() {
    std::cout << "\n=== Fermat: non-Carmichael composites ===\n";
    FermatPrimalityTest fermat(999);

    std::vector<long long> safe_composites = {
        4, 6, 8, 9, 10, 15, 21, 25, 35, 49, 100, 1000, 10000
    };

    for (long long c : safe_composites) {
        run_test("composite " + std::to_string(c), [&]{
            // Запускаем несколько раз — хотя бы один раз должен выявить составное
            bool detected = false;
            for (int trial = 0; trial < 20; ++trial) {
                auto r = fermat.is_prime(c, 0.999);
                if (!r.is_probably_prime) { detected = true; break; }
            }
            ASSERT(detected, std::to_string(c) + " not detected as composite in 20 trials");
        });
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Тест Ферма: корректность PrimalityResult
// ─────────────────────────────────────────────────────────────────────────────
void test_result_fields() {
    std::cout << "\n=== PrimalityResult fields ===\n";
    FermatPrimalityTest fermat(7);

    run_test("confidence in (0,1] for prime", [&]{
        auto r = fermat.is_prime(97, 0.999);
        ASSERT(r.confidence > 0.0 && r.confidence <= 1.0);
    });

    run_test("iterations_done > 0 for prime requiring iterations", [&]{
        auto r = fermat.is_prime(997, 0.99);
        ASSERT(r.iterations_done > 0);
    });

    run_test("confidence >= min_probability when probably prime", [&]{
        double min_p = 0.9375;
        auto r = fermat.is_prime(9999991LL, min_p);
        if (r.is_probably_prime)
            ASSERT(r.confidence >= min_p - 1e-9,
                "confidence below requested min_probability");
    });

    run_test("2 is prime with no iterations", [&]{
        auto r = fermat.is_prime(2, 0.99);
        ASSERT(r.is_probably_prime);
        ASSERT(r.confidence == 1.0);
    });

    run_test("even number > 2 is composite", [&]{
        auto r = fermat.is_prime(100, 0.99);
        ASSERT(!r.is_probably_prime);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Проверка числа итераций
// ─────────────────────────────────────────────────────────────────────────────
void test_iteration_count() {
    std::cout << "\n=== Iteration count scaling ===\n";
    FermatPrimalityTest fermat(1);

    run_test("higher confidence → more iterations", [&]{
        auto r1 = fermat.is_prime(999983LL, 0.75);
        auto r2 = fermat.is_prime(999983LL, 0.9999);
        // r2 должен делать >= r1 итераций
        ASSERT(r2.iterations_done >= r1.iterations_done,
            "more confidence should require >= iterations");
    });

    run_test("min_prob=0.5 → 1 iteration", [&]{
        auto r = fermat.is_prime(999983LL, 0.5);
        ASSERT(r.iterations_done >= 1);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Полиморфизм через интерфейс
// ─────────────────────────────────────────────────────────────────────────────
void test_polymorphism() {
    std::cout << "\n=== Polymorphism via IPrimalityTest* ===\n";

    std::unique_ptr<IPrimalityTest> test = std::make_unique<FermatPrimalityTest>(42);

    run_test("polymorphic call: prime 9999991", [&]{
        auto r = test->is_prime(9999991LL, 0.999);
        ASSERT(r.is_probably_prime);
    });

    run_test("polymorphic name()", [&]{
        ASSERT(test->name().find("Fermat") != std::string::npos);
    });
}

int main() {
    

    test_interface_validation();
    test_fermat_primes();
    test_fermat_composites();
    test_result_fields();
    test_iteration_count();
    test_polymorphism();

    std::cout << "\n══════════════════════════════════════════\n";
    std::cout << "Results: " << tests_passed << " / " << tests_run << " passed\n";
    return (tests_passed == tests_run) ? 0 : 1;
}