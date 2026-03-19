#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>

namespace crypto {

/// Результат вероятностного теста простоты
struct PrimalityResult {
    bool     is_probably_prime; ///< true — вероятно простое; false — составное
    double   confidence;        ///< достигнутая нижняя граница вероятности простоты
    int      iterations_done;   ///< фактически выполнено итераций
};

/// ─────────────────────────────────────────────────────────────────────────────
/// Интерфейс вероятностного теста простоты.
///
/// Контракт:
///   is_prime(n, min_probability)
///     n              — тестируемое значение (> 1)
///     min_probability — минимальная требуемая вероятность простоты, ∈ [0.5, 1)
///
/// Реализация обязана:
///   • при составном n вернуть {false, …}
///   • при вероятно простом n вернуть {true, p} где p >= min_probability
///   • при n <= 1 или min_probability ∉ [0.5, 1) бросить std::invalid_argument
/// ─────────────────────────────────────────────────────────────────────────────
class IPrimalityTest {
public:
    virtual ~IPrimalityTest() = default;

    /// Основной метод интерфейса.
    virtual PrimalityResult is_prime(long long n, double min_probability) const = 0;

    /// Человекочитаемое название теста (для логов / UI).
    virtual std::string name() const = 0;
};

} // namespace crypto