#pragma once
#include "primality_test.h"

namespace crypto {



class SolovayStrassenTest : public PrimalityTest {
public:
    std::string name()         const override { return "Solovay–Strassen"; }
    double      error_bound()  const override { return 0.5; }

protected:
    bool one_round(long long n, long long a) const override;
};



class MillerRabinTest : public PrimalityTest {
public:
    std::string name()         const override { return "Miller–Rabin"; }
    double      error_bound()  const override { return 0.25; }

protected:
    bool one_round(long long n, long long a) const override;
};

} // namespace crypto