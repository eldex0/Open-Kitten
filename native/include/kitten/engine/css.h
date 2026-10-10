#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "kitten/engine/dom.h"

namespace kitten::engine {

struct Declarations final {
  std::optional<std::string> color;
  std::optional<std::string> background_color;
  std::optional<float> font_size_px;
  std::optional<float> margin_px;
  std::optional<float> padding_px;
  std::optional<bool> bold;
  std::optional<bool> italic;
  std::optional<bool> underline;
  std::optional<bool> hidden;
};

struct Rule final {
  std::string selector;
  Declarations declarations;
  int specificity{0};
  std::size_t source_order{0};
};

struct ComputedStyle final {
  std::string color{"#202124"};
  std::string background_color{"transparent"};
  float font_size_px{16.0F};
  float margin_px{0.0F};
  float padding_px{0.0F};
  bool bold{false};
  bool italic{false};
  bool underline{false};
  bool hidden{false};
};

[[nodiscard]] std::vector<Rule> parse_css(std::string_view source);
[[nodiscard]] ComputedStyle compute_style(const Node& node,
                                          const ComputedStyle& inherited,
                                          const std::vector<Rule>& rules);

}  // namespace kitten::engine
