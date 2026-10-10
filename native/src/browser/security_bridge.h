#pragma once

#include <QLibrary>
#include <QString>
#include <cstddef>

class SecurityBridge final {
 public:
  SecurityBridge();

  [[nodiscard]] bool available() const;
  [[nodiscard]] bool is_tracking_host(const QString& host) const;
  [[nodiscard]] QString clean_url(const QString& url) const;

 private:
  using IsTrackingHost = bool (*)(const char*);
  using StripTrackingParameters = std::size_t (*)(const char*, char*, std::size_t);

  QLibrary library_;
  IsTrackingHost is_tracking_host_{nullptr};
  StripTrackingParameters strip_tracking_parameters_{nullptr};
};
