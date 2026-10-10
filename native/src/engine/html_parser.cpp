#include "kitten/engine/html_parser.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>

namespace kitten::engine {
namespace {

constexpr std::size_t kMaximumNodes = 50000;
constexpr std::size_t kMaximumTreeDepth = 256;

std::string lower(std::string_view value) {
  std::string result(value);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char item) { return static_cast<char>(std::tolower(item)); });
  return result;
}

void append_utf8(std::string& output, std::uint32_t code_point) {
  if (code_point <= 0x7f) {
    output.push_back(static_cast<char>(code_point));
  } else if (code_point <= 0x7ff) {
    output.push_back(static_cast<char>(0xc0 | (code_point >> 6)));
    output.push_back(static_cast<char>(0x80 | (code_point & 0x3f)));
  } else if (code_point <= 0xffff) {
    output.push_back(static_cast<char>(0xe0 | (code_point >> 12)));
    output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3f)));
    output.push_back(static_cast<char>(0x80 | (code_point & 0x3f)));
  } else {
    output.push_back(static_cast<char>(0xf0 | (code_point >> 18)));
    output.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3f)));
    output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3f)));
    output.push_back(static_cast<char>(0x80 | (code_point & 0x3f)));
  }
}

std::optional<std::uint32_t> numeric_entity(std::string_view entity) {
  if (entity.size() < 2 || entity.front() != '#') return std::nullopt;
  std::size_t position = 1;
  unsigned int base = 10;
  if (position < entity.size() && (entity[position] == 'x' || entity[position] == 'X')) {
    base = 16;
    ++position;
  }
  if (position == entity.size()) return std::nullopt;
  std::uint32_t value = 0;
  for (; position < entity.size(); ++position) {
    const unsigned char character = entity[position];
    unsigned int digit = 0;
    if (character >= '0' && character <= '9') digit = character - '0';
    else if (base == 16 && character >= 'a' && character <= 'f') digit = character - 'a' + 10;
    else if (base == 16 && character >= 'A' && character <= 'F') digit = character - 'A' + 10;
    else return std::nullopt;
    if (value > (0x10ffffU - digit) / base) return std::nullopt;
    value = value * base + digit;
  }
  if (value == 0 || value > 0x10ffffU || (value >= 0xd800U && value <= 0xdfffU))
    return std::nullopt;
  return value;
}

std::string decode_entities(std::string_view input) {
  std::string output;
  output.reserve(input.size());
  std::size_t position = 0;
  while (position < input.size()) {
    if (input[position] != '&') {
      output.push_back(input[position++]);
      continue;
    }
    const auto end = input.find(';', position + 1);
    if (end == std::string_view::npos || end - position > 16) {
      output.push_back(input[position++]);
      continue;
    }
    const std::string_view entity = input.substr(position + 1, end - position - 1);
    if (entity == "amp") output.push_back('&');
    else if (entity == "lt") output.push_back('<');
    else if (entity == "gt") output.push_back('>');
    else if (entity == "quot") output.push_back('"');
    else if (entity == "apos") output.push_back('\'');
    else if (entity == "nbsp") output.push_back(' ');
    else if (const auto numeric = numeric_entity(entity)) append_utf8(output, *numeric);
    else {
      output.append(input.substr(position, end - position + 1));
      position = end + 1;
      continue;
    }
    position = end + 1;
  }
  return output;
}

bool append_text(Node& parent, std::string value, std::size_t& node_count,
                 bool decode = true) {
  if (value.empty()) return true;
  if (node_count >= kMaximumNodes) return false;
  auto node = std::make_unique<Node>();
  node->type = NodeType::text;
  node->text = decode ? decode_entities(value) : std::move(value);
  node->parent = &parent;
  parent.children.push_back(std::move(node));
  ++node_count;
  return true;
}

std::size_t tag_end(std::string_view source, std::size_t offset) {
  char quote = 0;
  for (std::size_t index = offset; index < source.size(); ++index) {
    const char current = source[index];
    if (quote != 0) {
      if (current == quote) quote = 0;
    } else if (current == '\'' || current == '"') {
      quote = current;
    } else if (current == '>') {
      return index;
    }
  }
  return std::string_view::npos;
}

void parse_attributes(std::string_view input, Node& node) {
  std::size_t position = 0;
  while (position < input.size()) {
    while (position < input.size() && std::isspace(static_cast<unsigned char>(input[position]))) ++position;
    const std::size_t name_start = position;
    while (position < input.size() && !std::isspace(static_cast<unsigned char>(input[position])) &&
           input[position] != '=') ++position;
    if (position == name_start) break;
    std::string name = lower(input.substr(name_start, position - name_start));
    while (position < input.size() && std::isspace(static_cast<unsigned char>(input[position]))) ++position;
    std::string value;
    if (position < input.size() && input[position] == '=') {
      ++position;
      while (position < input.size() && std::isspace(static_cast<unsigned char>(input[position]))) ++position;
      if (position < input.size() && (input[position] == '\'' || input[position] == '"')) {
        const char quote = input[position++];
        const std::size_t start = position;
        while (position < input.size() && input[position] != quote) ++position;
        value = std::string(input.substr(start, position - start));
        if (position < input.size()) ++position;
      } else {
        const std::size_t start = position;
        while (position < input.size() && !std::isspace(static_cast<unsigned char>(input[position]))) ++position;
        value = std::string(input.substr(start, position - start));
      }
    }
    node.attributes.insert_or_assign(std::move(name), decode_entities(value));
  }
}

}  // namespace

Document parse_html(std::string_view source) {
  Document document;
  document.root = std::make_unique<Node>();
  document.root->type = NodeType::document;
  document.root->name = "document";
  std::vector<Node*> stack{document.root.get()};
  const std::string lowered_source = lower(source);
  const std::unordered_set<std::string> void_tags{
      "area", "base", "br", "col", "embed", "hr", "img", "input",
      "link", "meta", "source", "track", "wbr"};
  std::size_t position = 0;
  std::size_t node_count = 1;
  while (position < source.size()) {
    if ((stack.back()->name == "script" || stack.back()->name == "style") && stack.size() > 1) {
      const std::string closing = "</" + stack.back()->name;
      const std::size_t end = lowered_source.find(closing, position);
      if (end == std::string::npos) {
        append_text(*stack.back(), std::string(source.substr(position)), node_count, false);
        break;
      }
      if (!append_text(*stack.back(), std::string(source.substr(position, end - position)),
                       node_count, false)) break;
      position = end;
    }
    const std::size_t opening = source.find('<', position);
    if (opening == std::string_view::npos) {
      append_text(*stack.back(), std::string(source.substr(position)), node_count);
      break;
    }
    if (!append_text(*stack.back(), std::string(source.substr(position, opening - position)),
                     node_count)) break;
    if (source.substr(opening, 4) == "<!--") {
      const std::size_t end = source.find("-->", opening + 4);
      position = end == std::string_view::npos ? source.size() : end + 3;
      continue;
    }
    const std::size_t end = tag_end(source, opening + 1);
    if (end == std::string_view::npos) break;
    std::string inside(source.substr(opening + 1, end - opening - 1));
    position = end + 1;
    while (!inside.empty() && std::isspace(static_cast<unsigned char>(inside.front()))) inside.erase(inside.begin());
    while (!inside.empty() && std::isspace(static_cast<unsigned char>(inside.back()))) inside.pop_back();
    if (inside.empty() || inside.front() == '!') continue;
    if (inside.front() == '/') {
      std::string name = lower(std::string_view(inside).substr(1));
      const auto space = name.find_first_of(" \t\r\n");
      if (space != std::string::npos) name.resize(space);
      for (std::size_t index = stack.size(); index-- > 1;) {
        if (stack[index]->name == name) {
          stack.resize(index);
          break;
        }
      }
      continue;
    }
    const bool self_closing = inside.back() == '/';
    if (self_closing) inside.pop_back();
    const auto separator = inside.find_first_of(" \t\r\n");
    const std::string name = lower(std::string_view(inside).substr(0, separator));
    if (name.empty()) continue;
    if (node_count >= kMaximumNodes) break;
    auto node = std::make_unique<Node>();
    node->type = NodeType::element;
    node->name = name;
    node->parent = stack.back();
    if (separator != std::string::npos) parse_attributes(std::string_view(inside).substr(separator + 1), *node);
    Node* raw = node.get();
    stack.back()->children.push_back(std::move(node));
    ++node_count;
    if (!self_closing && !void_tags.contains(name) && stack.size() < kMaximumTreeDepth)
      stack.push_back(raw);
  }

  std::vector<const Node*> pending{document.root.get()};
  while (!pending.empty()) {
    const Node* node = pending.back();
    pending.pop_back();
    if (node->name == "title") {
      document.title = text_content(*node);
      break;
    }
    for (const auto& child : node->children) pending.push_back(child.get());
  }
  return document;
}

std::string text_content(const Node& node) {
  if (node.type == NodeType::text) return node.text;
  std::string result;
  for (const auto& child : node.children) result += text_content(*child);
  return result;
}

}  // namespace kitten::engine
