// Copyright 2022 Long Le Phi. All rights reserved.
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.

#ifndef INSTRUMENT_FEEDS_WORKER_HPP_
#define INSTRUMENT_FEEDS_WORKER_HPP_

#include <deque>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include "definitions.hpp"

namespace longlp {
  // A Worker who analyzes the order book and trade messages of a single
  // instrument. It is designed to only keep the previous order book record.
  // Thus minimizing the memory usage.
  class InstrumentFeedsWorker {
   public:
    // Compares changes with previous logged order book. Return the formatted
    // and classified orders (Intention Side Quantity @ Price)
    auto UpdateBookChangesUnsafe(std::unique_ptr<OrderBookRecord> new_book)
      -> std::string;

    // Log the trades between order book records.
    auto RecordNewTrade(std::unique_ptr<TradeRecord> new_trade) -> bool;

   private:
    std::unique_ptr<OrderBookRecord> old_book_{nullptr};

    // trade records are guaranteed in sorted order. Thus using a queue will
    // help to fast access the max and min prices without losing info.
    std::unique_ptr<std::deque<TradeRecord>> trades_{nullptr};
  };
}   // namespace longlp

#endif   // INSTRUMENT_FEEDS_WORKER_HPP_
