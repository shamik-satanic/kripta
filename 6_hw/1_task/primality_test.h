#pragma once
#include <string>
#include <cstdint>

namespace crypto {


class PrimalityTest {
public:
    virtual ~PrimalityTest() = default;

    virtual std::string name() const = 0;

  
    bool is_prime_prob(long long n, int rounds = 20) const;

    virtual double error_bound() const = 0;

protected:

    virtual bool one_round(long long n, long long a) const = 0;

    virtual long long pick_witness(long long n, int round_index) const;

private:
  
    static bool trivial_check(long long n);
};

} // namespace crypto