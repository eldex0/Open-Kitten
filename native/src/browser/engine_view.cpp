#include "browser/engine_view.h"

#include <QFont>
#include <QFontMetricsF>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <algorithm>
#include <cmath>

namespace {

QFont font_for(const kitten::engine::ComputedStyle& style, float zoom_factor) {
  QFont font("Segoe UI");
  font.setPixelSize(std::max(1, static_cast<int>(std::lround(
      style.font_size_px * zoom_factor))));
  font.setBold(style.bold);
  font.setItalic(style.italic);
  font.setUnderline(style.underline);
  return font;
}

}  // namespace

EngineView::EngineView(QWidget* parent) : QWidget(parent) {
  setMinimumSize(360, 240);
  setMouseTracking(true);
  setAutoFillBackground(true);
  QPalette colors = palette();
  colors.setColor(QPalette::Window, QColor("#ffffff"));
  setPalette(colors);
}

void EngineView::set_document(kitten::engine::Document document,
                              std::vector<kitten::engine::Rule> rules) {
  document_ = std::move(document);
  rules_ = std::move(rules);
  relayout();
}

void EngineView::set_link_handler(std::function<void(const std::string&)> handler) {
  link_handler_ = std::move(handler);
}

void EngineView::set_zoom_factor(float factor) {
  const float clamped = std::clamp(factor, 0.5F, 2.0F);
  if (std::abs(clamped - zoom_factor_) < 0.001F) return;
  zoom_factor_ = clamped;
  relayout();
}

float EngineView::zoom_factor() const { return zoom_factor_; }

void EngineView::relayout() {
  layout_ = kitten::engine::layout_document(
      document_, rules_, static_cast<float>(width()),
      [this](std::string_view text, const kitten::engine::ComputedStyle& style) {
        const QFontMetricsF metrics(font_for(style, zoom_factor_));
        return kitten::engine::Size{static_cast<float>(metrics.horizontalAdvance(
                                        QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size())))),
                                    static_cast<float>(metrics.height())};
      });
  setMinimumHeight(static_cast<int>(std::ceil(layout_.content_height)));
  update();
}

void EngineView::paintEvent(QPaintEvent*) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::TextAntialiasing);
  painter.fillRect(rect(), QColor("#ffffff"));
  for (const auto& fragment : layout_.fragments) {
    if (fragment.style.background_color != "transparent") {
      const QColor background(QString::fromStdString(fragment.style.background_color));
      if (background.isValid()) {
        painter.fillRect(QRectF(fragment.x, fragment.y, fragment.width,
                                fragment.height), background);
      }
    }
    painter.setFont(font_for(fragment.style, zoom_factor_));
    painter.setPen(QColor(QString::fromStdString(fragment.style.color)));
    painter.drawText(QPointF(fragment.x, fragment.y + fragment.height * 0.82F),
                     QString::fromStdString(fragment.text));
    if (!fragment.href.empty()) {
      painter.drawLine(QPointF(fragment.x, fragment.y + fragment.height),
                       QPointF(fragment.x + fragment.width, fragment.y + fragment.height));
    }
  }
}

void EngineView::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  relayout();
}

void EngineView::mouseReleaseEvent(QMouseEvent* event) {
  const QPointF point = event->position();
  for (const auto& fragment : layout_.fragments) {
    const QRectF bounds(fragment.x, fragment.y, fragment.width, fragment.height);
    if (!fragment.href.empty() && bounds.contains(point)) {
      if (link_handler_) link_handler_(fragment.href);
      return;
    }
  }
  QWidget::mouseReleaseEvent(event);
}

void EngineView::mouseMoveEvent(QMouseEvent* event) {
  const QPointF point = event->position();
  const bool over_link = std::any_of(layout_.fragments.begin(), layout_.fragments.end(),
      [point](const kitten::engine::Fragment& fragment) {
        return !fragment.href.empty() &&
               QRectF(fragment.x, fragment.y, fragment.width, fragment.height).contains(point);
      });
  setCursor(over_link ? Qt::PointingHandCursor : Qt::ArrowCursor);
  QWidget::mouseMoveEvent(event);
}
