// Copyright 2022 Long Le Phi. All rights reserved.
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.

#ifndef ORDER_BOOK_FEEDS_MANAGER_HPP_
#define ORDER_BOOK_FEEDS_MANAGER_HPP_

#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <taskflow/taskflow.hpp>
#include "instrument_feeds_worker.hpp"

namespace longlp {
  // The manager which has responsibility for parsing the market feeds (JSON
  // lines formatted) and generating parallel and heterogeneous tasks for high
  // performance analysis.
  class OrderBookFeedsManager {
   public:
    // parsing json feeds then setup the task flow.
    void InitFeedsAndGenerateTaskFlow(const std::string& json_file,
                                      std::string_view out_dir);

    // parallel run the analysis with assigned number of threads.
    // It should be called after InitFeedsAndGenerateTaskFlow
    void RunTaskFlow(size_t threads);

   private:
    // assign the corresponded worker for analyzing the order book changes.
    // Marked as unsafe because of thread safety awareness.
    void UpdateBookChangesUnsafe(const std::string& symbol,
                                 std::unique_ptr<OrderBookRecord> new_book);

    // assign the corresponded worker for caching the trade records.
    // Marked as unsafe because of thread safety awareness.
    void RecordNewTradeUnsafe(const std::string& symbol,
                              std::unique_ptr<TradeRecord> new_trade);

    // Manage worker by instrument symbol. Lazy initialzation
    using WorkerList =
      std::map<std::string /* symbol */, InstrumentFeedsWorker>;

    // Manage file writer by instrument symbol. Lazy initialzation
    using WriterList = std::map<std::string /* symbol */, std::ofstream>;

    std::unique_ptr<tf::Executor> executor_{nullptr};
    std::unique_ptr<tf::Taskflow> flow_{nullptr};

    std::unique_ptr<WorkerList> workers_{nullptr};
    std::unique_ptr<WriterList> writers_{nullptr};
  };
}   // namespace longlp

#endif   // ORDER_BOOK_FEEDS_MANAGER_HPP_
