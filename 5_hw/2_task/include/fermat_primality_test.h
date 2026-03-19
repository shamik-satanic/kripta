#pragma once
#include "probabilistic_primality_base.h"
#include "../include/math_service.h"
#include <random>
#include <string>

namespace crypto {

/// ─────────────────────────────────────────────────────────────────────────────
/// Вероятностный тест простоты Ферма.
///
/// Основан на малой теореме Ферма:
///   если p — простое и gcd(a, p) = 1, то  a^(p-1) ≡ 1 (mod p).
///
/// Одна итерация:
///   1. Выбрать случайное a ∈ [2, n-2]
///   2. Если gcd(a, n) > 1 → n составное (return false)
///   3. Вычислить a^(n-1) mod n
///   4. Если результат ≠ 1 → n составное (return false)
///   5. Иначе → «n вероятно простое» (return true)
///
/// Слабость: числа Кармайкла проходят тест для всех a с gcd(a,n)=1.
/// Вероятность ошибки одной итерации ≤ 1/2 (теорема Эрдёша).
/// ─────────────────────────────────────────────────────────────────────────────
class FermatPrimalityTest final : public ProbabilisticPrimalityTestBase {
public:
    explicit FermatPrimalityTest(unsigned seed = std::random_device{}())
        : rng_(seed) {}

    std::string name() const override {
        return "Fermat Probabilistic Primality Test";
    }

protected:
    bool run_iteration(long long n, int /*iter_index*/) const override {
        // Выбираем случайное основание a ∈ [2, n-2]
        std::uniform_int_distribution<long long> dist(2, n - 2);
        long long a = dist(rng_);

        // Шаг 2: проверка через НОД
        if (crypto::gcd(a, n) != 1)
            return false;  // нашли нетривиальный делитель

        // Шаг 3-4: малая теорема Ферма
        long long result = crypto::mod_pow(a, n - 1, n);
        return result == 1;
    }

    double error_probability_per_iteration() const override {
        return 0.5;  // консервативная оценка (≤ 1/2)
    }

private:
    mutable std::mt19937_64 rng_;
};

} // namespace crypto