/****************************************************************-*- C++ -*-****
 * Copyright (c) 2024 NVIDIA Corporation & Affiliates.                         *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/

#include "cudaq/solvers/operators/operator_pools/spin_complement_gsd.h"

using namespace cudaqx;

namespace cudaq::solvers {

inline cudaq::spin_op adag(std::size_t numQubits, std::size_t j) {
  cudaq::spin_op zprod(numQubits);
  for (std::size_t k = 0; k < j; k++)
    zprod *= cudaq::spin::z(k);
  return 0.5 * zprod *
         (cudaq::spin::x(j) - std::complex<double>{0, 1} * cudaq::spin::y(j));
}

inline cudaq::spin_op a(std::size_t numQubits, std::size_t j) {
  cudaq::spin_op zprod(numQubits);
  for (std::size_t k = 0; k < j; k++)
    zprod *= cudaq::spin::z(k);
  return 0.5 * zprod *
         (cudaq::spin::x(j) + std::complex<double>{0, 1} * cudaq::spin::y(j));
}

std::vector<cudaq::spin_op>
spin_complement_gsd::generate(const heterogeneous_map &config) const {
  auto numOrbitals = config.get<std::size_t>({"num-orbitals", "num_orbitals"});

  std::vector<cudaq::spin_op> pool;
  auto numQubits = 2 * numOrbitals;
  std::vector<int> alphaOrbs, betaOrbs;
  for (auto i : cudaq::range(numOrbitals)) {
    alphaOrbs.push_back(2 * i);
    betaOrbs.push_back(alphaOrbs.back() + 1);
  }

  for (auto p : alphaOrbs) {
    for (auto q : alphaOrbs) {
      if (p >= q)
        continue;
      auto oneElectron = adag(numQubits, q) * a(numQubits, p) -
                         adag(numQubits, p) * a(numQubits, q);
      oneElectron += adag(numQubits, q + 1) * a(numQubits, p + 1) -
                     adag(numQubits, p + 1) * a(numQubits, q + 1);

      std::unordered_map<cudaq::spin_op::spin_op_term, std::complex<double>>
          terms;
      oneElectron.for_each_term([&](cudaq::spin_op &term) {
        auto coeff = term.get_coefficient();
        if (std::fabs(coeff.real()) < 1e-12 && std::fabs(coeff.imag()) < 1e-12)
          return;

        if (std::fabs(coeff.real()) < 1e-12)
          terms.insert({term.get_raw_data().first[0],
                        std::complex<double>{0., coeff.imag()}});
      });

      if (!terms.empty())
        pool.emplace_back(terms);
    }
  }

  int pq = 0;
  for (auto p : alphaOrbs) {
    for (auto q : alphaOrbs) {
      if (p > q)
        continue;

      int rs = 0;
      for (auto r : alphaOrbs) {
        for (auto s : alphaOrbs) {
          if (r > s)
            continue;
          if (pq < rs)
            continue;

          auto twoElectron = adag(numQubits, r) * a(numQubits, p) *
                                 adag(numQubits, s) * a(numQubits, q) -
                             adag(numQubits, q) * a(numQubits, s) *
                                 adag(numQubits, p) * a(numQubits, r);
          twoElectron += adag(numQubits, r + 1) * a(numQubits, p + 1) *
                             adag(numQubits, s + 1) * a(numQubits, q + 1) -
                         adag(numQubits, q + 1) * a(numQubits, s + 1) *
                             adag(numQubits, p + 1) * a(numQubits, r + 1);

          std::unordered_map<cudaq::spin_op::spin_op_term, std::complex<double>>
              terms;
          twoElectron.for_each_term([&](cudaq::spin_op &term) {
            auto coeff = term.get_coefficient();
            if (std::fabs(coeff.real()) < 1e-12 &&
                std::fabs(coeff.imag()) < 1e-12)
              return;

            if (std::fabs(coeff.real()) < 1e-12)
              terms.insert({term.get_raw_data().first[0],
                            std::complex<double>{0., coeff.imag()}});
          });

          if (!terms.empty())
            pool.push_back(terms);
          rs++;
        }
      }
      pq++;
    }
  }

  pq = 0;
  for (auto p : alphaOrbs) {
    for (auto q : betaOrbs) {

      int rs = 0;
      for (auto r : alphaOrbs) {
        for (auto s : betaOrbs) {

          if (pq < rs)
            continue;

          auto twoElectron = adag(numQubits, r) * a(numQubits, p) *
                                 adag(numQubits, s) * a(numQubits, q) -
                             adag(numQubits, q) * a(numQubits, s) *
                                 adag(numQubits, p) * a(numQubits, r);
          if (p > q)
            continue;

          twoElectron += adag(numQubits, s - 1) * a(numQubits, q - 1) *
                             adag(numQubits, r + 1) * a(numQubits, p + 1) -
                         adag(numQubits, p + 1) * a(numQubits, r + 1) *
                             adag(numQubits, q - 1) * a(numQubits, s - 1);
          std::unordered_map<cudaq::spin_op::spin_op_term, std::complex<double>>
              terms;
          twoElectron.for_each_term([&](cudaq::spin_op &term) {
            auto coeff = term.get_coefficient();
            if (std::fabs(coeff.real()) < 1e-12 &&
                std::fabs(coeff.imag()) < 1e-12)
              return;

            if (std::fabs(coeff.real()) < 1e-12)
              terms.insert({term.get_raw_data().first[0],
                            std::complex<double>{0., coeff.imag()}});
          });
          if (!terms.empty())
            pool.push_back(terms);
          rs++;
        }
      }
      pq++;
    }
  }

  return pool;
}

} // namespace cudaq::solvers
