#pragma once

#include <string>
#include <string_view>

namespace kitten::engine {

[[nodiscard]] bool is_safe_web_url(std::string_view url);
[[nodiscard]] std::string resolve_url(std::string_view base, std::string_view relative);

}  // namespace kitten::engine
