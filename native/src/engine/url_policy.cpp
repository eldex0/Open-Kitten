#include "kitten/engine/url_policy.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace kitten::engine {
namespace {

bool starts_with_ascii_case_insensitive(std::string_view value,
                                        std::string_view prefix) {
  return value.size() >= prefix.size() &&
         std::equal(prefix.begin(), prefix.end(), value.begin(),
                    [](unsigned char left, unsigned char right) {
                      return std::tolower(left) == std::tolower(right);
                    });
}

bool valid_port(std::string_view port) {
  if (port.empty() || port.size() > 5) return false;
  unsigned int value = 0;
  for (const unsigned char character : port) {
    if (!std::isdigit(character)) return false;
    value = value * 10U + static_cast<unsigned int>(character - '0');
  }
  return value > 0 && value <= 65535;
}

std::string without_fragment(std::string_view url) {
  const auto fragment = url.find('#');
  return std::string(url.substr(0, fragment));
}

std::string origin_of(std::string_view url) {
  const auto scheme_end = url.find("://");
  if (scheme_end == std::string_view::npos) return {};
  const auto path = url.find_first_of("/?#", scheme_end + 3);
  return std::string(url.substr(0, path));
}

std::string normalize_path(std::string_view path) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  while (start <= path.size()) {
    const auto end = path.find('/', start);
    const std::string part(path.substr(start, end - start));
    if (part == "..") {
      if (!parts.empty()) parts.pop_back();
    } else if (!part.empty() && part != ".") {
      parts.push_back(part);
    }
    if (end == std::string_view::npos) break;
    start = end + 1;
  }
  std::string result = "/";
  for (std::size_t index = 0; index < parts.size(); ++index) {
    if (index > 0) result += '/';
    result += parts[index];
  }
  if (path.ends_with('/') && !result.ends_with('/')) result += '/';
  return result;
}

}  // namespace

bool is_safe_web_url(std::string_view url) {
  if (url.empty() || url.size() > 8192 ||
      url.find_first_of("\r\n\t ") != std::string_view::npos) return false;
  if (!starts_with_ascii_case_insensitive(url, "https://") &&
      !starts_with_ascii_case_insensitive(url, "http://")) return false;
  const auto separator = url.find("://");
  const auto host_start = separator + 3;
  const auto host_end = url.find_first_of("/?#", host_start);
  const auto authority = url.substr(host_start, host_end - host_start);
  if (authority.empty() || authority.find('@') != std::string_view::npos) return false;
  std::string_view host;
  if (authority.front() == '[') {
    const auto closing_bracket = authority.find(']');
    if (closing_bracket == std::string_view::npos) return false;
    host = authority.substr(0, closing_bracket + 1);
    const auto remainder = authority.substr(closing_bracket + 1);
    if (!remainder.empty() &&
        (remainder.front() != ':' || !valid_port(remainder.substr(1)))) return false;
  } else {
    const auto colon = authority.find(':');
    host = authority.substr(0, colon);
    if (colon != std::string_view::npos && !valid_port(authority.substr(colon + 1))) return false;
  }
  if (host.empty() || host == "[]" || host.front() == '.' || host.back() == '.') return false;
  if (host.find("..") != std::string_view::npos) return false;
  return std::all_of(host.begin(), host.end(), [](unsigned char character) {
    return std::isalnum(character) || character == '.' || character == '-' ||
           character == '[' || character == ']' || character == ':';
  });
}

std::string resolve_url(std::string_view base, std::string_view relative) {
  if (is_safe_web_url(relative)) return std::string(relative);
  if (!is_safe_web_url(base) || relative.empty() ||
      relative.find_first_of("\r\n\t ") != std::string_view::npos) return {};
  const auto scheme_end = base.find("://");
  const std::string scheme(base.substr(0, scheme_end));
  if (relative.starts_with("//")) {
    const std::string result = scheme + ":" + std::string(relative);
    return is_safe_web_url(result) ? result : std::string{};
  }
  if (relative.front() == '#') return without_fragment(base) + std::string(relative);
  if (relative.front() == '?') {
    std::string clean = without_fragment(base);
    const auto query = clean.find('?');
    if (query != std::string::npos) clean.resize(query);
    return clean + std::string(relative);
  }
  const std::string origin = origin_of(base);
  std::string relative_text(relative);
  std::string suffix;
  const auto suffix_start = relative_text.find_first_of("?#");
  if (suffix_start != std::string::npos) {
    suffix = relative_text.substr(suffix_start);
    relative_text.resize(suffix_start);
  }
  std::string path;
  if (relative_text.starts_with('/')) {
    path = relative_text;
  } else {
    const auto base_path_start = base.find('/', scheme_end + 3);
    std::string base_path = base_path_start == std::string_view::npos
        ? "/" : std::string(base.substr(base_path_start));
    const auto query = base_path.find_first_of("?#");
    if (query != std::string::npos) base_path.resize(query);
    const auto slash = base_path.rfind('/');
    path = base_path.substr(0, slash + 1) + relative_text;
  }
  const std::string result = origin + normalize_path(path) + suffix;
  return is_safe_web_url(result) ? result : std::string{};
}

}  // namespace kitten::engine
