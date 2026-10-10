#pragma once

#include <string_view>
#include "kitten/engine/dom.h"

namespace kitten::engine {

[[nodiscard]] Document parse_html(std::string_view source);
[[nodiscard]] std::string text_content(const Node& node);

}  // namespace kitten::engine
