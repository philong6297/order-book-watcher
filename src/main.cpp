// Copyright 2022 Long Le Phi. All rights reserved.
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.

#include <fmt/core.h>
#include <fmt/format.h>
#include <chrono>
#include <thread>
#include "definitions.hpp"
#include "longlp_config.hpp"
#include "order_book_feeds_manager.hpp"

namespace {
  namespace chrono = std::chrono;
}   // namespace

auto main() -> int32_t {
  longlp::OrderBookFeedsManager manager{};

  fmt::print("Parsing input and setting up tasks\n");
  {
    auto start = chrono::high_resolution_clock::now();

    manager.InitFeedsAndGenerateTaskFlow(
      fmt::format("{}/input/input.json", longlp::config::data_dir),
      fmt::format("{}/output", longlp::config::data_dir));

    fmt::print("Execution time {}ms\n",
               chrono::duration_cast<chrono::milliseconds>(
                 chrono::high_resolution_clock::now() - start)
                 .count());
  }

  const auto threads = std::thread::hardware_concurrency();
  fmt::print("Running task flow with {} threads\n", threads);
  {
    auto start = chrono::high_resolution_clock::now();

    manager.RunTaskFlow(threads);

    fmt::print("Execution time {}ms\n",
               chrono::duration_cast<chrono::milliseconds>(
                 chrono::high_resolution_clock::now() - start)
                 .count());
  }
}
