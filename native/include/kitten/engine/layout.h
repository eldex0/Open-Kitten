#pragma once

#include <functional>
#include <string>
#include <vector>
#include "kitten/engine/css.h"

namespace kitten::engine {

struct Size final { float width{0}; float height{0}; };

struct Fragment final {
  std::string text;
  std::string href;
  ComputedStyle style;
  float x{0};
  float y{0};
  float width{0};
  float height{0};
};

struct LayoutResult final {
  std::vector<Fragment> fragments;
  float content_height{0};
};

using TextMeasurer = std::function<Size(std::string_view, const ComputedStyle&)>;

[[nodiscard]] LayoutResult layout_document(const Document& document,
                                           const std::vector<Rule>& rules,
                                           float viewport_width,
                                           const TextMeasurer& measure);

}  // namespace kitten::engine
