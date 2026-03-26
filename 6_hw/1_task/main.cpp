#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include "math_service.h"
#include "probabilistic_tests.h"

// ─── Helpers ──────────────────────────────────────────────────────────────────

static void print_header(const std::string& title) {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  " << std::left << std::setw(57) << title << "║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
}

static void run_demo(const crypto::PrimalityTest& test,
                     const std::vector<long long>& candidates,
                     int rounds)
{
    using namespace crypto;
    std::cout << "\n[" << test.name() << "]  rounds=" << rounds
              << "  (error per round ≤ " << test.error_bound()
              << ", total error ≤ "
              << std::scientific << std::setprecision(2)
              << std::pow(test.error_bound(), rounds) << ")\n";
    std::cout << std::fixed;

    std::cout << std::string(60, '-') << "\n";
    std::cout << std::setw(12) << "n"
              << std::setw(15) << "Test result"
              << std::setw(15) << "Ground truth"
              << std::setw(12) << "Match?\n";
    std::cout << std::string(60, '-') << "\n";

    int correct = 0;
    for (long long n : candidates) {
        bool test_says = test.is_prime_prob(n, rounds);
        bool truth     = is_prime(n);
        bool match     = (test_says == truth);
        if (match) ++correct;

        std::cout << std::setw(12) << n
                  << std::setw(15) << (test_says ? "PRIME"     : "composite")
                  << std::setw(15) << (truth     ? "prime"     : "composite")
                  << std::setw(12) << (match     ? "✓"         : "✗ ERROR")
                  << "\n";
    }
    std::cout << std::string(60, '-') << "\n";
    std::cout << "Correct: " << correct << "/" << candidates.size() << "\n";
}

// ─── Main ─────────────────────────────────────────────────────────────────────

int main() {
    // ── Test candidates ──────────────────────────────────────────────────────
    std::vector<long long> candidates = {
        // small primes
        2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31,
        // small composites
        4, 6, 8, 9, 10, 15, 21, 25, 35, 49,
        // Carmichael numbers (fool naive Fermat test)
        561,      // = 3 × 11 × 17
        1105,     // = 5 × 13 × 17
        1729,     // = 7 × 13 × 19  (Hardy–Ramanujan number)
        2465,
        8911,
        // large primes
        7919,
        104729,
        15485863,
        // large composites
        7917,     // = 3 × 2639
        104723,   // = 223 × 469 + ...  composite
        // near-billion primes
        999999937LL,
        1000000007LL,
        // near-billion composites
        999999938LL,
        1000000008LL,
    };

    crypto::SolovayStrassenTest ss;
    crypto::MillerRabinTest     mr;

    // ── Demo with 20 rounds ──────────────────────────────────────────────────
    print_header("Probabilistic Primality Tests  —  Demo");
    run_demo(ss, candidates, 20);
    run_demo(mr, candidates, 20);

    // ── Error probability table ──────────────────────────────────────────────
    print_header("Error probability vs. number of rounds");
    std::cout << std::setw(10) << "rounds"
              << std::setw(24) << "SS  (≤ (1/2)^k)"
              << std::setw(24) << "MR  (≤ (1/4)^k)\n";
    std::cout << std::string(58, '-') << "\n";
    for (int k : {1, 5, 10, 20, 40, 64}) {
        std::cout << std::setw(10) << k
                  << std::setw(24) << std::scientific << std::setprecision(6)
                  << std::pow(0.5,  k)
                  << std::setw(24) << std::pow(0.25, k)
                  << "\n";
    }

    // ── Carmichael number highlight ──────────────────────────────────────────
    print_header("Carmichael numbers (fool Fermat, caught by SS & MR)");
    std::vector<long long> carmichael = {561, 1105, 1729, 2465, 8911, 41041};
    for (long long c : carmichael) {
        bool ss_says = ss.is_prime_prob(c, 20);
        bool mr_says = mr.is_prime_prob(c, 20);
        std::cout << "  n=" << std::setw(8) << c
                  << "  SS=" << (ss_says ? "prime?" : "composite")
                  << "  MR=" << (mr_says ? "prime?" : "composite")
                  << "  (truth: composite)\n";
    }

    return 0;
}