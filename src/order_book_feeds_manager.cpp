// Copyright 2022 Long Le Phi. All rights reserved.
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.

#include "order_book_feeds_manager.hpp"

#include <fmt/core.h>
#include <fmt/format.h>
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <taskflow/taskflow.hpp>

namespace longlp {
  namespace {
    namespace json = nlohmann;

  }   // namespace

  void OrderBookFeedsManager::InitFeedsAndGenerateTaskFlow(
    const std::string& json_file,
    std::string_view out_dir) {
    std::ifstream opener(json_file);
    if (!opener.is_open()) {
      fmt::print("Cannot open {}", json_file);
      return;
    }

    workers_ = std::make_unique<WorkerList>();
    writers_ = std::make_unique<WriterList>();
    flow_    = std::make_unique<tf::Taskflow>();

    // record the previous task for setting up the flow graph.
    std::map<std::string /* symbol */, tf::Task> prev_task{};

    std::string line{};
    for (auto i = 1; std::getline(opener, line); ++i) {
      const auto& record_json = json::json::parse(line, nullptr, false);

      if (record_json.is_discarded()) {
        fmt::print("parse {} error at line {}", json_file, i);
        return;
      }

      // Detected a order book record
      if (record_json.contains("book")) {
        const auto& book_json = record_json["book"];
        const auto& symbol    = book_json["symbol"].get<std::string>();

        OrderBookRecord book{};
        for (const auto& bid : book_json["bid"]) {
          book.bids.emplace_back(Level{bid["count"].get<double>(),
                                       bid["quantity"].get<double>(),
                                       bid["price"].get<double>()});
        }

        for (const auto& ask : book_json["ask"]) {
          book.asks.emplace_back(Level{ask["count"].get<double>(),
                                       ask["quantity"].get<double>(),
                                       ask["price"].get<double>()});
        }

        if (workers_->find(symbol) == workers_->end()) {
          // Whenever detected a new symbol, we should create a new worker and
          // writer synchronously for thread safety in data writting.
          auto writer_it = writers_->try_emplace(symbol, std::ofstream{}).first;
          writer_it->second.open(fmt::format("{out_dir}/{symbol}.txt",
                                             fmt::arg("out_dir", out_dir),
                                             fmt::arg("symbol", symbol)));
          UpdateBookChangesUnsafe(symbol,
                                  std::make_unique<OrderBookRecord>(book));
        }
        else {
          // Setup the task and create the dependency on the previous one with
          // the same symbol
          auto task = flow_->emplace([this, symbol, book] {
            UpdateBookChangesUnsafe(symbol,
                                    std::make_unique<OrderBookRecord>(book));
          });

          // the new task should be run after the previous one
          if (auto prev_it = prev_task.find(symbol);
              prev_it != prev_task.end()) {
            task.succeed(prev_it->second);
          }
          prev_task[symbol] = task;
        }

        continue;
      }

      // Detected a trade record
      if (record_json.contains("trade")) {
        const auto& trade_json = record_json["trade"];
        const auto& symbol     = trade_json["symbol"].get<std::string>();

        TradeRecord trade{};
        trade.price    = trade_json["price"].get<double>();
        trade.quantity = trade_json["quantity"].get<double>();

        // Setup the task and create the dependency on the previous one with
        // the same symbol
        auto task = flow_->emplace([this, symbol, trade] {
          RecordNewTradeUnsafe(symbol, std::make_unique<TradeRecord>(trade));
        });

        // the new task should be run after the previous one
        if (auto prev_it = prev_task.find(symbol); prev_it != prev_task.end()) {
          task.succeed(prev_it->second);
        }
        prev_task[symbol] = task;

        continue;
      }

      fmt::print("Invalid json line {}", i);
      return;
    }
  }

  void OrderBookFeedsManager::RunTaskFlow(const size_t threads) {
    if (flow_ == nullptr || flow_->empty()) {
      fmt::print("No flow task declared\n");
      return;
    }

    executor_ = std::make_unique<tf::Executor>(threads);
    executor_->run(*flow_).wait();
  }

  void OrderBookFeedsManager::UpdateBookChangesUnsafe(
    const std::string& symbol,
    std::unique_ptr<OrderBookRecord> new_book) {
    auto worker_it = workers_->try_emplace(symbol).first;

    writers_->at(symbol) << worker_it->second.UpdateBookChangesUnsafe(
      std::move(new_book));
  }

  void OrderBookFeedsManager::RecordNewTradeUnsafe(
    const std::string& symbol,
    std::unique_ptr<TradeRecord> new_trade) {
    if (auto worker_it = workers_->find(symbol); worker_it != workers_->end()) {
      worker_it->second.RecordNewTrade(std::move(new_trade));
    }
    else {
      fmt::print("There is no book recorded with symbol {}\n", symbol);
    }
  }

}   // namespace longlp
