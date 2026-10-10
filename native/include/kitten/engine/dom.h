#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace kitten::engine {

enum class NodeType { document, element, text };

struct Node final {
  NodeType type{NodeType::element};
  std::string name;
  std::string text;
  std::unordered_map<std::string, std::string> attributes;
  std::vector<std::unique_ptr<Node>> children;
  Node* parent{nullptr};

  [[nodiscard]] std::string attribute(const std::string& key) const {
    const auto found = attributes.find(key);
    return found == attributes.end() ? std::string{} : found->second;
  }
};

struct Document final {
  std::unique_ptr<Node> root;
  std::string title;
};

}  // namespace kitten::engine
