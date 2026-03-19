#include "math_service.h"
#include <stdexcept>
#include <cmath>
#include <vector>
#include <numbers>

namespace crypto {

// ─── Helpers ──────────────────────────────────────────────────────────────────

bool is_prime(long long n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (long long i = 3; i * i <= n; i += 2)
        if (n % i == 0) return false;
    return true;
}

// ─── GCD ─────────────────────────────────────────────────────────────────────

long long gcd(long long a, long long b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b != 0) {
        a %= b;
        long long tmp = a; a = b; b = tmp;
    }
    return a;
}

// ─── Extended GCD ────────────────────────────────────────────────────────────

std::tuple<long long, long long, long long> extended_gcd(long long a, long long b) {
    long long old_r = a, r = b;
    long long old_s = 1, s = 0;
    long long old_t = 0, t = 1;
    while (r != 0) {
        long long q = old_r / r;
        long long tmp;
        tmp = r;   r   = old_r - q * r;   old_r = tmp;
        tmp = s;   s   = old_s - q * s;   old_s = tmp;
        tmp = t;   t   = old_t - q * t;   old_t = tmp;
    }
    return {old_r, old_s, old_t};
}

// ─── Legendre symbol ──────────────────────────────────────────────────────────

int legendre(long long a, long long p) {
    if (p < 2 || !is_prime(p))
        throw std::invalid_argument("p must be an odd prime for Legendre symbol");
    if (p == 2)
        throw std::invalid_argument("p must be odd");

    a = ((a % p) + p) % p;
    if (a == 0) return 0;

    // Euler's criterion: a^((p-1)/2) mod p
    long long val = mod_pow(a, (p - 1) / 2, p);
    if (val == 1)  return  1;
    if (val == p - 1) return -1;
    return 0;
}

// ─── Jacobi symbol ────────────────────────────────────────────────────────────

int jacobi(long long a, long long n) {
    if (n <= 0 || n % 2 == 0)
        throw std::invalid_argument("n must be a positive odd integer for Jacobi symbol");

    a = ((a % n) + n) % n;
    int result = 1;

    while (a != 0) {
        // Factor out powers of 2
        while (a % 2 == 0) {
            a /= 2;
            long long r = n % 8;
            if (r == 3 || r == 5) result = -result;
        }
        // Swap
        std::swap(a, n);
        // Quadratic reciprocity
        if (a % 4 == 3 && n % 4 == 3) result = -result;
        a = a % n;
    }
    return (n == 1) ? result : 0;
}

// ─── Modular exponentiation ───────────────────────────────────────────────────

long long mod_pow(long long base, long long exp, long long mod) {
    if (mod <= 0)
        throw std::invalid_argument("Modulus must be positive");
    if (exp < 0)
        throw std::invalid_argument("Exponent must be non-negative");
    if (mod == 1) return 0;

    // Use unsigned 128-bit to avoid overflow
    typedef unsigned long long ull;
    typedef __uint128_t u128;

    ull result = 1;
    ull b = ((base % mod) + mod) % mod;
    ull e = (ull)exp;
    ull m = (ull)mod;

    while (e > 0) {
        if (e & 1)
            result = (u128)result * b % m;
        b = (u128)b * b % m;
        e >>= 1;
    }
    return (long long)result;
}

// ─── Multiplicative inverse ───────────────────────────────────────────────────

long long mod_inverse(long long a, long long n) {
    if (n <= 0)
        throw std::invalid_argument("Modulus must be positive");
    auto [g, x, y] = extended_gcd(a, n);
    (void)y;
    if (g != 1)
        throw std::invalid_argument("Inverse does not exist: gcd(a,n) != 1");
    return ((x % n) + n) % n;
}

// ─── Euler totient: by definition ────────────────────────────────────────────

long long euler_totient_definition(long long n) {
    if (n <= 0)
        throw std::invalid_argument("n must be positive");
    if (n == 1) return 1;
    long long count = 0;
    for (long long k = 1; k <= n; ++k)
        if (gcd(k, n) == 1) ++count;
    return count;
}

// ─── Euler totient: via factorisation ────────────────────────────────────────

long long euler_totient_factorization(long long n) {
    if (n <= 0)
        throw std::invalid_argument("n must be positive");
    if (n == 1) return 1;

    long long result = n;
    long long temp = n;

    for (long long p = 2; p * p <= temp; ++p) {
        if (temp % p == 0) {
            while (temp % p == 0) temp /= p;
            result -= result / p;
        }
    }
    if (temp > 1)
        result -= result / temp;

    return result;
}

// ─── Euler totient: via DFT formula ──────────────────────────────────────────

long long euler_totient_dft(long long n) {
    if (n <= 0)
        throw std::invalid_argument("n must be positive");
    if (n == 1) return 1;

    // φ(n) = Σ_{k=1}^{n} gcd(k,n) · cos(2πk/n)
    // The sum is real-valued and equals φ(n) exactly.
    const double TWO_PI = 2.0 * std::acos(-1.0);
    double sum = 0.0;
    for (long long k = 1; k <= n; ++k) {
        double angle = TWO_PI * (double)k / (double)n;
        sum += (double)gcd(k, n) * std::cos(angle);
    }
    return (long long)std::round(sum);
}

} // namespace crypto