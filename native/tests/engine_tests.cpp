#include <cassert>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>
#include <vector>

#include "kitten/engine/css.h"
#include "kitten/engine/html_parser.h"
#include "kitten/engine/layout.h"
#include "kitten/engine/url_policy.h"

int main() {
  using namespace kitten::engine;
  auto document = parse_html(
      "<title>Kitten test</title><style>.note{color:#123456}</style>"
      "<h1>Hello</h1><p class='note featured'>small <a href='/next'>link</a></p>");
  assert(document.root);
  assert(document.title == "Kitten test");
  assert(text_content(*document.root).find("Hello") != std::string::npos);

  const auto entities = parse_html(
      "<title>Tom &amp; Kitten</title><p title='A &quot;quote&quot;'>"
      "1 &lt; 2 &#38;&#x20AC; &unknown;</p>");
  assert(entities.title == "Tom & Kitten");
  assert(text_content(*entities.root).find("1 < 2 &\xE2\x82\xAC &unknown;") !=
         std::string::npos);
  const Node* paragraph = entities.root->children.back().get();
  assert(paragraph->attribute("title") == "A \"quote\"");

  std::string wide_document;
  wide_document.reserve(240000);
  for (int index = 0; index < 60000; ++index) wide_document += "<br>";
  const auto bounded_nodes = parse_html(wide_document);
  std::size_t node_count = 0;
  std::vector<const Node*> nodes{bounded_nodes.root.get()};
  while (!nodes.empty()) {
    const Node* node = nodes.back();
    nodes.pop_back();
    ++node_count;
    for (const auto& child : node->children) nodes.push_back(child.get());
  }
  assert(node_count <= 50000);

  std::string deep_document;
  for (int index = 0; index < 1000; ++index) deep_document += "<div>";
  deep_document += "safe";
  for (int index = 0; index < 1000; ++index) deep_document += "</div>";
  const auto bounded_depth = parse_html(deep_document);
  std::size_t maximum_depth = 0;
  std::vector<std::pair<const Node*, std::size_t>> pending{{bounded_depth.root.get(), 1}};
  while (!pending.empty()) {
    const auto [node, depth] = pending.back();
    pending.pop_back();
    maximum_depth = std::max(maximum_depth, depth);
    for (const auto& child : node->children) pending.emplace_back(child.get(), depth + 1);
  }
  assert(maximum_depth <= 257);

  const auto rules = parse_css(
      ".note { color: #123456; background-color: #eef2f6; font-size: 18px } "
      "h1 { margin: 8px }");
  assert(rules.size() == 2);
  const Node* note = document.root->children.back().get();
  const ComputedStyle note_style = compute_style(*note, ComputedStyle{}, rules);
  assert(note_style.color == "#123456");
  assert(note_style.background_color == "#eef2f6");
  assert(note_style.font_size_px == 18.0F);
  const auto compound_rules = parse_css(
      "p.note.featured { font-weight: 600; text-decoration: underline } "
      "p#summary { visibility: hidden } .note { font-weight: normal }");
  const ComputedStyle compound_style = compute_style(
      *note, ComputedStyle{}, compound_rules);
  assert(compound_style.bold);
  assert(compound_style.underline);
  auto summary = parse_html("<p id='summary'>hidden</p>");
  assert(compute_style(*summary.root->children.back(), ComputedStyle{},
                       compound_rules).hidden);
  assert(is_safe_web_url("https://example.org/path"));
  assert(!is_safe_web_url("javascript:alert(1)"));
  assert(resolve_url("https://example.org/a/index.html", "/next") ==
         "https://example.org/next");
  assert(resolve_url("https://example.org/a/b/index.html", "../next?q=1#x") ==
         "https://example.org/a/next?q=1#x");
  assert(resolve_url("https://example.org/a?old=1", "?new=2") ==
         "https://example.org/a?new=2");
  assert(!is_safe_web_url("https://user@example.org/"));
  assert(is_safe_web_url("HTTPS://example.org:443/path"));
  assert(is_safe_web_url("http://[::1]:8080/"));
  assert(!is_safe_web_url("https://example.org:bad/"));
  assert(!is_safe_web_url("https://example.org:70000/"));
  assert(!is_safe_web_url("https://example..org/"));
  assert(!is_safe_web_url("http://[::1]trailing/"));

  const auto layout = layout_document(
      document, rules, 480.0F,
      [](std::string_view text, const ComputedStyle& style) {
        return Size{static_cast<float>(text.size()) * style.font_size_px * 0.55F,
                    style.font_size_px * 1.25F};
      });
  assert(!layout.fragments.empty());
  assert(layout.content_height > 20.0F);
  bool found_link = false;
  for (const auto& fragment : layout.fragments) {
    if (fragment.href == "/next") found_link = true;
    assert(std::isfinite(fragment.x));
    assert(std::isfinite(fragment.y));
  }
  assert(found_link);
  std::cout << "kitten_engine_tests: ok\n";
  return 0;
}
