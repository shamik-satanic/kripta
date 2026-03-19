#pragma once
#include <cstdint>
#include <tuple>
#include <stdexcept>
#include <cmath>
#include <vector>
#include <string>

namespace crypto {

// ─── GCD & Bezout ────────────────────────────────────────────────────────────

/// Euclidean GCD for non-negative integers
long long gcd(long long a, long long b);

/// Extended Euclidean: returns (gcd, x, y) such that a*x + b*y = gcd
std::tuple<long long, long long, long long> extended_gcd(long long a, long long b);

// ─── Legendre & Jacobi symbols ───────────────────────────────────────────────

/// Legendre symbol (a/p), p must be odd prime. Returns -1, 0, or 1.
int legendre(long long a, long long p);

/// Jacobi symbol (a/n), n must be odd positive. Returns -1, 0, or 1.
int jacobi(long long a, long long n);

// ─── Modular exponentiation ───────────────────────────────────────────────────

/// Computes base^exp mod m using repeated squaring. m > 0.
long long mod_pow(long long base, long long exp, long long mod);

// ─── Multiplicative inverse ───────────────────────────────────────────────────

/// Modular multiplicative inverse of a in Z_n (a*inv ≡ 1 mod n).
/// Throws if gcd(a,n) != 1.
long long mod_inverse(long long a, long long n);

// ─── Euler's totient ──────────────────────────────────────────────────────────

/// φ(n) by definition: count k in [1,n] with gcd(k,n)=1
long long euler_totient_definition(long long n);

/// φ(n) via prime factorisation (Arithmetic Fundamental Theorem)
long long euler_totient_factorization(long long n);

/// φ(n) via discrete Fourier transform formula:
///   φ(n) = Σ_{k=1}^{n} gcd(k,n)·cos(2πk/n)
long long euler_totient_dft(long long n);

// ─── Primality (helper for Legendre) ─────────────────────────────────────────
bool is_prime(long long n);

} // namespace crypto