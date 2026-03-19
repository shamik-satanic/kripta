// Unit tests – no external framework needed; pure C++14 assertions
#include "../include/math_service.h"
#include <cassert>
#include <iostream>
#include <string>
#include <functional>

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

// ─── GCD ─────────────────────────────────────────────────────────────────────

void test_gcd() {
    std::cout << "\n=== GCD ===\n";
    run_test("gcd(12,8)==4",   []{ ASSERT(gcd(12,8)==4); });
    run_test("gcd(0,5)==5",    []{ ASSERT(gcd(0,5)==5); });
    run_test("gcd(7,0)==7",    []{ ASSERT(gcd(7,0)==7); });
    run_test("gcd(1,1)==1",    []{ ASSERT(gcd(1,1)==1); });
    run_test("gcd(-12,8)==4",  []{ ASSERT(gcd(-12,8)==4); });
    run_test("gcd(100,75)==25",[]{ ASSERT(gcd(100,75)==25); });
    run_test("gcd(13,7)==1",   []{ ASSERT(gcd(13,7)==1); });
    run_test("gcd(48,18)==6",  []{ ASSERT(gcd(48,18)==6); });
}

// ─── Extended GCD ────────────────────────────────────────────────────────────

void test_extended_gcd() {
    std::cout << "\n=== Extended GCD ===\n";
    auto check = [](long long a, long long b) {
        auto [g, x, y] = extended_gcd(a, b);
        ASSERT(g > 0, "gcd must be positive");
        ASSERT(a*x + b*y == g, "Bezout relation violated");
        ASSERT(g == gcd(a, b), "gcd mismatch");
    };
    run_test("egcd(12,8)",   [&]{ check(12,8); });
    run_test("egcd(35,15)",  [&]{ check(35,15); });
    run_test("egcd(13,7)",   [&]{ check(13,7); });
    run_test("egcd(100,75)", [&]{ check(100,75); });
    run_test("egcd(3,11)",   [&]{ check(3,11); });
    run_test("egcd(1,1)",    [&]{ check(1,1); });
}

// ─── Legendre ─────────────────────────────────────────────────────────────────

void test_legendre() {
    std::cout << "\n=== Legendre symbol ===\n";
    run_test("(1/5)==1",   []{ ASSERT(legendre(1,5)==1); });
    run_test("(4/5)==1",   []{ ASSERT(legendre(4,5)==1); });
    run_test("(2/5)==-1",  []{ ASSERT(legendre(2,5)==-1); });
    run_test("(0/5)==0",   []{ ASSERT(legendre(0,5)==0); });
    run_test("(5/5)==0",   []{ ASSERT(legendre(5,5)==0); });
    run_test("(2/7)==1",   []{ ASSERT(legendre(2,7)==1); });
    run_test("(3/7)==-1",  []{ ASSERT(legendre(3,7)==-1); });
    run_test("(1/11)==1",  []{ ASSERT(legendre(1,11)==1); });
    run_test("(non-prime throws)", []{
        bool threw = false;
        try { legendre(2,4); } catch(...){ threw = true; }
        ASSERT(threw);
    });
}

// ─── Jacobi ──────────────────────────────────────────────────────────────────

void test_jacobi() {
    std::cout << "\n=== Jacobi symbol ===\n";
    run_test("(1/1)==1",   []{ ASSERT(jacobi(1,1)==1); });
    run_test("(2/9)==1",   []{ ASSERT(jacobi(2,9)==1); });   // 2 is not QR mod 9, but J=1
    run_test("(3/5)==-1",  []{ ASSERT(jacobi(3,5)==-1); });
    run_test("(5/9)==1",   []{ ASSERT(jacobi(5,9)==1); });
    run_test("(0/5)==0",   []{ ASSERT(jacobi(0,5)==0); });
    run_test("(30/7)==1",  []{ ASSERT(jacobi(30,7)==1); });
    run_test("even n throws", []{
        bool threw = false;
        try { jacobi(2,4); } catch(...){ threw = true; }
        ASSERT(threw);
    });
    // Jacobi matches Legendre for primes
    run_test("Jacobi==Legendre for prime 13", []{
        for (int a = 0; a < 13; ++a)
            ASSERT(jacobi(a,13) == legendre(a,13));
    });
}

// ─── Modular exponentiation ───────────────────────────────────────────────────

void test_mod_pow() {
    std::cout << "\n=== Modular exponentiation ===\n";
    run_test("2^10 mod 1000==24",  []{ ASSERT(mod_pow(2,10,1000)==24); });
    run_test("3^0 mod 7==1",       []{ ASSERT(mod_pow(3,0,7)==1); });
    run_test("0^5 mod 7==0",       []{ ASSERT(mod_pow(0,5,7)==0); });
    run_test("2^256 mod 1e9+7",    []{ ASSERT(mod_pow(2,256,1000000007LL) > 0); });
    run_test("Fermat: 2^(p-1) mod p==1", []{
        long long p = 998244353LL;
        ASSERT(mod_pow(2, p-1, p) == 1);
    });
    run_test("mod==1 always 0",    []{ ASSERT(mod_pow(999,999,1)==0); });
    run_test("negative base ok",   []{ ASSERT(mod_pow(-3,2,7) == mod_pow(4,2,7)); });
}

// ─── Multiplicative inverse ───────────────────────────────────────────────────

void test_mod_inverse() {
    std::cout << "\n=== Multiplicative inverse ===\n";
    auto check = [](long long a, long long n) {
        long long inv = mod_inverse(a, n);
        ASSERT(((a * inv) % n + n) % n == 1 % n, "a*inv != 1 mod n");
    };
    run_test("inv(3,7)",   [&]{ check(3,7); });
    run_test("inv(5,11)",  [&]{ check(5,11); });
    run_test("inv(2,9)",   [&]{ check(2,9); });
    run_test("inv(1,13)",  [&]{ check(1,13); });
    run_test("inv(6,7)",   [&]{ check(6,7); });
    run_test("inv(10,101)",[&]{ check(10,101); });
    run_test("no inverse throws", []{
        bool threw = false;
        try { mod_inverse(2,4); } catch(...){ threw = true; }
        ASSERT(threw);
    });
}

// ─── Euler totient ────────────────────────────────────────────────────────────

void test_euler_totient() {
    std::cout << "\n=== Euler totient ===\n";
    struct Case { long long n, phi; };
    std::vector<Case> cases = {
        {1,1},{2,1},{3,2},{4,2},{5,4},{6,2},
        {7,6},{8,4},{9,6},{10,4},{12,4},{30,8},{100,40}
    };

    for (auto& c : cases) {
        run_test("def  φ("+std::to_string(c.n)+")=="+std::to_string(c.phi), [&]{
            ASSERT(euler_totient_definition(c.n) == c.phi);
        });
        run_test("fact φ("+std::to_string(c.n)+")=="+std::to_string(c.phi), [&]{
            ASSERT(euler_totient_factorization(c.n) == c.phi);
        });
        run_test("dft  φ("+std::to_string(c.n)+")=="+std::to_string(c.phi), [&]{
            ASSERT(euler_totient_dft(c.n) == c.phi);
        });
    }

    // Three methods agree for random values
    run_test("All 3 methods agree for n=1..50", []{
        for (long long n = 1; n <= 50; ++n) {
            long long d = euler_totient_definition(n);
            long long f = euler_totient_factorization(n);
            long long t = euler_totient_dft(n);
            ASSERT(d == f && f == t,
                "Methods disagree for n="+std::to_string(n));
        }
    });
}

// ─── Main ─────────────────────────────────────────────────────────────────────

int main() {
  
    test_gcd();
    test_extended_gcd();
    test_legendre();
    test_jacobi();
    test_mod_pow();
    test_mod_inverse();
    test_euler_totient();

    std::cout << "\n══════════════════════════════════════\n";
    std::cout << "Results: " << tests_passed << " / " << tests_run << " passed\n";
    return (tests_passed == tests_run) ? 0 : 1;
}