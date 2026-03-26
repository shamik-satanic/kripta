#pragma once
#include <tuple>

namespace crypto {

bool is_prime(long long n);
long long gcd(long long a, long long b);
std::tuple<long long, long long, long long> extended_gcd(long long a, long long b);
int legendre(long long a, long long p);
int jacobi(long long a, long long n);
long long mod_pow(long long base, long long exp, long long mod);
long long mod_inverse(long long a, long long n);
long long euler_totient_definition(long long n);
long long euler_totient_factorization(long long n);
long long euler_totient_dft(long long n);

} // namespace crypto