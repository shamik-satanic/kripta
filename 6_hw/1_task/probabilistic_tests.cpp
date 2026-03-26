#include "probabilistic_tests.h"
#include "math_service.h"   // jacobi(), mod_pow()

namespace crypto {

// ─── Solovay–Strassen ─────────────────────────────────────────────────────────

bool SolovayStrassenTest::one_round(long long n, long long a) const {
  
    int J = jacobi(a, n);

    if (J == 0) return false;

    long long E = mod_pow(a, (n - 1) / 2, n);

    long long J_mod = ((long long)J % n + n) % n;

    return (E == J_mod);
}

// ─── Miller–Rabin ─────────────────────────────────────────────────────────────

bool MillerRabinTest::one_round(long long n, long long a) const {
   
    long long d = n - 1;
    int s = 0;
    while (d % 2 == 0) { d /= 2; ++s; }

    
    long long x = mod_pow(a, d, n);

    if (x == 1 || x == n - 1) return true; 

    for (int r = 1; r < s; ++r) {
        x = mod_pow(x, 2, n);
        if (x == n - 1) return true; 
    }

    return false; 
}

} // namespace crypto