#pragma once

#include <QWidget>
#include <functional>
#include <string>
#include <vector>

#include "kitten/engine/css.h"
#include "kitten/engine/dom.h"
#include "kitten/engine/layout.h"

class EngineView final : public QWidget {
 public:
  explicit EngineView(QWidget* parent = nullptr);

  void set_document(kitten::engine::Document document,
                    std::vector<kitten::engine::Rule> rules);
  void set_link_handler(std::function<void(const std::string&)> handler);
  void set_zoom_factor(float factor);
  [[nodiscard]] float zoom_factor() const;

 protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

 private:
  void relayout();

  kitten::engine::Document document_;
  std::vector<kitten::engine::Rule> rules_;
  kitten::engine::LayoutResult layout_;
  std::function<void(const std::string&)> link_handler_;
  float zoom_factor_{1.0F};
};
