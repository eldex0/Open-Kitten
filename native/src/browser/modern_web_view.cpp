#include "browser/modern_web_view.h"

#include <QDir>
#include <QDebug>
#include <QEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QResizeEvent>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <algorithm>

#ifdef __has_attribute
#undef __has_attribute
#endif
#define __has_attribute(attribute) 0
#ifndef __attribute__
#define __attribute__(attribute)
#endif
#include <WebView2EnvironmentOptions.h>
#include <windows.h>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Make;

namespace {

QString take_wide_string(LPWSTR value) {
  const QString result = value ? QString::fromWCharArray(value) : QString{};
  CoTaskMemFree(value);
  return result;
}

}  // namespace

ModernWebView::ModernWebView(bool private_mode, const QString& profile_name,
                             bool password_autosave, bool strict_tracking,
                             QWidget* parent)
    : QWidget(parent), private_mode_(private_mode), profile_name_(profile_name),
      password_autosave_(password_autosave), strict_tracking_(strict_tracking) {
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_DontCreateNativeAncestors);
  setFocusPolicy(Qt::StrongFocus);
  setAutoFillBackground(true);
  QTimer::singleShot(0, this, [this] { initialize(); });
}

ModernWebView::~ModernWebView() {
  if (controller_) controller_->Close();
}

void ModernWebView::initialize() {
  const QString data_path = QStandardPaths::writableLocation(
      QStandardPaths::AppLocalDataLocation) + "/ChromiumProfile";
  QDir().mkpath(data_path);
  const std::wstring data_folder = QDir::toNativeSeparators(data_path).toStdWString();

  auto options = Make<CoreWebView2EnvironmentOptions>();
  ComPtr<ICoreWebView2EnvironmentOptions5> privacy_options;
  if (SUCCEEDED(options.As(&privacy_options))) {
    privacy_options->put_EnableTrackingPrevention(TRUE);
  }
  ComPtr<ICoreWebView2EnvironmentOptions6> extension_options;
  if (SUCCEEDED(options.As(&extension_options)) && extension_options) {
    extension_options->put_AreBrowserExtensionsEnabled(TRUE);
  }

  const QPointer<ModernWebView> guard(this);
  const HRESULT result = CreateCoreWebView2EnvironmentWithOptions(
      nullptr, data_folder.c_str(), options.Get(),
      Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
          [guard](HRESULT status, ICoreWebView2Environment* environment) -> HRESULT {
            if (!guard) return S_OK;
            if (FAILED(status) || !environment) {
              guard->fail(QString("WebView2 environment error: 0x%1")
                              .arg(static_cast<qulonglong>(status), 8, 16, QChar('0')));
              return S_OK;
            }
            guard->environment_ = environment;
            guard->create_controller();
            return S_OK;
          }).Get());
  if (FAILED(result)) {
    fail(QString("WebView2 startup error: 0x%1")
             .arg(static_cast<qulonglong>(result), 8, 16, QChar('0')));
  }
}

void ModernWebView::create_controller() {
  if (!environment_) return;
  const HWND host = reinterpret_cast<HWND>(winId());
  const QPointer<ModernWebView> guard(this);
  auto completion = Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
      [guard](HRESULT status, ICoreWebView2Controller* controller) -> HRESULT {
        if (!guard) return S_OK;
        if (FAILED(status) || !controller) {
          guard->fail(QString("WebView2 controller error: 0x%1")
              .arg(static_cast<qulonglong>(status), 8, 16, QChar('0')));
          return S_OK;
        }
        guard->controller_ = controller;
        controller->get_CoreWebView2(&guard->webview_);
        guard->configure_webview();
        return S_OK;
      });

  ComPtr<ICoreWebView2Environment10> environment10;
  ComPtr<ICoreWebView2ControllerOptions> controller_options;
  if (SUCCEEDED(environment_.As(&environment10)) && environment10 &&
      SUCCEEDED(environment10->CreateCoreWebView2ControllerOptions(
          &controller_options)) && controller_options) {
    const std::wstring profile = profile_name_.toStdWString();
    controller_options->put_ProfileName(profile.c_str());
    if (private_mode_) {
      controller_options->put_IsInPrivateModeEnabled(TRUE);
    }
    const HRESULT result = environment10->CreateCoreWebView2ControllerWithOptions(
        host, controller_options.Get(), completion.Get());
    if (SUCCEEDED(result)) return;
  }
  if (private_mode_) {
    fail("Private browsing is not supported by the installed WebView2 runtime");
    return;
  }
  const HRESULT result = environment_->CreateCoreWebView2Controller(
      host, completion.Get());
  if (FAILED(result)) {
    fail(QString("WebView2 controller startup error: 0x%1")
        .arg(static_cast<qulonglong>(result), 8, 16, QChar('0')));
  }
}

void ModernWebView::configure_webview() {
  if (!webview_ || !controller_) {
    fail("WebView2 initialization failed");
    return;
  }

  ComPtr<ICoreWebView2Settings> settings;
  if (SUCCEEDED(webview_->get_Settings(&settings)) && settings) {
    settings->put_IsScriptEnabled(TRUE);
    settings->put_AreDefaultScriptDialogsEnabled(TRUE);
    settings->put_AreDevToolsEnabled(TRUE);
    settings->put_IsStatusBarEnabled(FALSE);
    settings->put_IsZoomControlEnabled(TRUE);
    settings->put_AreHostObjectsAllowed(FALSE);
    settings->put_IsWebMessageEnabled(FALSE);
    ComPtr<ICoreWebView2Settings4> profile_settings;
    if (SUCCEEDED(settings.As(&profile_settings)) && profile_settings) {
      profile_settings->put_IsPasswordAutosaveEnabled(
          !private_mode_ && password_autosave_ ? TRUE : FALSE);
      if (private_mode_) profile_settings->put_IsGeneralAutofillEnabled(FALSE);
    }
  }
  set_tracking_prevention(strict_tracking_);

  const QPointer<ModernWebView> guard(this);
  EventRegistrationToken token{};
  webview_->add_NavigationStarting(
      Callback<ICoreWebView2NavigationStartingEventHandler>(
          [guard](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
            if (!guard) return S_OK;
            LPWSTR raw_uri = nullptr;
            args->get_Uri(&raw_uri);
            const QString target = take_wide_string(raw_uri);
            const bool internal_document = !guard->virtual_url_.isEmpty() &&
                                           target == "about:blank";
            if (!internal_document && !guard->virtual_url_.isEmpty()) {
              guard->virtual_url_.clear();
              guard->virtual_title_.clear();
            }
            BOOL redirected = FALSE;
            args->get_IsRedirected(&redirected);
            if (!internal_document && redirected == FALSE) {
              guard->blocked_resource_count_ = 0;
            }
            if (!internal_document && guard->navigation_policy_ &&
                !guard->navigation_policy_(target, redirected == TRUE)) {
              args->put_Cancel(TRUE);
              guard->loading_ = false;
              guard->notify_state();
              return S_OK;
            }
            if (!internal_document) guard->url_ = target;
            guard->loading_ = true;
            guard->error_message_.clear();
            guard->notify_state();
            return S_OK;
          }).Get(), &token);
  webview_->add_NavigationCompleted(
      Callback<ICoreWebView2NavigationCompletedEventHandler>(
          [guard](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
            if (!guard) return S_OK;
            guard->loading_ = false;
            BOOL succeeded = FALSE;
            args->get_IsSuccess(&succeeded);
            if (!succeeded) {
              COREWEBVIEW2_WEB_ERROR_STATUS status{};
              args->get_WebErrorStatus(&status);
              guard->error_message_ = QString("Navigation error: %1")
                  .arg(static_cast<int>(status));
            }
            guard->update_history_state();
            guard->notify_state();
            return S_OK;
          }).Get(), &token);
  webview_->add_SourceChanged(
      Callback<ICoreWebView2SourceChangedEventHandler>(
          [guard](ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs*) -> HRESULT {
            if (!guard) return S_OK;
            LPWSTR source = nullptr;
            sender->get_Source(&source);
            guard->url_ = take_wide_string(source);
            guard->update_history_state();
            guard->notify_state();
            return S_OK;
          }).Get(), &token);
  webview_->add_DocumentTitleChanged(
      Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
          [guard](ICoreWebView2* sender, IUnknown*) -> HRESULT {
            if (!guard) return S_OK;
            LPWSTR title = nullptr;
            sender->get_DocumentTitle(&title);
            guard->title_ = take_wide_string(title).simplified();
            if (guard->title_.isEmpty()) guard->title_ = QUrl(guard->url_).host();
            guard->notify_state();
            return S_OK;
          }).Get(), &token);
  webview_->add_HistoryChanged(
      Callback<ICoreWebView2HistoryChangedEventHandler>(
          [guard](ICoreWebView2*, IUnknown*) -> HRESULT {
            if (!guard) return S_OK;
            guard->update_history_state();
            guard->notify_state();
            return S_OK;
          }).Get(), &token);
  webview_->add_NewWindowRequested(
      Callback<ICoreWebView2NewWindowRequestedEventHandler>(
          [guard](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
            if (!guard) return S_OK;
            LPWSTR raw_uri = nullptr;
            args->get_Uri(&raw_uri);
            const QString target = take_wide_string(raw_uri);
            args->put_Handled(TRUE);
            if (guard->new_window_handler_) guard->new_window_handler_(target);
            else guard->navigate(target);
            return S_OK;
          }).Get(), &token);
  webview_->add_PermissionRequested(
      Callback<ICoreWebView2PermissionRequestedEventHandler>(
          [guard](ICoreWebView2*, ICoreWebView2PermissionRequestedEventArgs* args)
              -> HRESULT {
            if (!guard || !args) return S_OK;
            LPWSTR raw_uri = nullptr;
            COREWEBVIEW2_PERMISSION_KIND kind =
                COREWEBVIEW2_PERMISSION_KIND_UNKNOWN_PERMISSION;
            BOOL user_initiated = FALSE;
            args->get_Uri(&raw_uri);
            args->get_PermissionKind(&kind);
            args->get_IsUserInitiated(&user_initiated);
            const QString origin = take_wide_string(raw_uri);
            const bool allowed = guard->permission_handler_ &&
                guard->permission_handler_(origin, kind, user_initiated == TRUE);
            args->put_State(allowed ? COREWEBVIEW2_PERMISSION_STATE_ALLOW
                                    : COREWEBVIEW2_PERMISSION_STATE_DENY);
            return S_OK;
          }).Get(), &token);
  webview_->AddWebResourceRequestedFilter(
      L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
  webview_->add_WebResourceRequested(
      Callback<ICoreWebView2WebResourceRequestedEventHandler>(
          [guard](ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* args)
              -> HRESULT {
            if (!guard || !guard->resource_policy_) return S_OK;
            ComPtr<ICoreWebView2WebResourceRequest> request;
            if (FAILED(args->get_Request(&request)) || !request) return S_OK;
            LPWSTR raw_uri = nullptr;
            request->get_Uri(&raw_uri);
            const QString target = take_wide_string(raw_uri);
            if (guard->resource_policy_(target)) return S_OK;
            ++guard->blocked_resource_count_;
            ComPtr<ICoreWebView2WebResourceResponse> response;
            if (guard->environment_ && SUCCEEDED(
                guard->environment_->CreateWebResourceResponse(
                    nullptr, 403, L"Blocked by Kitten Security Core",
                    L"Content-Type: text/plain\r\nCache-Control: no-store\r\n",
                    &response))) {
              args->put_Response(response.Get());
            }
            return S_OK;
          }).Get(), &token);

  ready_ = true;
  controller_->put_IsVisible(TRUE);
  controller_->put_ZoomFactor(zoom_factor_);
  update_bounds();
  notify_state();
  if (!pending_url_.isEmpty()) {
    const QString target = pending_url_;
    pending_url_.clear();
    navigate(target);
  } else if (!pending_html_.isEmpty()) {
    const QString html = pending_html_;
    pending_html_.clear();
    const std::wstring source = html.toStdWString();
    webview_->NavigateToString(source.c_str());
  }
}

void ModernWebView::navigate(const QString& url) {
  virtual_url_.clear();
  virtual_title_.clear();
  pending_html_.clear();
  if (!webview_) {
    pending_url_ = url;
    url_ = url;
    loading_ = true;
    notify_state();
    return;
  }
  const std::wstring target = url.toStdWString();
  const HRESULT result = webview_->Navigate(target.c_str());
  if (FAILED(result)) fail(QString("Navigation startup error: 0x%1")
      .arg(static_cast<qulonglong>(result), 8, 16, QChar('0')));
}

void ModernWebView::set_html(const QString& html, const QString& virtual_url,
                             const QString& title) {
  virtual_url_ = virtual_url;
  virtual_title_ = title;
  url_ = virtual_url;
  title_ = title;
  pending_url_.clear();
  loading_ = true;
  if (!webview_) {
    pending_html_ = html;
    notify_state();
    return;
  }
  const std::wstring source = html.toStdWString();
  const HRESULT result = webview_->NavigateToString(source.c_str());
  if (FAILED(result)) fail(QString("HTML navigation error: 0x%1")
      .arg(static_cast<qulonglong>(result), 8, 16, QChar('0')));
}

void ModernWebView::go_back() { if (webview_ && can_go_back_) webview_->GoBack(); }
void ModernWebView::go_forward() { if (webview_ && can_go_forward_) webview_->GoForward(); }
void ModernWebView::reload() { if (webview_) webview_->Reload(); }
void ModernWebView::stop() { if (webview_) webview_->Stop(); }

void ModernWebView::print_page() {
  if (!webview_) return;
  ComPtr<ICoreWebView2_16> printing;
  if (SUCCEEDED(webview_.As(&printing)) && printing) {
    printing->ShowPrintUI(COREWEBVIEW2_PRINT_DIALOG_KIND_BROWSER);
  }
}

void ModernWebView::show_downloads() {
  if (!webview_) return;
  ComPtr<ICoreWebView2_9> downloads;
  if (SUCCEEDED(webview_.As(&downloads)) && downloads) {
    downloads->OpenDefaultDownloadDialog();
  }
}

void ModernWebView::open_developer_tools() {
  if (webview_) webview_->OpenDevToolsWindow();
}

void ModernWebView::find_text(const QString& query) {
  if (!webview_ || query.isEmpty()) return;
  const QJsonArray arguments{query};
  QString json = QString::fromUtf8(QJsonDocument(arguments).toJson(
      QJsonDocument::Compact));
  json = json.mid(1, json.size() - 2);
  const std::wstring script = QString("window.find(%1, false, false, true, false, true, false);")
      .arg(json).toStdWString();
  webview_->ExecuteScript(script.c_str(), nullptr);
}

void ModernWebView::set_zoom_factor(double factor) {
  zoom_factor_ = std::clamp(factor, 0.5, 2.0);
  if (controller_) controller_->put_ZoomFactor(zoom_factor_);
}

void ModernWebView::set_password_autosave(bool enabled) {
  password_autosave_ = enabled;
  if (!webview_ || private_mode_) return;
  ComPtr<ICoreWebView2Settings> settings;
  ComPtr<ICoreWebView2Settings4> profile_settings;
  if (SUCCEEDED(webview_->get_Settings(&settings)) && settings &&
      SUCCEEDED(settings.As(&profile_settings)) && profile_settings) {
    profile_settings->put_IsPasswordAutosaveEnabled(enabled ? TRUE : FALSE);
  }
}

void ModernWebView::set_tracking_prevention(bool strict) {
  strict_tracking_ = strict;
  if (!webview_) return;
  ComPtr<ICoreWebView2_13> webview13;
  ComPtr<ICoreWebView2Profile> profile;
  ComPtr<ICoreWebView2Profile3> profile3;
  if (SUCCEEDED(webview_.As(&webview13)) && webview13 &&
      SUCCEEDED(webview13->get_Profile(&profile)) && profile &&
      SUCCEEDED(profile.As(&profile3)) && profile3) {
    profile3->put_PreferredTrackingPreventionLevel(
        strict ? COREWEBVIEW2_TRACKING_PREVENTION_LEVEL_STRICT
               : COREWEBVIEW2_TRACKING_PREVENTION_LEVEL_BALANCED);
  }
}

void ModernWebView::clear_browsing_data(std::function<void(bool)> completion) {
  if (!webview_) {
    if (completion) completion(false);
    return;
  }
  ComPtr<ICoreWebView2_13> webview13;
  ComPtr<ICoreWebView2Profile> profile;
  ComPtr<ICoreWebView2Profile2> profile2;
  if (FAILED(webview_.As(&webview13)) || !webview13 ||
      FAILED(webview13->get_Profile(&profile)) || !profile ||
      FAILED(profile.As(&profile2)) || !profile2) {
    if (completion) completion(false);
    return;
  }
  const QPointer<ModernWebView> guard(this);
  profile2->ClearBrowsingDataAll(
      Callback<ICoreWebView2ClearBrowsingDataCompletedHandler>(
          [guard, completion](HRESULT status) -> HRESULT {
            if (guard && completion) completion(SUCCEEDED(status));
            return S_OK;
          }).Get());
}

void ModernWebView::install_extension(
    const QString& folder,
    std::function<void(bool, const QString&)> completion) {
  if (!webview_) {
    if (completion) completion(false, "WebView2 is not ready");
    return;
  }
  ComPtr<ICoreWebView2_13> webview13;
  ComPtr<ICoreWebView2Profile> profile;
  ComPtr<ICoreWebView2Profile7> profile7;
  if (FAILED(webview_.As(&webview13)) || !webview13 ||
      FAILED(webview13->get_Profile(&profile)) || !profile ||
      FAILED(profile.As(&profile7)) || !profile7) {
    if (completion) completion(false, "This WebView2 runtime does not support extensions");
    return;
  }
  const QPointer<ModernWebView> guard(this);
  const std::wstring path = folder.toStdWString();
  const HRESULT started = profile7->AddBrowserExtension(
      path.c_str(),
      Callback<ICoreWebView2ProfileAddBrowserExtensionCompletedHandler>(
          [guard, completion](
              HRESULT status, ICoreWebView2BrowserExtension*) -> HRESULT {
            if (guard && completion) {
              completion(SUCCEEDED(status), SUCCEEDED(status)
                  ? QString{} : QString("Extension error: 0x%1")
                      .arg(static_cast<qulonglong>(status), 8, 16, QChar('0')));
            }
            return S_OK;
          }).Get());
  if (FAILED(started) && completion) {
    completion(false, QString("Extension startup error: 0x%1")
        .arg(static_cast<qulonglong>(started), 8, 16, QChar('0')));
  }
}

void ModernWebView::set_state_handler(std::function<void()> handler) {
  state_handler_ = std::move(handler);
}

void ModernWebView::set_navigation_policy(
    std::function<bool(const QString&, bool)> policy) {
  navigation_policy_ = std::move(policy);
}

void ModernWebView::set_resource_policy(
    std::function<bool(const QString&)> policy) {
  resource_policy_ = std::move(policy);
}

void ModernWebView::set_new_window_handler(
    std::function<void(const QString&)> handler) {
  new_window_handler_ = std::move(handler);
}

void ModernWebView::set_permission_handler(std::function<bool(
    const QString&, COREWEBVIEW2_PERMISSION_KIND, bool)> handler) {
  permission_handler_ = std::move(handler);
}

QString ModernWebView::url() const {
  return virtual_url_.isEmpty() ? url_ : virtual_url_;
}
QString ModernWebView::title() const {
  return virtual_title_.isEmpty() ? title_ : virtual_title_;
}
bool ModernWebView::can_go_back() const { return can_go_back_; }
bool ModernWebView::can_go_forward() const { return can_go_forward_; }
bool ModernWebView::is_loading() const { return loading_; }
bool ModernWebView::is_ready() const { return ready_; }
double ModernWebView::zoom_factor() const { return zoom_factor_; }
int ModernWebView::blocked_resource_count() const { return blocked_resource_count_; }
QString ModernWebView::error_message() const { return error_message_; }

void ModernWebView::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  update_bounds();
}

bool ModernWebView::event(QEvent* event) {
  const bool result = QWidget::event(event);
  if (event->type() == QEvent::Show || event->type() == QEvent::LayoutRequest ||
      event->type() == QEvent::WindowStateChange || event->type() == QEvent::ScreenChangeInternal) {
    QTimer::singleShot(0, this, [this] { update_bounds(); });
  }
  return result;
}

void ModernWebView::update_bounds() {
  if (!controller_) return;
  const qreal scale = devicePixelRatioF();
  const LONG physical_width = static_cast<LONG>(qRound(width() * scale));
  const LONG physical_height = static_cast<LONG>(qRound(height() * scale));
  const RECT bounds{0, 0, physical_width, physical_height};
  controller_->put_Bounds(bounds);
}

void ModernWebView::update_history_state() {
  if (!webview_) return;
  BOOL value = FALSE;
  webview_->get_CanGoBack(&value);
  can_go_back_ = value == TRUE;
  value = FALSE;
  webview_->get_CanGoForward(&value);
  can_go_forward_ = value == TRUE;
}

void ModernWebView::notify_state() {
  if (state_handler_) state_handler_();
}

void ModernWebView::fail(const QString& message) {
  qWarning().noquote() << "Kitten Engine:" << message;
  error_message_ = message;
  loading_ = false;
  ready_ = false;
  notify_state();
}
