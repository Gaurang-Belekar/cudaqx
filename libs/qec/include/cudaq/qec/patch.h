/****************************************************************-*- C++ -*-****
 * Copyright (c) 2024 NVIDIA Corporation & Affiliates.                         *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/
#pragma once

#include "cudaq/qis/qubit_qis.h"

namespace cudaq::qec {

/// @brief Represents a logical qubit patch for quantum error correction
///
/// This type is for CUDA-Q kernel code only.
///
/// This structure defines a patch of qubits used in quantum error correction
/// codes. It consists of data qubits and ancilla qubits for X and Z stabilizer
/// measurements.
struct patch {
  /// @brief View of the data qubits in the patch
  cudaq::qview<> data;

  /// @brief View of the ancilla qubits used for X stabilizer measurements
  cudaq::qview<> ancx;

  /// @brief View of the ancilla qubits used for Z stabilizer measurements
  cudaq::qview<> ancz;
};

} // namespace cudaq::qec
