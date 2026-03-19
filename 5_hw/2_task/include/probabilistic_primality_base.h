#pragma once
#include "i_primality_test.h"
#include <cmath>
#include <stdexcept>
#include <string>

namespace crypto {

/// ─────────────────────────────────────────────────────────────────────────────
/// Абстрактный базовый класс вероятностного теста простоты.
///
/// Реализует паттерн «Шаблонный метод» (Template Method):
///   is_prime()  — неизменяемый алгоритм (final)
///   run_iteration() — кастомизируемый шаг, переопределяется в наследниках
///
/// Математика количества итераций:
///   После k независимых итераций вероятность ошибки ≤ ε^k,
///   где ε — вероятность ошибки одной итерации (специфична для теста).
///   Требуемое число итераций: k ≥ ceil( log(1 - min_probability) / log(ε) )
/// ─────────────────────────────────────────────────────────────────────────────
class ProbabilisticPrimalityTestBase : public IPrimalityTest {
public:
    // ── IPrimalityTest ────────────────────────────────────────────────────────

    /// Шаблонный метод: запускает run_iteration() нужное число раз.
    /// Не переопределяется наследниками.
    PrimalityResult is_prime(long long n, double min_probability) const final {
        validate_inputs(n, min_probability);

        // Детерминированные случаи
        if (n == 2 || n == 3) return {true,  1.0, 0};
        if (n % 2 == 0)       return {false, 1.0, 0};

        const int k = required_iterations(min_probability);

        for (int i = 0; i < k; ++i) {
            if (!run_iteration(n, i)) {
                // Точно составное
                double conf = error_probability_per_iteration();
                return {false, 1.0 - conf, i + 1};
            }
        }

        // Вероятно простое
        double err = std::pow(error_probability_per_iteration(), k);
        return {true, 1.0 - err, k};
    }

protected:
    // ── Кастомизируемые точки ─────────────────────────────────────────────────

    /// Одна итерация теста для числа n (итерация номер iter_index, 0-based).
    /// @return true — «прошло», число ещё считается вероятно простым
    ///         false — найден свидетель составности, число точно составное
    virtual bool run_iteration(long long n, int iter_index) const = 0;

    /// Вероятность ошибки одной итерации (т.е. вероятность того, что
    /// составное число «пройдёт» одну итерацию).
    /// Например, для теста Ферма — 1/2 (по теореме Эрдёша).
    virtual double error_probability_per_iteration() const = 0;

    // ── Вспомогательные методы ────────────────────────────────────────────────

    /// Минимальное число итераций для достижения min_probability.
    int required_iterations(double min_probability) const {
        // k >= log(1 - min_p) / log(err_per_iter)
        double eps = error_probability_per_iteration();
        if (eps <= 0.0) return 1;
        int k = static_cast<int>(
            std::ceil(std::log(1.0 - min_probability) / std::log(eps))
        );
        return (k < 1) ? 1 : k;
    }

private:
    static void validate_inputs(long long n, double min_probability) {
        if (n <= 1)
            throw std::invalid_argument("n must be > 1");
        if (min_probability < 0.5 || min_probability >= 1.0)
            throw std::invalid_argument(
                "min_probability must be in [0.5, 1)");
    }
};

} // namespace crypto