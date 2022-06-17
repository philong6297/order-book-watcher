// Copyright 2022 Long Le Phi. All rights reserved.
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.

#include "order_book_feeds_manager.hpp"

#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <variant>
#include "instrument_feeds_worker.hpp"

namespace longlp {
  namespace {
    using Record = std::variant<OrderBookRecord, TradeRecord>;

    auto TestHelper(InstrumentFeedsWorker& worker,
                    const std::vector<std::string>& expected,
                    const std::vector<Record>& records) {
      auto index = 0U;
      for (const auto& record : records) {
        if (const auto* as_book = std::get_if<OrderBookRecord>(&record)) {
          EXPECT_EQ(worker.UpdateBookChangesUnsafe(
                      std::make_unique<OrderBookRecord>(*as_book)),
                    expected.at(index));
          ++index;
          continue;
        }

        if (const auto* as_trade = std::get_if<TradeRecord>(&record)) {
          EXPECT_TRUE(
            worker.RecordNewTrade(std::make_unique<TradeRecord>(*as_trade)));
          continue;
        }
      }
    }
  }   // namespace

  TEST(OrderBookFeedsManager, InvalidBook) {
    InstrumentFeedsWorker worker{};
    const std::vector<std::string> expected = {"update invalid book\n"};

    EXPECT_EQ(expected.front(), worker.UpdateBookChangesUnsafe(nullptr));
  }

  TEST(OrderBookFeedsManager, AddNewBook) {
    InstrumentFeedsWorker worker{};
    const std::vector<std::string> expected = {""};

    EXPECT_EQ(
      expected.front(),
      worker.UpdateBookChangesUnsafe(std::make_unique<OrderBookRecord>()));
  }

  TEST(OrderBookFeedsManager, PartialAgressive) {
    InstrumentFeedsWorker worker{};
    const std::vector<std::string> expected = {
      "",
      "AGGRESSIVE SELL 1460.00 @ 11.01\n",
    };

    // clang-format off
    std::vector<Record> records = {
      OrderBookRecord{{{1, 100, 11.11}, {1, 1380, 11.01}}, {{1, 860, 11.14}}},
      TradeRecord{100, 11.11},
      TradeRecord{1360, 11.01},
      OrderBookRecord{{{1, 20, 11.11}}, {{1, 860, 11.14}}},
    };
    // clang-format on

    TestHelper(worker, expected, records);
  }

  TEST(OrderBookFeedsManager, FullAgressive) {
    InstrumentFeedsWorker worker{};
    const std::vector<std::string> expected = {
      "",
      "AGGRESSIVE BUY 2540.00 @ 11.11\n",
    };

    // clang-format off
    std::vector<Record> records = {
      OrderBookRecord{
        {{1, 2780, 10.97}, {1, 2300, 10.82}},
        {{1, 620, 11.07}, {1, 1820, 11.08}, {1, 860, 11.14}}},
      TradeRecord{620, 11.07},
      TradeRecord{1820, 11.08},
      OrderBookRecord{
        {{1, 100, 11.11}, {1, 2780, 10.97}, {1, 2300, 10.82}},
        {{1, 860, 11.14}}},
    };
    // clang-format on

    TestHelper(worker, expected, records);
  }

  TEST(OrderBookFeedsManager, HomeTestExample) {
    InstrumentFeedsWorker worker{};
    const std::vector<std::string> expected = {
      "",
      "PASSIVE BUY 1300.00 @ 50.10\n",
      "PASSIVE BUY 900.00 @ 50.12\n",
      "PASSIVE SELL 1900.00 @ 50.14\n",
      "PASSIVE BUY 400.00 @ 50.12\n",
      "PASSIVE BUY 230.00 @ 50.12\n",
      "PASSIVE BUY 200.00 @ 50.13\n",
      "AGGRESSIVE SELL 420.00 @ 50.13\n",
      "PASSIVE SELL 330.00 @ 50.13\n",
      "PASSIVE SELL 105.00 @ 50.13\n",
      "PASSIVE SELL 590.00 @ 50.13\n",
      "AGGRESSIVE BUY 1000.00 @ 50.13\n",
    };

    // clang-format off
    std::vector<Record> records = {
      OrderBookRecord{                                  {},                 {}},
      OrderBookRecord{                  {{1, 1300, 50.10}},                 {}},
      OrderBookRecord{ {{1, 900, 50.12}, {1, 1300, 50.10}},                 {}},
      OrderBookRecord{ {{1, 900, 50.12}, {1, 1300, 50.10}}, {{1, 1900, 50.14}}},
      OrderBookRecord{{{2, 1300, 50.12}, {1, 1300, 50.10}}, {{1, 1900, 50.14}}},
      OrderBookRecord{{{3, 1530, 50.12}, {1, 1300, 50.10}}, {{1, 1900, 50.14}}},
      OrderBookRecord{{{1, 200, 50.13}, {3, 1530, 50.12}, {1,1300,50.10}}, {{1, 1900, 50.14}}},
      TradeRecord{200, 50.13},
      OrderBookRecord{{{3, 1530, 50.12}, {1,1300,50.10}}, {{1,220,50.13},{1, 1900, 50.14}}},
      OrderBookRecord{{{3, 1530, 50.12}, {1,1300,50.10}}, {{2,550,50.13},{1, 1900, 50.14}}},
      OrderBookRecord{{{3, 1530, 50.12}, {1,1300,50.10}}, {{3,655,50.13},{1, 1900, 50.14}}},
      OrderBookRecord{{{3, 1530, 50.12}, {1,1300,50.10}}, {{4,1245,50.13},{1, 1900, 50.14}}},
      TradeRecord{220, 50.13},
      TradeRecord{330, 50.13},
      TradeRecord{105, 50.13},
      TradeRecord{345, 50.13},
      OrderBookRecord{{{3, 1530, 50.12}, {1,1300,50.10}}, {{1,245,50.13},{1, 1900, 50.14}}},
    };
    // clang-format on

    TestHelper(worker, expected, records);
  }
}   // namespace longlp
