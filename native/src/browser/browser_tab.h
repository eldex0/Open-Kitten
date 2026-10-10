#pragma once

#include <QPointer>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <functional>

#include "browser/localization.h"

class EngineView;
class ModernWebView;
class QNetworkAccessManager;
class QNetworkReply;
class QStackedWidget;
class SecurityBridge;

class BrowserTab final : public QWidget {
 public:
  BrowserTab(SecurityBridge* security, UiLanguage language, bool private_mode,
             const QString& search_engine, const QString& profile_name,
             bool password_autosave, bool strict_tracking,
             bool prefer_safe_view,
             QWidget* parent = nullptr);
  ~BrowserTab() override;

  void navigate(const QString& input, bool record_history = true);
  void go_back();
  void go_forward();
  void reload();
  void stop();
  void show_home();
  void show_privacy();
  void set_language(UiLanguage language);
  void set_search_engine(const QString& search_engine);
  void set_password_autosave(bool enabled);
  void set_tracking_prevention(bool strict);
  void set_prefer_safe_view(bool enabled);
  void clear_browsing_data(std::function<void(bool)> completion);
  void install_extension(const QString& folder,
                         std::function<void(bool, const QString&)> completion);
  void set_state_handler(std::function<void()> handler);
  void set_new_tab_handler(std::function<void(const QString&)> handler);
  void zoom_in();
  void zoom_out();
  void reset_zoom();
  void use_safe_view();
  void use_kitten_engine();
  void print_page();
  void show_downloads();
  void open_developer_tools();
  void find_text(const QString& query);
  void set_protection_enabled(bool enabled);

  [[nodiscard]] QString url() const;
  [[nodiscard]] QString title() const;
  [[nodiscard]] bool can_go_back() const;
  [[nodiscard]] bool can_go_forward() const;
  [[nodiscard]] bool is_loading() const;
  [[nodiscard]] int zoom_percent() const;
  [[nodiscard]] bool uses_modern_engine() const;
  [[nodiscard]] bool modern_engine_ready() const;
  [[nodiscard]] bool protection_enabled() const;
  [[nodiscard]] int blocked_resource_count() const;

 private:
  [[nodiscard]] QString resolve_input(const QString& input) const;
  void start_request(const QString& candidate, bool record_history, int redirect_count);
  void finish_navigation(QNetworkReply* reply, bool record_history, int redirect_count);
  void show_html(const QByteArray& source, const QString& final_url);
  void show_error(const QString& message, const QString& url = {});
  void notify_state();

  SecurityBridge* security_{nullptr};
  UiLanguage language_{UiLanguage::french};
  EngineView* view_{nullptr};
  ModernWebView* modern_view_{nullptr};
  QStackedWidget* view_stack_{nullptr};
  QNetworkAccessManager* network_{nullptr};
  QPointer<QNetworkReply> reply_;
  QString current_url_{"kitten://home"};
  QString title_{"Nouvel onglet"};
  QStringList history_;
  int history_index_{-1};
  bool loading_{false};
  bool reply_too_large_{false};
  bool modern_mode_{false};
  bool modern_error_reported_{false};
  bool protection_enabled_{true};
  bool private_mode_{false};
  bool prefer_safe_view_{false};
  QString search_engine_{"duckduckgo"};
  std::function<void()> state_handler_;
  std::function<void(const QString&)> new_tab_handler_;
};
