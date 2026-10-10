#include "kitten/engine/layout.h"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <unordered_set>

namespace kitten::engine {
namespace {

const std::unordered_set<std::string> kBlocks{
    "address", "article", "aside", "blockquote", "body", "div", "footer",
    "form", "h1", "h2", "h3", "header", "hr", "li", "main", "nav",
    "ol", "p", "pre", "section", "table", "ul"};
const std::unordered_set<std::string> kHidden{"head", "meta", "script", "style", "title"};

struct Cursor final {
  float x{16.0F};
  float y{16.0F};
  float line_height{20.0F};
};

void new_line(Cursor& cursor, float left, float spacing = 0.0F) {
  cursor.x = left;
  cursor.y += cursor.line_height + spacing;
  cursor.line_height = 20.0F;
}

std::vector<std::string> words(std::string_view source) {
  std::vector<std::string> output;
  std::string current;
  bool pending_space = false;
  for (const unsigned char character : source) {
    if (std::isspace(character)) {
      pending_space = !current.empty() || !output.empty();
      continue;
    }
    if (pending_space && !current.empty()) {
      output.push_back(std::move(current));
      current.clear();
    }
    current.push_back(static_cast<char>(character));
    pending_space = false;
  }
  if (!current.empty()) output.push_back(std::move(current));
  return output;
}

void layout_node(const Node& node, const ComputedStyle& inherited,
                 const std::vector<Rule>& rules, float viewport_width,
                 const TextMeasurer& measure, const std::string& inherited_href,
                 Cursor& cursor, LayoutResult& result, float left) {
  if (node.type == NodeType::text) {
    for (const std::string& word : words(node.text)) {
      const std::string rendered = cursor.x > left ? " " + word : word;
      Size size = measure(rendered, inherited);
      if (cursor.x + size.width > viewport_width - 16.0F && cursor.x > left) {
        new_line(cursor, left);
        size = measure(word, inherited);
        result.fragments.push_back({word, inherited_href, inherited, cursor.x,
                                    cursor.y, size.width, size.height});
      } else {
        result.fragments.push_back({rendered, inherited_href, inherited, cursor.x,
                                    cursor.y, size.width, size.height});
      }
      cursor.x += size.width;
      cursor.line_height = std::max(cursor.line_height, size.height);
    }
    return;
  }
  if (node.type == NodeType::element && kHidden.contains(node.name)) return;

  const ComputedStyle style = node.type == NodeType::element
                                  ? compute_style(node, inherited, rules)
                                  : inherited;
  if (style.hidden) return;
  const bool block = node.type == NodeType::document || kBlocks.contains(node.name);
  const float child_left = left + style.margin_px + style.padding_px;
  if (block && cursor.x > left) new_line(cursor, left, style.margin_px);
  if (block) {
    cursor.x = child_left;
    cursor.y += style.margin_px + style.padding_px;
  }

  std::string href = inherited_href;
  if (node.name == "a") href = node.attribute("href");
  if (node.name == "br") new_line(cursor, child_left);
  if (node.name == "img") {
    const std::string alt = node.attribute("alt").empty() ? "[image]" : node.attribute("alt");
    const Size size = measure(alt, style);
    if (cursor.x + size.width > viewport_width - 16.0F) new_line(cursor, child_left);
    result.fragments.push_back({alt, href, style, cursor.x, cursor.y, size.width, size.height});
    cursor.x += size.width;
    cursor.line_height = std::max(cursor.line_height, size.height);
  }
  for (const auto& child : node.children) {
    layout_node(*child, style, rules, viewport_width, measure, href, cursor, result,
                child_left);
  }
  if (block) {
    new_line(cursor, left, style.padding_px + style.margin_px);
  }
}

}  // namespace

LayoutResult layout_document(const Document& document, const std::vector<Rule>& rules,
                             float viewport_width, const TextMeasurer& measure) {
  LayoutResult result;
  if (!document.root || viewport_width <= 32.0F || !measure) return result;
  Cursor cursor;
  layout_node(*document.root, ComputedStyle{}, rules, viewport_width, measure, {},
              cursor, result, 16.0F);
  result.content_height = cursor.y + cursor.line_height + 16.0F;
  return result;
}

}  // namespace kitten::engine
