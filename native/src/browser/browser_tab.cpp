#include "browser/browser_tab.h"

#include <QByteArray>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include "browser/engine_view.h"
#include "browser/modern_web_view.h"
#include "browser/security_bridge.h"
#include "kitten/engine/css.h"
#include "kitten/engine/html_parser.h"
#include "kitten/engine/url_policy.h"

namespace {
constexpr qint64 kMaximumPageBytes = 5 * 1024 * 1024;
constexpr int kMaximumRedirects = 10;

QString normalized_search_engine(QString engine) {
  engine = engine.trimmed().toLower();
  return engine == "brave" || engine == "qwant" ? engine : "duckduckgo";
}

QString search_engine_name(const QString& engine) {
  if (engine == "brave") return "Brave Search";
  if (engine == "qwant") return "Qwant";
  return "DuckDuckGo";
}

QString search_endpoint(const QString& engine) {
  if (engine == "brave") return "https://search.brave.com/search";
  if (engine == "qwant") return "https://www.qwant.com/";
  return "https://duckduckgo.com/";
}

QString search_url(const QString& engine, const QString& query) {
  return search_endpoint(engine) + "?q=" +
         QString::fromLatin1(QUrl::toPercentEncoding(query));
}

void collect_css(const kitten::engine::Node& node, std::string& output) {
  if (node.name == "style") output += kitten::engine::text_content(node) + "\n";
  for (const auto& child : node.children) collect_css(*child, output);
}

QString home_page(UiLanguage language, bool private_mode,
                  const QString& search_engine) {
  QFile logo(":/open-kitten/icon.png");
  QString logo_source;
  if (logo.open(QIODevice::ReadOnly)) {
    logo_source = "data:image/png;base64," + QString::fromLatin1(logo.readAll().toBase64());
  }
  const QString description = ui_text(
      language,
      "Chromium pour le Web moderne. Kitten Security Core pour votre confidentialité.",
      "Chromium for the modern Web. Kitten Security Core for your privacy.");
  const QString provider = search_engine_name(search_engine);
  const QString search = ui_text(language, "Rechercher avec %1", "Search with %1")
                             .arg(provider);
  const QString engine = ui_text(language, "Kitten Engine 2.0", "Kitten Engine 2.0");
  const QString mode = private_mode
      ? ui_text(language, "Fenêtre privée", "Private window")
      : ui_text(language, "Navigation protégée", "Protected browsing");
  const QString stats_title = ui_text(language, "Confidentialité en un coup d’œil",
                                      "Privacy at a glance");
  const QString trackers = ui_text(language, "Traqueurs et publicités filtrés",
                                   "Trackers and ads filtered");
  const QString upgrades = ui_text(language, "Connexions sécurisées privilégiées",
                                   "Secure connections preferred");
  const QString safe_view = ui_text(language, "Lecture isolée sans scripts distants",
                                    "Isolated reading without remote scripts");
  QString html = QString(R"HTML(<!doctype html><html lang="fr"><head><meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1"><title>Open Kitten</title><style>
  :root{color-scheme:dark;font-family:Inter,"Segoe UI",Arial,sans-serif}*{box-sizing:border-box}
  body{margin:0;min-height:100vh;color:#f7f8ff;background:#101116;overflow:auto}
  body:before{content:"";position:fixed;inset:0;background:radial-gradient(circle at 50% -10%,rgba(92,75,255,.27),transparent 38%),radial-gradient(circle at 92% 82%,rgba(22,135,218,.13),transparent 30%);pointer-events:none}
  .page{position:relative;min-height:100vh;padding:7vh 6vw 4vh;display:flex;flex-direction:column;align-items:center}
  .brand{display:flex;align-items:center;gap:16px;margin-bottom:25px}.brand img{width:68px;height:68px;object-fit:contain;filter:drop-shadow(0 10px 25px rgba(78,91,255,.28))}
  h1{font-size:35px;letter-spacing:-1.1px;margin:0}.engine{color:#a9afc0;font-size:13px;margin-top:3px}
  form{width:min(720px,90vw);display:flex;align-items:center;background:#232630;border:1px solid #4e5877;border-radius:16px;padding:5px 7px 5px 18px;box-shadow:0 18px 52px rgba(0,0,0,.32)}
  input{flex:1;background:transparent;border:0;outline:0;color:white;font-size:16px;padding:13px 5px}input::placeholder{color:#969cac}
  button{border:0;border-radius:11px;background:#5b62ed;color:white;font-weight:700;padding:12px 18px;cursor:pointer}button:hover{background:#6b72ff}
  .links{display:grid;grid-template-columns:repeat(4,minmax(120px,1fr));gap:12px;width:min(720px,90vw);margin-top:24px}
  .links a{color:#edf0f8;text-decoration:none;background:#20232b;border:1px solid #303541;border-radius:13px;padding:15px;transition:.15s}
  .links a:hover{transform:translateY(-2px);border-color:#626cf5;background:#292d38}.links strong{display:block;margin-bottom:5px}.links span{color:#9fa6b8;font-size:12px}
  .dashboard{width:min(720px,90vw);margin-top:auto;padding-top:30px}.dashboard h2{font-size:14px;color:#cdd1de;margin:0 0 10px}
  .stats{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}.stat{background:rgba(29,32,40,.92);border:1px solid #2d323e;border-radius:12px;padding:14px}.stat b{display:block;color:#7f8cff;font-size:19px;margin-bottom:4px}.stat span{color:#aeb4c2;font-size:12px;line-height:1.35}
  .mode{margin-top:15px;color:#858c9e;font-size:11px;text-align:center}
  @media(max-width:680px){.links{grid-template-columns:repeat(2,1fr)}.stats{grid-template-columns:1fr}.brand img{width:58px;height:58px}h1{font-size:30px}}
  </style></head><body><main class="page"><div class="brand"><img src="%1" alt="Open Kitten"><div><h1>Open Kitten</h1><div class="engine">%2</div></div></div>
  <form action="%3" method="get"><input name="q" autofocus autocomplete="off" placeholder="%4"><button type="submit">%5</button></form>
  <section class="links"><a href="%3"><strong>%6</strong><span>Recherche privée</span></a><a href="https://github.com/"><strong>GitHub</strong><span>Code et projets</span></a><a href="https://www.youtube.com/"><strong>YouTube</strong><span>Vidéos</span></a><a href="kitten://privacy"><strong>Kitten Shields</strong><span>Voir les protections</span></a></section>
  <section class="dashboard"><h2>%7</h2><div class="stats"><div class="stat"><b>01</b><span>%8</span></div><div class="stat"><b>HTTPS</b><span>%9</span></div><div class="stat"><b>SAFE</b><span>%10</span></div></div><div class="mode">%11 · %12</div></section></main></body></html>)HTML");
  return html.arg(logo_source)
      .arg(engine.toHtmlEscaped())
      .arg(search_endpoint(search_engine).toHtmlEscaped())
      .arg(search.toHtmlEscaped())
      .arg(ui_text(language, "Rechercher", "Search"))
      .arg(provider.toHtmlEscaped())
      .arg(stats_title.toHtmlEscaped())
      .arg(trackers.toHtmlEscaped())
      .arg(upgrades.toHtmlEscaped())
      .arg(safe_view.toHtmlEscaped())
      .arg(mode.toHtmlEscaped())
      .arg(description.toHtmlEscaped());
}

QString privacy_page(UiLanguage language) {
  const QString title = ui_text(language, "Protection Kitten", "Kitten protection");
  const QString summary = ui_text(
      language,
      "Une protection claire, locale et contrôlable pour chaque onglet.",
      "Clear, local protection that you control for every tab.");
  const QString body = ui_text(
      language,
      "Kitten Security Core nettoie les paramètres de pistage, bloque les hôtes de suivi connus et refuse les redirections HTTPS vers HTTP. Les permissions sensibles restent soumises à votre confirmation.",
      "Kitten Security Core removes tracking parameters, blocks known tracking hosts and refuses HTTPS-to-HTTP redirects. Sensitive permissions still require your confirmation.");
  return QString(R"HTML(<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>%1</title><style>
  :root{color-scheme:dark;font-family:Inter,"Segoe UI",Arial,sans-serif}*{box-sizing:border-box}body{margin:0;background:#101116;color:#f5f7ff}.wrap{width:min(820px,90vw);margin:8vh auto}.back{color:#aab1ff;text-decoration:none}.hero{margin:34px 0 24px}h1{font-size:38px;margin:0 0 10px}.lead{color:#aeb4c4;font-size:17px}.card{background:#1d2028;border:1px solid #303542;border-radius:16px;padding:22px;margin:13px 0}.card h2{font-size:17px;margin:0 0 8px}.card p{color:#b7bdca;line-height:1.55;margin:0}.pill{display:inline-block;color:#aeb7ff;background:#282d4e;border:1px solid #454e8a;border-radius:999px;padding:7px 11px;font-size:12px;font-weight:700}</style></head><body><main class="wrap"><a class="back" href="kitten://home">← Open Kitten</a><section class="hero"><span class="pill">KITTEN ENGINE 2.0</span><h1>%1</h1><p class="lead">%2</p></section><section class="card"><h2>Kitten Shields</h2><p>%3</p></section><section class="card"><h2>Kitten Safe View</h2><p>%4</p></section><section class="card"><h2>Chromium WebView2</h2><p>%5</p></section></main></body></html>)HTML")
      .arg(title.toHtmlEscaped(), summary.toHtmlEscaped(), body.toHtmlEscaped(),
           ui_text(language, "Mode de lecture C++ limité, sans JavaScript distant, pour consulter une page de manière isolée.", "Restricted C++ reading mode without remote JavaScript for isolated page viewing."),
           ui_text(language, "Compatibilité avec les sites modernes, les médias, WebAssembly et les applications Web.", "Compatibility with modern sites, media, WebAssembly and Web applications."));
}
}  // namespace

BrowserTab::BrowserTab(SecurityBridge* security, UiLanguage language,
                       bool private_mode, const QString& search_engine,
                       const QString& profile_name, bool password_autosave,
                       bool strict_tracking, bool prefer_safe_view,
                       QWidget* parent)
    : QWidget(parent), security_(security), language_(language),
      private_mode_(private_mode),
      prefer_safe_view_(prefer_safe_view),
      search_engine_(normalized_search_engine(search_engine)) {
  network_ = new QNetworkAccessManager(this);
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  view_stack_ = new QStackedWidget(this);
  auto* scroll = new QScrollArea(view_stack_);
  scroll->setWidgetResizable(true);
  view_ = new EngineView(scroll);
  scroll->setWidget(view_);
  modern_view_ = new ModernWebView(private_mode_, profile_name,
                                   password_autosave, strict_tracking,
                                   view_stack_);
  view_stack_->addWidget(scroll);
  view_stack_->addWidget(modern_view_);
  layout->addWidget(view_stack_);
  view_->set_link_handler([this](const std::string& href) {
    const std::string resolved = kitten::engine::resolve_url(current_url_.toStdString(), href);
    if (!resolved.empty()) navigate(QString::fromStdString(resolved));
  });
  modern_view_->set_navigation_policy(
      [this](const QString& target, bool redirected) {
        if (target.startsWith("kitten://", Qt::CaseInsensitive)) {
          QTimer::singleShot(0, this, [this, target] { navigate(target); });
          return false;
        }
        if (!kitten::engine::is_safe_web_url(target.toStdString())) return false;
        const QUrl destination(target);
        if (protection_enabled_ && security_ &&
            security_->is_tracking_host(destination.host())) return false;
        if (protection_enabled_ && redirected && QUrl(current_url_).scheme() == "https" &&
            destination.scheme() == "http") return false;
        return true;
      });
  modern_view_->set_resource_policy([this](const QString& target) {
    const QUrl resource(target);
    if (resource.scheme() != "http" && resource.scheme() != "https") return true;
    return !protection_enabled_ || !security_ ||
           !security_->is_tracking_host(resource.host());
  });
  modern_view_->set_new_window_handler([this](const QString& target) {
    if (new_tab_handler_) new_tab_handler_(target);
  });
  modern_view_->set_permission_handler(
      [this](const QString& origin, COREWEBVIEW2_PERMISSION_KIND kind,
             bool user_initiated) {
        if (!user_initiated) return false;
        QString permission;
        switch (kind) {
          case COREWEBVIEW2_PERMISSION_KIND_MICROPHONE:
            permission = ui_text(language_, "le microphone", "the microphone"); break;
          case COREWEBVIEW2_PERMISSION_KIND_CAMERA:
            permission = ui_text(language_, "la caméra", "the camera"); break;
          case COREWEBVIEW2_PERMISSION_KIND_GEOLOCATION:
            permission = ui_text(language_, "votre position", "your location"); break;
          case COREWEBVIEW2_PERMISSION_KIND_NOTIFICATIONS:
            permission = ui_text(language_, "les notifications", "notifications"); break;
          case COREWEBVIEW2_PERMISSION_KIND_CLIPBOARD_READ:
            permission = ui_text(language_, "le presse-papiers", "the clipboard"); break;
          case COREWEBVIEW2_PERMISSION_KIND_MULTIPLE_AUTOMATIC_DOWNLOADS:
            permission = ui_text(language_, "plusieurs téléchargements automatiques",
                                 "multiple automatic downloads"); break;
          case COREWEBVIEW2_PERMISSION_KIND_FILE_READ_WRITE:
            permission = ui_text(language_, "vos fichiers", "your files"); break;
          default:
            permission = ui_text(language_, "une permission sensible",
                                 "a sensitive permission"); break;
        }
        const QString site = QUrl(origin).host().isEmpty() ? origin : QUrl(origin).host();
        const QString question = ui_text(
            language_, "%1 souhaite utiliser %2. Autoriser pour cette demande ?",
            "%1 wants to use %2. Allow for this request?").arg(site, permission);
        return QMessageBox::question(
            this, ui_text(language_, "Autorisation du site", "Site permission"),
            question, QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No) == QMessageBox::Yes;
      });
  modern_view_->set_state_handler([this] {
    if (!modern_mode_) return;
    if (!modern_view_->url().isEmpty()) current_url_ = modern_view_->url();
    if (!modern_view_->title().isEmpty()) title_ = modern_view_->title();
    if (!modern_view_->error_message().isEmpty() && !modern_view_->is_ready() &&
        !modern_error_reported_) {
      modern_error_reported_ = true;
      show_error(modern_view_->error_message(), current_url_);
      return;
    }
    notify_state();
  });
}

BrowserTab::~BrowserTab() {
  if (reply_) reply_->abort();
}

QString BrowserTab::resolve_input(const QString& input) const {
  QString candidate = input.trimmed();
  if (candidate.isEmpty()) return {};
  if (candidate.startsWith("kitten://", Qt::CaseInsensitive)) return candidate;
  const bool looks_like_search = candidate.contains(QChar(' ')) ||
      (!candidate.contains("://") && !candidate.contains(QChar('.')) &&
       candidate.compare("localhost", Qt::CaseInsensitive) != 0);
  if (looks_like_search) {
    return search_url(search_engine_, candidate);
  }
  if (!candidate.contains("://")) candidate.prepend("https://");
  return candidate;
}

void BrowserTab::navigate(const QString& input, bool record_history) {
  if (reply_) {
    QPointer<QNetworkReply> previous = reply_;
    reply_.clear();
    previous->abort();
    previous->deleteLater();
    loading_ = false;
  }
  QString candidate = resolve_input(input);
  if (candidate == "kitten://home" || candidate == "kitten://privacy") {
    modern_mode_ = true;
    view_stack_->setCurrentIndex(1);
    if (candidate == "kitten://privacy") show_privacy();
    else show_home();
    if (record_history) {
      while (history_.size() > history_index_ + 1) history_.removeLast();
      if (history_.isEmpty() || history_.last() != candidate) history_.append(candidate);
      history_index_ = history_.size() - 1;
    }
    notify_state();
    return;
  }
  if (protection_enabled_ && security_) candidate = security_->clean_url(candidate);
  if (!kitten::engine::is_safe_web_url(candidate.toStdString())) {
    show_error(ui_text(language_, "Adresse HTTP/HTTPS non valide",
                       "Invalid HTTP/HTTPS address"), candidate);
    return;
  }
  const QUrl parsed(candidate);
  if (protection_enabled_ && security_ && security_->is_tracking_host(parsed.host())) {
    show_error(ui_text(language_, "Domaine de suivi bloqué par Kitten Security Core",
                       "Tracking domain blocked by Kitten Security Core"), candidate);
    return;
  }
  if (prefer_safe_view_) {
    modern_mode_ = false;
    view_stack_->setCurrentIndex(0);
    start_request(candidate, record_history, 0);
    return;
  }
  modern_mode_ = true;
  modern_error_reported_ = false;
  view_stack_->setCurrentIndex(1);
  modern_view_->navigate(candidate);
  notify_state();
}

void BrowserTab::start_request(const QString& input, bool record_history,
                               int redirect_count) {
  QString candidate = protection_enabled_ && security_
      ? security_->clean_url(input) : input;
  if (!kitten::engine::is_safe_web_url(candidate.toStdString())) {
    loading_ = false;
    show_error(ui_text(language_, "Adresse HTTP/HTTPS non valide",
                       "Invalid HTTP/HTTPS address"), candidate);
    return;
  }
  const QUrl parsed(candidate);
  if (protection_enabled_ && security_ && security_->is_tracking_host(parsed.host())) {
    loading_ = false;
    show_error(ui_text(language_, "Domaine de suivi bloqué par Kitten Security Core",
                       "Tracking domain blocked by Kitten Security Core"), candidate);
    return;
  }
  QNetworkRequest request{parsed};
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                       QNetworkRequest::ManualRedirectPolicy);
  request.setTransferTimeout(30000);
  request.setRawHeader("User-Agent", "Open-Kitten/1.0 Kitten-Engine/1.0");
  request.setRawHeader("Accept", "text/html,application/xhtml+xml,text/plain;q=0.9,*/*;q=0.1");
  reply_ = network_->get(request);
  reply_too_large_ = false;
  loading_ = true;
  notify_state();
  connect(reply_.data(), &QNetworkReply::downloadProgress, this,
          [this, item = reply_.data()](qint64 received, qint64 total) {
            if ((received > kMaximumPageBytes || total > kMaximumPageBytes) &&
                item == reply_.data()) {
              reply_too_large_ = true;
              item->abort();
            }
          });
  connect(reply_.data(), &QNetworkReply::finished, this,
          [this, item = reply_.data(), record_history, redirect_count] {
            finish_navigation(item, record_history, redirect_count);
          });
}

void BrowserTab::finish_navigation(QNetworkReply* reply, bool record_history,
                                   int redirect_count) {
  if (!reply || reply != reply_.data()) {
    if (reply) reply->deleteLater();
    return;
  }
  const bool too_large = reply_too_large_;
  const QUrl source_url = reply->url();
  const QUrl redirect = reply->attribute(
      QNetworkRequest::RedirectionTargetAttribute).toUrl();
  reply_.clear();
  reply_too_large_ = false;

  if (redirect.isValid() && !redirect.isEmpty()) {
    const QUrl target = source_url.resolved(redirect);
    const QString target_url = target.toString();
    reply->deleteLater();
    if (redirect_count >= kMaximumRedirects) {
      loading_ = false;
      show_error(ui_text(language_, "Trop de redirections", "Too many redirects"),
                 target_url);
      return;
    }
    if (source_url.scheme() == "https" && target.scheme() == "http") {
      loading_ = false;
      show_error(ui_text(language_, "Redirection non sécurisée bloquée",
                         "Insecure redirect blocked"), target_url);
      return;
    }
    start_request(target_url, record_history, redirect_count + 1);
    return;
  }

  loading_ = false;
  const QString final_url = reply->url().toString();
  const QByteArray content_type = reply->rawHeader("Content-Type").toLower();
  const QByteArray body = reply->readAll();
  const bool supported_type = content_type.isEmpty() || content_type.contains("text/html") ||
                              content_type.contains("application/xhtml") ||
                              content_type.contains("text/plain");
  const bool ok = reply->error() == QNetworkReply::NoError &&
                  body.size() <= kMaximumPageBytes && supported_type &&
                  kitten::engine::is_safe_web_url(final_url.toStdString());
  const QString error = reply->errorString();
  reply->deleteLater();
  const bool tracking_host = protection_enabled_ && security_ &&
      security_->is_tracking_host(QUrl(final_url).host());
  if (!ok || tracking_host || too_large) {
    QString message = error;
    if (too_large) {
      message = ui_text(language_, "Page trop volumineuse (limite de 5 Mio)",
                        "Page is too large (5 MiB limit)");
    } else if (tracking_host) {
      message = ui_text(language_, "Domaine de suivi bloqué par Kitten Security Core",
                        "Tracking domain blocked by Kitten Security Core");
    } else if (!supported_type) {
      message = ui_text(language_, "Type de contenu non pris en charge",
                        "Unsupported content type");
    }
    show_error(message, final_url);
    return;
  }
  show_html(body, final_url);
  if (record_history) {
    while (history_.size() > history_index_ + 1) history_.removeLast();
    if (history_.isEmpty() || history_.last() != final_url) history_.append(final_url);
    history_index_ = history_.size() - 1;
  }
  notify_state();
}

void BrowserTab::show_html(const QByteArray& source, const QString& final_url) {
  modern_mode_ = false;
  view_stack_->setCurrentIndex(0);
  auto document = kitten::engine::parse_html(
      std::string_view(source.constData(), static_cast<std::size_t>(source.size())));
  std::string css;
  collect_css(*document.root, css);
  title_ = document.title.empty() ? QUrl(final_url).host()
                                  : QString::fromStdString(document.title).simplified();
  if (title_.isEmpty()) title_ = "Open Kitten";
  current_url_ = final_url;
  view_->set_document(std::move(document), kitten::engine::parse_css(css));
}

void BrowserTab::show_home() {
  title_ = ui_text(language_, "Nouvel onglet", "New tab");
  current_url_ = "kitten://home";
  modern_mode_ = true;
  view_stack_->setCurrentIndex(1);
  modern_view_->set_html(home_page(language_, private_mode_, search_engine_),
                         current_url_, title_);
}

void BrowserTab::show_privacy() {
  title_ = ui_text(language_, "Protection Kitten", "Kitten protection");
  current_url_ = "kitten://privacy";
  modern_mode_ = true;
  view_stack_->setCurrentIndex(1);
  modern_view_->set_html(privacy_page(language_), current_url_, title_);
}

void BrowserTab::show_error(const QString& message, const QString& url) {
  const QString error_title = ui_text(language_, "Erreur", "Error");
  const QString unavailable = ui_text(language_, "Page indisponible", "Page unavailable");
  const QByteArray html = QString("<title>%1</title><h1>%2</h1><p>%3</p><p>%4</p>")
      .arg(error_title, unavailable, message.toHtmlEscaped(), url.toHtmlEscaped()).toUtf8();
  show_html(html, url.isEmpty() ? current_url_ : url);
  notify_state();
}

void BrowserTab::go_back() {
  if (modern_mode_) {
    modern_view_->go_back();
    return;
  }
  if (!can_go_back()) return;
  --history_index_;
  navigate(history_.at(history_index_), false);
}
void BrowserTab::go_forward() {
  if (modern_mode_) {
    modern_view_->go_forward();
    return;
  }
  if (!can_go_forward()) return;
  ++history_index_;
  navigate(history_.at(history_index_), false);
}
void BrowserTab::reload() {
  if (modern_mode_) modern_view_->reload();
  else navigate(current_url_, false);
}
void BrowserTab::stop() {
  if (modern_mode_) {
    modern_view_->stop();
    notify_state();
    return;
  }
  if (!reply_) return;
  QPointer<QNetworkReply> previous = reply_;
  reply_.clear();
  loading_ = false;
  reply_too_large_ = false;
  previous->abort();
  previous->deleteLater();
  notify_state();
}
void BrowserTab::set_language(UiLanguage language) {
  language_ = language;
  if (current_url_ == "kitten://home") show_home();
  else if (current_url_ == "kitten://privacy") show_privacy();
  notify_state();
}
void BrowserTab::set_search_engine(const QString& search_engine) {
  search_engine_ = normalized_search_engine(search_engine);
  if (current_url_ == "kitten://home") show_home();
  notify_state();
}
void BrowserTab::set_password_autosave(bool enabled) {
  modern_view_->set_password_autosave(enabled);
}
void BrowserTab::set_tracking_prevention(bool strict) {
  modern_view_->set_tracking_prevention(strict);
}
void BrowserTab::set_prefer_safe_view(bool enabled) {
  prefer_safe_view_ = enabled;
}
void BrowserTab::clear_browsing_data(std::function<void(bool)> completion) {
  modern_view_->clear_browsing_data(std::move(completion));
}
void BrowserTab::install_extension(
    const QString& folder,
    std::function<void(bool, const QString&)> completion) {
  modern_view_->install_extension(folder, std::move(completion));
}
void BrowserTab::set_state_handler(std::function<void()> handler) { state_handler_ = std::move(handler); }
void BrowserTab::set_new_tab_handler(
    std::function<void(const QString&)> handler) {
  new_tab_handler_ = std::move(handler);
}
void BrowserTab::zoom_in() {
  if (modern_mode_) modern_view_->set_zoom_factor(modern_view_->zoom_factor() + 0.1);
  else view_->set_zoom_factor(view_->zoom_factor() + 0.1F);
}
void BrowserTab::zoom_out() {
  if (modern_mode_) modern_view_->set_zoom_factor(modern_view_->zoom_factor() - 0.1);
  else view_->set_zoom_factor(view_->zoom_factor() - 0.1F);
}
void BrowserTab::reset_zoom() {
  if (modern_mode_) modern_view_->set_zoom_factor(1.0);
  else view_->set_zoom_factor(1.0F);
}
void BrowserTab::use_safe_view() {
  if (current_url_.startsWith("http://") || current_url_.startsWith("https://")) {
    modern_mode_ = false;
    view_stack_->setCurrentIndex(0);
    start_request(current_url_, false, 0);
  }
}
void BrowserTab::use_kitten_engine() {
  prefer_safe_view_ = false;
  if (current_url_.startsWith("http://") || current_url_.startsWith("https://")) {
    navigate(current_url_, false);
  }
}
void BrowserTab::print_page() {
  if (modern_mode_) modern_view_->print_page();
}
void BrowserTab::show_downloads() {
  if (modern_mode_) modern_view_->show_downloads();
}
void BrowserTab::open_developer_tools() {
  if (modern_mode_) modern_view_->open_developer_tools();
}
void BrowserTab::find_text(const QString& query) {
  if (modern_mode_) modern_view_->find_text(query);
}
void BrowserTab::set_protection_enabled(bool enabled) {
  if (protection_enabled_ == enabled) return;
  protection_enabled_ = enabled;
  if (modern_mode_ && current_url_.startsWith("http")) modern_view_->reload();
  notify_state();
}
QString BrowserTab::url() const { return current_url_; }
QString BrowserTab::title() const { return title_; }
bool BrowserTab::can_go_back() const {
  return modern_mode_ ? modern_view_->can_go_back() : history_index_ > 0;
}
bool BrowserTab::can_go_forward() const {
  return modern_mode_ ? modern_view_->can_go_forward()
      : history_index_ >= 0 && history_index_ + 1 < history_.size();
}
bool BrowserTab::is_loading() const {
  return modern_mode_ ? modern_view_->is_loading() : loading_;
}
int BrowserTab::zoom_percent() const {
  return modern_mode_
      ? static_cast<int>(modern_view_->zoom_factor() * 100.0 + 0.5)
      : static_cast<int>(view_->zoom_factor() * 100.0F + 0.5F);
}
bool BrowserTab::uses_modern_engine() const { return modern_mode_; }
bool BrowserTab::modern_engine_ready() const { return modern_view_->is_ready(); }
bool BrowserTab::protection_enabled() const { return protection_enabled_; }
int BrowserTab::blocked_resource_count() const {
  return modern_mode_ ? modern_view_->blocked_resource_count() : 0;
}
void BrowserTab::notify_state() { if (state_handler_) state_handler_(); }
