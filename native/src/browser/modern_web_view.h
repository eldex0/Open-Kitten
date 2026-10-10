#pragma once

#include <windows.h>
#ifndef interface
#define interface struct
#endif
#include <WebView2.h>
#include <wrl.h>

#include <QWidget>
#include <functional>

class ModernWebView final : public QWidget {
 public:
  explicit ModernWebView(bool private_mode, const QString& profile_name,
                         bool password_autosave, bool strict_tracking,
                         QWidget* parent = nullptr);
  ~ModernWebView() override;

  void navigate(const QString& url);
  void set_html(const QString& html, const QString& virtual_url,
                const QString& title);
  void go_back();
  void go_forward();
  void reload();
  void stop();
  void print_page();
  void show_downloads();
  void open_developer_tools();
  void find_text(const QString& query);
  void set_zoom_factor(double factor);
  void set_password_autosave(bool enabled);
  void set_tracking_prevention(bool strict);
  void clear_browsing_data(std::function<void(bool)> completion);
  void install_extension(const QString& folder,
                         std::function<void(bool, const QString&)> completion);
  void set_state_handler(std::function<void()> handler);
  void set_navigation_policy(std::function<bool(const QString&, bool)> policy);
  void set_resource_policy(std::function<bool(const QString&)> policy);
  void set_new_window_handler(std::function<void(const QString&)> handler);
  void set_permission_handler(std::function<bool(
      const QString&, COREWEBVIEW2_PERMISSION_KIND, bool)> handler);

  [[nodiscard]] QString url() const;
  [[nodiscard]] QString title() const;
  [[nodiscard]] bool can_go_back() const;
  [[nodiscard]] bool can_go_forward() const;
  [[nodiscard]] bool is_loading() const;
  [[nodiscard]] bool is_ready() const;
  [[nodiscard]] double zoom_factor() const;
  [[nodiscard]] int blocked_resource_count() const;
  [[nodiscard]] QString error_message() const;

 protected:
  bool event(QEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

 private:
  void initialize();
  void create_controller();
  void configure_webview();
  void update_bounds();
  void update_history_state();
  void notify_state();
  void fail(const QString& message);

  Microsoft::WRL::ComPtr<ICoreWebView2Environment> environment_;
  Microsoft::WRL::ComPtr<ICoreWebView2Controller> controller_;
  Microsoft::WRL::ComPtr<ICoreWebView2> webview_;
  QString pending_url_;
  QString pending_html_;
  QString url_;
  QString virtual_url_;
  QString virtual_title_;
  QString title_{"Open Kitten"};
  QString error_message_;
  bool loading_{false};
  bool ready_{false};
  bool can_go_back_{false};
  bool can_go_forward_{false};
  int blocked_resource_count_{0};
  double zoom_factor_{1.0};
  bool private_mode_{false};
  QString profile_name_{"Default"};
  bool password_autosave_{true};
  bool strict_tracking_{false};
  std::function<void()> state_handler_;
  std::function<bool(const QString&, bool)> navigation_policy_;
  std::function<bool(const QString&)> resource_policy_;
  std::function<void(const QString&)> new_window_handler_;
  std::function<bool(const QString&, COREWEBVIEW2_PERMISSION_KIND, bool)>
      permission_handler_;
};
