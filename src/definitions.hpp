// Copyright 2022 Long Le Phi. All rights reserved.
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.

#ifndef DEFINITIONS_HPP_
#define DEFINITIONS_HPP_

#include <cstdint>
#include <string_view>
#include <variant>
#include <vector>

namespace longlp {

  // A row in a order book side.
  struct Level {
    double count{0};
    double quantity{0};
    double price{0};
  };

  using SideList = std::vector<Level>;

  struct OrderBookRecord {
    SideList bids;
    SideList asks;
  };

  struct TradeRecord {
    double quantity;
    double price;
  };

}   // namespace longlp

#endif   // DEFINITIONS_HPP_
