#include "kitten/engine/css.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <system_error>

namespace kitten::engine {
namespace {

std::string trim(std::string value) {
  const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); });
  const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c); }).base();
  return first < last ? std::string(first, last) : std::string{};
}

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

std::optional<float> pixels(std::string value) {
  value = lower(trim(std::move(value)));
  if (value.ends_with("px")) value.resize(value.size() - 2);
  float output = 0;
  const auto result = std::from_chars(value.data(), value.data() + value.size(), output);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) return std::nullopt;
  return std::clamp(output, 0.0F, 256.0F);
}

void assign(Declarations& declarations, std::string name, std::string value) {
  name = lower(trim(std::move(name)));
  value = trim(std::move(value));
  if (name == "color") declarations.color = value;
  else if (name == "background" || name == "background-color") declarations.background_color = value;
  else if (name == "font-size") declarations.font_size_px = pixels(value);
  else if (name == "margin") declarations.margin_px = pixels(value);
  else if (name == "padding") declarations.padding_px = pixels(value);
  else if (name == "font-weight") {
    const std::string normalized = lower(value);
    if (normalized == "bold" || normalized == "bolder") {
      declarations.bold = true;
    } else if (normalized == "normal" || normalized == "lighter") {
      declarations.bold = false;
    } else {
      int weight = 0;
      const auto parsed = std::from_chars(normalized.data(),
                                          normalized.data() + normalized.size(),
                                          weight);
      if (parsed.ec == std::errc{} &&
          parsed.ptr == normalized.data() + normalized.size()) {
        declarations.bold = weight >= 600;
      }
    }
  }
  else if (name == "font-style") declarations.italic = lower(value) == "italic";
  else if (name == "text-decoration") {
    const std::string normalized = lower(value);
    if (normalized == "none") declarations.underline = false;
    else if (normalized.find("underline") != std::string::npos) {
      declarations.underline = true;
    }
  }
  else if (name == "display") declarations.hidden = lower(value) == "none";
  else if (name == "visibility") declarations.hidden = lower(value) == "hidden";
}

Declarations parse_declarations(std::string_view source) {
  Declarations result;
  std::stringstream stream{std::string(source)};
  std::string declaration;
  while (std::getline(stream, declaration, ';')) {
    const auto colon = declaration.find(':');
    if (colon != std::string::npos) assign(result, declaration.substr(0, colon), declaration.substr(colon + 1));
  }
  return result;
}

bool matches(const Node& node, const std::string& selector) {
  if (node.type != NodeType::element || selector.empty()) return false;
  std::string tag;
  std::string id;
  std::vector<std::string> required_classes;
  std::size_t position = 0;
  while (position < selector.size() && selector[position] != '.' &&
         selector[position] != '#') {
    tag.push_back(selector[position++]);
  }
  while (position < selector.size()) {
    const char marker = selector[position++];
    const std::size_t start = position;
    while (position < selector.size() && selector[position] != '.' &&
           selector[position] != '#') ++position;
    if (start == position) return false;
    if (marker == '#') id = selector.substr(start, position - start);
    else required_classes.push_back(selector.substr(start, position - start));
  }
  if (!tag.empty() && tag != "*" && node.name != tag) return false;
  if (!id.empty() && lower(node.attribute("id")) != id) return false;
  if (required_classes.empty()) return true;
  std::vector<std::string> actual_classes;
  std::stringstream classes(node.attribute("class"));
  std::string item;
  while (classes >> item) actual_classes.push_back(lower(std::move(item)));
  return std::all_of(required_classes.begin(), required_classes.end(),
                     [&actual_classes](const std::string& required) {
    return std::find(actual_classes.begin(), actual_classes.end(), required) !=
           actual_classes.end();
  });
}

int specificity(const std::string& selector) {
  return static_cast<int>(std::count(selector.begin(), selector.end(), '#')) * 100 +
         static_cast<int>(std::count(selector.begin(), selector.end(), '.')) * 10 +
         ((!selector.empty() && selector.front() != '#' &&
           selector.front() != '.' && selector.front() != '*') ? 1 : 0);
}

void apply(ComputedStyle& style, const Declarations& values) {
  if (values.color) style.color = *values.color;
  if (values.background_color) style.background_color = *values.background_color;
  if (values.font_size_px) style.font_size_px = *values.font_size_px;
  if (values.margin_px) style.margin_px = *values.margin_px;
  if (values.padding_px) style.padding_px = *values.padding_px;
  if (values.bold) style.bold = *values.bold;
  if (values.italic) style.italic = *values.italic;
  if (values.underline) style.underline = *values.underline;
  if (values.hidden) style.hidden = *values.hidden;
}

}  // namespace

std::vector<Rule> parse_css(std::string_view source) {
  std::string cleaned(source);
  std::size_t comment = 0;
  while ((comment = cleaned.find("/*", comment)) != std::string::npos) {
    const auto end = cleaned.find("*/", comment + 2);
    cleaned.erase(comment, end == std::string::npos ? std::string::npos : end + 2 - comment);
  }
  source = cleaned;
  std::vector<Rule> rules;
  std::size_t position = 0;
  std::size_t order = 0;
  while (position < source.size()) {
    const auto opening = source.find('{', position);
    if (opening == std::string_view::npos) break;
    const auto closing = source.find('}', opening + 1);
    if (closing == std::string_view::npos) break;
    std::stringstream selectors(std::string(source.substr(position, opening - position)));
    std::string selector;
    const Declarations declarations = parse_declarations(source.substr(opening + 1, closing - opening - 1));
    while (std::getline(selectors, selector, ',')) {
      selector = lower(trim(std::move(selector)));
      const bool simple = !selector.empty() && selector.find_first_of(" >+~:[") == std::string::npos;
      if (simple) rules.push_back({selector, declarations,
                                   specificity(selector), order++});
    }
    position = closing + 1;
  }
  return rules;
}

ComputedStyle compute_style(const Node& node, const ComputedStyle& inherited,
                            const std::vector<Rule>& rules) {
  ComputedStyle style = inherited;
  style.margin_px = 0;
  style.padding_px = 0;
  if (node.name == "h1") { style.font_size_px = 32; style.bold = true; style.margin_px = 12; }
  else if (node.name == "h2") { style.font_size_px = 26; style.bold = true; style.margin_px = 10; }
  else if (node.name == "h3") { style.font_size_px = 22; style.bold = true; style.margin_px = 8; }
  else if (node.name == "b" || node.name == "strong") style.bold = true;
  else if (node.name == "i" || node.name == "em") style.italic = true;
  else if (node.name == "a") {
    style.color = "#2457c5";
    style.underline = true;
  }
  std::vector<const Rule*> matching;
  for (const Rule& rule : rules) if (matches(node, rule.selector)) matching.push_back(&rule);
  std::stable_sort(matching.begin(), matching.end(), [](const Rule* left, const Rule* right) {
    return left->specificity < right->specificity ||
           (left->specificity == right->specificity && left->source_order < right->source_order);
  });
  for (const Rule* rule : matching) apply(style, rule->declarations);
  apply(style, parse_declarations(node.attribute("style")));
  return style;
}

}  // namespace kitten::engine
