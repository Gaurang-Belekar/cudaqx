/*******************************************************************************
 * Copyright (c) 2024 NVIDIA Corporation & Affiliates.                         *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/
#include "cuda-qx/core/tear_down.h"
#include <memory>
#include <stdio.h>
#include <vector>

namespace cudaqx {
class tear_down_holder {
public:
  std::vector<std::unique_ptr<tear_down>> services;
  ~tear_down_holder() {
    for (auto &s : services)
      s->runTearDown();
  }
};

static tear_down_holder holder;
void scheduleTearDown(std::unique_ptr<tear_down> service) {
  holder.services.emplace_back(std::move(service));
}
} // namespace cudaqx
