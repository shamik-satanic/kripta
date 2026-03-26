#include "primality_test.h"
#include <stdexcept>
#include <random>

namespace crypto {

// ─── Trivial deterministic pre-filter ────────────────────────────────────────

bool PrimalityTest::trivial_check(long long n) {
    if (n < 2)  return false;
    if (n == 2 || n == 3 || n == 5 || n == 7) return true;
    if (n % 2 == 0 || n % 3 == 0 || n % 5 == 0) return false;
    return true; // inconclusive
}

// ─── Random witness picker ────────────────────────────────────────────────────

long long PrimalityTest::pick_witness(long long n, int ) const {

    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<long long> dist(2, n - 2);
    return dist(rng);
}

// ─── Main interface ───────────────────────────────────────────────────────────

bool PrimalityTest::is_prime_prob(long long n, int rounds) const {
    if (rounds <= 0)
        throw std::invalid_argument("rounds must be positive");

   
    if (n < 2)  return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0) return false;
    if (n < 9)  return true;   // 5, 7
    if (n % 3 == 0) return false;

    
    for (int i = 0; i < rounds; ++i) {
        long long a = pick_witness(n, i);
        if (!one_round(n, a))
            return false; 
    }
    return true; 
}

} // namespace crypto