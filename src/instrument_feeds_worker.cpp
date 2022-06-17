// Copyright 2022 Long Le Phi. All rights reserved.
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.

#include "instrument_feeds_worker.hpp"

#include <fmt/format.h>
#include <array>
#include <cmath>
#include <numeric>
#include <sstream>

namespace longlp {
  namespace {
    constexpr std::array<std::string_view, 3> kIntentionStrings = {
      "CANCEL",
      "PASSIVE",
      "AGGRESSIVE"};
    enum class Intention : std::size_t {
      kCancelled  = 0,
      kPassive    = 1,
      kAggressive = 2,
    };

    constexpr std::array<std::string_view, 2> kSideStrings = {"BUY", "SELL"};
    enum class Side : std::size_t { kBuy = 0, kSell = 1 };

    // fast floating point comparision
    auto IsSame(const double left, const double right) -> bool {
      return std::fabs(left - right) < std::numeric_limits<double>::epsilon();
    }

    // generate formatted orders.
    auto GenerateStatus(const Intention intention,
                        const Side side,
                        const double quantity,
                        const double price) {
      static constexpr std::string_view format =
        "{intention} {side} {quantity:.2f} @ {price:.2f}\n";
      return fmt::format(
        format,
        fmt::arg("intention",
                 kIntentionStrings.at(static_cast<size_t>(intention))),
        fmt::arg("side", kSideStrings.at(static_cast<size_t>(side))),
        fmt::arg("quantity", quantity),
        fmt::arg("price", price));
    };

    // Compare the changes of a side between old and new order book records.
    template <Side side>
    auto CompareSideListChange(const SideList& old_list,
                               const SideList& new_list,
                               std::ostream& writer) {
      auto old_it = old_list.begin();
      auto new_it = new_list.begin();

      using is_new_order_placed_comparer =
        std::conditional_t<side == Side::kBuy,
                           std::greater<double>,
                           std::less<double>>;
      const is_new_order_placed_comparer is_new_order_placed{};

      for (; old_it != old_list.end() || new_it != new_list.end();) {
        if (old_it == old_list.end()) {
          // new order
          writer << GenerateStatus(Intention::kPassive,
                                   side,
                                   new_it->quantity,
                                   new_it->price);
          ++new_it;
          continue;
        }

        if (new_it == new_list.end()) {
          // cancel order
          writer << GenerateStatus(Intention::kCancelled,
                                   side,
                                   old_it->quantity,
                                   old_it->price);
          ++old_it;
          continue;
        }

        if (IsSame(old_it->price, new_it->price)) {
          // Log if there is a change in quantity
          if (const auto quant_diff = new_it->quantity - old_it->quantity;
              !IsSame(quant_diff, 0.0)) {
            writer << GenerateStatus(
              quant_diff > 0 ? Intention::kPassive : Intention::kCancelled,
              side,
              quant_diff,
              new_it->price);
          }

          ++new_it;
          ++old_it;
          continue;
        }

        // There is a new order which changes the positions in order book
        if (is_new_order_placed(new_it->price, old_it->price)) {
          writer << GenerateStatus(Intention::kPassive,
                                   side,
                                   new_it->quantity,
                                   new_it->price);
          ++new_it;
          continue;
        }

        // There is a cancel order which changes the positions in order
        // book
        writer << GenerateStatus(Intention::kCancelled,
                                 side,
                                 old_it->quantity,
                                 old_it->price);
        ++old_it;
      }
    }
  }   // namespace

  auto InstrumentFeedsWorker::UpdateBookChangesUnsafe(
    std::unique_ptr<OrderBookRecord> new_book) -> std::string {
    std::ostringstream result;
    if (new_book == nullptr) {
      result << "update invalid book\n";
      return result.str();
    }

    // Since there is no previous book, update and early exit.
    if (old_book_ == nullptr) {
      old_book_ = std::move(new_book);
      return result.str();
    }

    // The case where there are no trades between order book records.
    if (trades_ == nullptr || trades_->empty()) {
      CompareSideListChange<Side::kBuy>(old_book_->bids,
                                        new_book->bids,
                                        result);
      CompareSideListChange<Side::kSell>(old_book_->asks,
                                         new_book->asks,
                                         result);

      old_book_ = std::move(new_book);
      return result.str();
    }

    const auto total_trade_quantity =
      std::accumulate(trades_->begin(),
                      trades_->end(),
                      0.0,
                      [](const double quantity, const TradeRecord& trade) {
                        return quantity + trade.quantity;
                      });

    auto quantity    = total_trade_quantity;
    auto order_price = trades_->back().price;

    // aggressive sell
    // trades are guaranteed in price-descending order
    //
    // first trade (largest trade price) should <= old book's best bid
    // (largest buy price)
    if (!old_book_->bids.empty() &&
        trades_->front().price <= old_book_->bids.front().price) {
      if (!new_book->asks.empty() &&
          trades_->back().price >= new_book->asks.front().price) {
        order_price = new_book->asks.front().price;
        quantity += new_book->asks.front().quantity;
      }

      result << GenerateStatus(Intention::kAggressive,
                               Side::kSell,
                               quantity,
                               order_price);
    }
    // aggressive buy
    // trades are guaranteed in price-ascending order
    //
    // first trade (smallest trade price) should >= old book's best ask
    // (smallest sell price)
    else if (!old_book_->asks.empty() &&
             trades_->front().price >= old_book_->asks.front().price) {
      if (!new_book->bids.empty() &&
          trades_->back().price <= new_book->bids.front().price) {
        order_price = new_book->bids.front().price;
        quantity += new_book->bids.front().quantity;
      }
      result << GenerateStatus(Intention::kAggressive,
                               Side::kBuy,
                               quantity,
                               order_price);
    }
    // Should not reach here
    else {
      result << "invalid trade\n";
    }

    // Always update new states to prepare for the next call
    trades_->clear();
    old_book_ = std::move(new_book);

    return result.str();
  }

  auto InstrumentFeedsWorker::RecordNewTrade(
    std::unique_ptr<TradeRecord> new_trade) -> bool {
    if (new_trade == nullptr) {
      return false;
    }

    if (trades_ == nullptr) {
      trades_ = std::make_unique<std::deque<TradeRecord>>();
    }

    if (trades_->empty() || !IsSame(trades_->back().price, new_trade->price)) {
      trades_->push_back(*new_trade);
      return true;
    }

    // Accumulate the trades which have the same price because there is no
    // requirement to analyze each record.
    trades_->back().quantity += new_trade->quantity;
    return true;
  }

}   // namespace longlp
