#include "../include/math_service.h"
#include <iostream>
#include <iomanip>
#include <string>

using namespace crypto;

void separator(const std::string& title) {

    std::cout << " " << std::left << std::setw(44) << title << "\n";

}

int main() {

    // GCD
    separator("a/c. GCD via Euclidean algorithm");
    long long pairs[][2] = {{48,18},{100,75},{13,7},{0,5}};
    for (auto& p : pairs)
        std::cout << "  gcd(" << p[0] << ", " << p[1] << ") = " << gcd(p[0], p[1]) << "\n";

    // Extended GCD
    separator("d. Extended Euclidean + Bezout");
    long long epairs[][2] = {{35,15},{13,7},{3,11}};
    for (auto& p : epairs) {
        auto [g, x, y] = extended_gcd(p[0], p[1]);
        std::cout << "  extended_gcd(" << p[0] << ", " << p[1] << ") → "
                  << "gcd=" << g << "  x=" << x << "  y=" << y
                  << "   (" << p[0] << "*" << x << " + " << p[1] << "*" << y
                  << " = " << p[0]*x + p[1]*y << ")\n";
    }

    // Legendre
    separator("a. Legendre symbol (a/p)");
    struct LP { long long a, p; };
    for (auto& l : std::vector<LP>{{1,5},{2,5},{4,5},{0,5},{2,7},{3,7}}) {
        std::cout << "  Legendre(" << l.a << "/" << l.p << ") = "
                  << legendre(l.a, l.p) << "\n";
    }

    // Jacobi
    separator("b. Jacobi symbol (a/n)");
    struct JP { long long a, n; };
    for (auto& j : std::vector<JP>{{2,9},{3,5},{5,9},{0,5},{30,7}}) {
        std::cout << "  Jacobi(" << j.a << "/" << j.n << ") = "
                  << jacobi(j.a, j.n) << "\n";
    }

    // Modular exponentiation
    separator("e. Modular exponentiation (base^exp mod m)");
    struct ME { long long b, e, m; };
    for (auto& x : std::vector<ME>{{2,10,1000},{3,100,13},{2,256,1000000007LL}}) {
        std::cout << "  " << x.b << "^" << x.e << " mod " << x.m
                  << " = " << mod_pow(x.b, x.e, x.m) << "\n";
    }

    // Multiplicative inverse
    separator("f. Multiplicative inverse in Z_n");
    struct MI { long long a, n; };
    for (auto& i : std::vector<MI>{{3,7},{5,11},{2,9},{10,101}}) {
        long long inv = mod_inverse(i.a, i.n);
        std::cout << "  inv(" << i.a << ", " << i.n << ") = " << inv
                  << "   [check: " << i.a << "*" << inv << " mod " << i.n
                  << " = " << (i.a * inv) % i.n << "]\n";
    }

    // Euler totient
    separator("g. Euler's totient φ(n)  — three methods");
    std::cout << "  " << std::left
              << std::setw(6) << "n"
              << std::setw(12) << "definition"
              << std::setw(14) << "factorization"
              << "DFT\n";
    std::cout << "  " << std::string(42, '-') << "\n";
    for (long long n : {1LL,2LL,6LL,9LL,10LL,12LL,30LL,36LL,100LL}) {
        std::cout << "  " << std::left
                  << std::setw(6) << n
                  << std::setw(12) << euler_totient_definition(n)
                  << std::setw(14) << euler_totient_factorization(n)
                  << euler_totient_dft(n) << "\n";
    }

    std::cout << "\n✓ Demo complete.\n";
    return 0;
}