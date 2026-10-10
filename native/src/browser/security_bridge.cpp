#include "browser/security_bridge.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <algorithm>
#include <vector>

SecurityBridge::SecurityBridge() {
  const QString base = QDir(QCoreApplication::applicationDirPath())
                           .filePath("kitten_security_core");
  library_.setFileName(base);
  if (!library_.load()) return;
  is_tracking_host_ = reinterpret_cast<IsTrackingHost>(library_.resolve("kitten_is_tracking_host"));
  strip_tracking_parameters_ = reinterpret_cast<StripTrackingParameters>(
      library_.resolve("kitten_strip_tracking_parameters"));
  if (!is_tracking_host_ || !strip_tracking_parameters_) library_.unload();
}

bool SecurityBridge::available() const {
  return library_.isLoaded() && is_tracking_host_ && strip_tracking_parameters_;
}

bool SecurityBridge::is_tracking_host(const QString& host) const {
  if (!available()) return false;
  const QByteArray encoded = host.toUtf8();
  return is_tracking_host_(encoded.constData());
}

QString SecurityBridge::clean_url(const QString& url) const {
  if (!available()) return url;
  const QByteArray input = url.toUtf8();
  const std::size_t required = strip_tracking_parameters_(input.constData(), nullptr, 0);
  if (required == 0 || required > 64 * 1024) return url;
  std::vector<char> output(required + 1, '\0');
  const std::size_t written = strip_tracking_parameters_(
      input.constData(), output.data(), output.size());
  if (written != required) return url;
  return QString::fromUtf8(output.data(), static_cast<qsizetype>(written));
}
