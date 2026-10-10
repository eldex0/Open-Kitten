#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QTimer>

#include <cstring>

#ifdef Q_OS_WIN
#include <objbase.h>
#endif

#include "browser/security_bridge.h"
#include "browser/browser_window.h"
#include "kitten/version.h"

int main(int argc, char* argv[]) {
  bool self_test = false;
  bool ui_smoke_test = false;
  bool private_smoke_test = false;
  for (int index = 1; index < argc; ++index) {
    if (std::strcmp(argv[index], "--self-test") == 0) self_test = true;
    if (std::strcmp(argv[index], "--ui-smoke-test") == 0) ui_smoke_test = true;
    if (std::strcmp(argv[index], "--private-smoke-test") == 0) private_smoke_test = true;
  }
  if (self_test) {
    QCoreApplication application(argc, argv);
    application.setApplicationName("Open Kitten Self-Test");
    const SecurityBridge security;
    return security.available() ? 0 : 2;
  }

#ifdef Q_OS_WIN
  const HRESULT com_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
#endif
  QApplication application(argc, argv);
  application.setApplicationName("Open Kitten");
  application.setOrganizationName("Open Kitten");
  application.setApplicationVersion(QString::fromLatin1(kitten::kProductVersion.data(),
      static_cast<qsizetype>(kitten::kProductVersion.size())));
  application.setWindowIcon(QIcon(":/open-kitten/icon.png"));
  BrowserWindow browser(private_smoke_test);
  if (ui_smoke_test || private_smoke_test) {
    browser.ensurePolished();
    auto* readiness_check = new QTimer(&application);
    readiness_check->setInterval(100);
    QObject::connect(readiness_check, &QTimer::timeout, &application,
                     [&application, &browser, readiness_check] {
      if (!browser.modern_engine_ready()) return;
      readiness_check->stop();
      application.exit(0);
    });
    readiness_check->start();
    QTimer::singleShot(15000, &application, [&application, &browser] {
      if (!browser.modern_engine_ready()) application.exit(3);
    });
  } else {
    browser.show();
  }
  const int result = application.exec();
#ifdef Q_OS_WIN
  if (SUCCEEDED(com_result)) CoUninitialize();
#endif
  return result;
}
