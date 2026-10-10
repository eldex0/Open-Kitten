#include "browser/browser_window.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QTabBar>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <cmath>

#include "browser/browser_tab.h"
#include "browser/security_bridge.h"

namespace {

QString normalized_profile_name(QString name) {
  name = name.trimmed().left(48);
  name.remove(QRegularExpression(R"([\\/:*?"<>|])"));
  return name.isEmpty() ? QStringLiteral("Default") : name;
}

QString settings_application(const QString& profile_name) {
  const QString normalized = normalized_profile_name(profile_name);
  return normalized == "Default" ? QStringLiteral("OpenKitten")
                                  : QStringLiteral("OpenKitten-") + normalized;
}

QJsonArray read_history(QSettings& settings) {
  const QByteArray data = settings.value("history/items").toByteArray();
  const QJsonDocument document = QJsonDocument::fromJson(data);
  return document.isArray() ? document.array() : QJsonArray{};
}

void write_history(QSettings& settings, const QJsonArray& history) {
  settings.setValue("history/items",
                    QJsonDocument(history).toJson(QJsonDocument::Compact));
}

void collect_bookmark_urls(const QJsonValue& value, QStringList& urls) {
  if (value.isArray()) {
    for (const QJsonValue& child : value.toArray()) collect_bookmark_urls(child, urls);
    return;
  }
  if (!value.isObject()) return;
  const QJsonObject object = value.toObject();
  if (object.value("type").toString() == "url") {
    const QString url = object.value("url").toString();
    if (url.startsWith("http://") || url.startsWith("https://")) urls.append(url);
  }
  for (auto item = object.begin(); item != object.end(); ++item) {
    if (item.value().isArray() || item.value().isObject()) {
      collect_bookmark_urls(item.value(), urls);
    }
  }
}

enum class ChromeIcon { back, forward, reload, stop, home, shield, star,
                        star_filled, download, menu, plus, close };

QIcon chrome_icon(ChromeIcon glyph) {
  QPixmap pixmap(24, 24);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);
  const QColor color = glyph == ChromeIcon::shield || glyph == ChromeIcon::star_filled
      ? QColor("#a9b2ff") : QColor("#e7eaf0");
  QPen pen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  painter.setPen(pen);
  painter.setBrush(Qt::NoBrush);
  switch (glyph) {
    case ChromeIcon::back:
      painter.drawPolyline(QPolygonF{{15, 6}, {9, 12}, {15, 18}});
      break;
    case ChromeIcon::forward:
      painter.drawPolyline(QPolygonF{{9, 6}, {15, 12}, {9, 18}});
      break;
    case ChromeIcon::reload:
      painter.drawArc(QRectF(5, 5, 14, 14), 35 * 16, 285 * 16);
      painter.drawPolyline(QPolygonF{{17, 4}, {19, 10}, {13, 9}});
      break;
    case ChromeIcon::stop:
      painter.setBrush(color);
      painter.drawRoundedRect(QRectF(7, 7, 10, 10), 1, 1);
      break;
    case ChromeIcon::home:
      painter.drawPolyline(QPolygonF{{4, 11}, {12, 4}, {20, 11}});
      painter.drawRect(QRectF(6.5, 10, 11, 10));
      painter.drawRect(QRectF(10, 14, 4, 6));
      break;
    case ChromeIcon::shield: {
      QPainterPath path;
      path.moveTo(12, 3); path.lineTo(19, 6); path.lineTo(19, 11);
      path.cubicTo(19, 16, 16, 19, 12, 21);
      path.cubicTo(8, 19, 5, 16, 5, 11); path.lineTo(5, 6); path.closeSubpath();
      painter.drawPath(path);
      painter.drawPolyline(QPolygonF{{8.5, 12}, {11, 14.5}, {15.8, 9}});
      break;
    }
    case ChromeIcon::star:
    case ChromeIcon::star_filled: {
      QPolygonF star;
      for (int point = 0; point < 10; ++point) {
        const double angle = (-90.0 + point * 36.0) * 3.141592653589793 / 180.0;
        const double radius = point % 2 == 0 ? 8.5 : 3.8;
        star << QPointF(12 + std::cos(angle) * radius,
                        12 + std::sin(angle) * radius);
      }
      if (glyph == ChromeIcon::star_filled) painter.setBrush(color);
      painter.drawPolygon(star);
      break;
    }
    case ChromeIcon::download:
      painter.drawLine(QPointF(12, 4), QPointF(12, 15));
      painter.drawPolyline(QPolygonF{{7.5, 11}, {12, 15.5}, {16.5, 11}});
      painter.drawLine(QPointF(5, 19), QPointF(19, 19));
      break;
    case ChromeIcon::menu:
      painter.drawLine(QPointF(5, 7), QPointF(19, 7));
      painter.drawLine(QPointF(5, 12), QPointF(19, 12));
      painter.drawLine(QPointF(5, 17), QPointF(19, 17));
      break;
    case ChromeIcon::plus:
      painter.drawLine(QPointF(12, 5), QPointF(12, 19));
      painter.drawLine(QPointF(5, 12), QPointF(19, 12));
      break;
    case ChromeIcon::close:
      painter.drawLine(QPointF(7, 7), QPointF(17, 17));
      painter.drawLine(QPointF(17, 7), QPointF(7, 17));
      break;
  }
  return QIcon(pixmap);
}

QToolButton* action_button(QAction* action, QWidget* parent) {
  auto* button = new QToolButton(parent);
  button->setDefaultAction(action);
  button->setIconSize(QSize(18, 18));
  button->setFixedSize(34, 34);
  return button;
}

}  // namespace

BrowserWindow::BrowserWindow(bool private_mode, const QString& profile_name)
    : settings_("OpenKitten", settings_application(profile_name)),
      profile_name_(normalized_profile_name(profile_name)),
      private_mode_(private_mode) {
  const QString saved_language = settings_.value("ui/language").toString();
  language_ = saved_language.isEmpty()
      ? language_from_locale(QLocale::system().language())
      : language_from_code(saved_language);
  prefer_safe_view_ = settings_.value("engine/preferSafeView", false).toBool();
  search_engine_ = settings_.value("search/engine", "duckduckgo").toString();
  if (search_engine_ != "brave" && search_engine_ != "qwant") {
    search_engine_ = "duckduckgo";
  }

  setWindowTitle("Open Kitten 2.0");
  setMinimumSize(760, 520);
  resize(1180, 800);
  security_ = new SecurityBridge();

  auto* shell = new QWidget(this);
  shell->setObjectName("browserShell");
  auto* shell_layout = new QVBoxLayout(shell);
  shell_layout->setContentsMargins(0, 0, 0, 0);
  shell_layout->setSpacing(0);

  auto* tab_strip = new QWidget(shell);
  tab_strip->setObjectName("tabStrip");
  tab_strip->setFixedHeight(48);
  auto* tab_layout = new QHBoxLayout(tab_strip);
  tab_layout->setContentsMargins(8, 5, 8, 0);
  tab_layout->setSpacing(4);
  tab_bar_ = new QTabBar(tab_strip);
  tab_bar_->setDocumentMode(true);
  tab_bar_->setTabsClosable(false);
  tab_bar_->setMovable(true);
  tab_bar_->setExpanding(false);
  tab_bar_->setElideMode(Qt::ElideRight);
  tab_bar_->setUsesScrollButtons(true);
  tab_bar_->setContextMenuPolicy(Qt::CustomContextMenu);
  tab_bar_->setFixedHeight(42);
  tab_layout->addWidget(tab_bar_);
  auto* add_tab_button = new QToolButton(tab_strip);
  add_tab_button->setObjectName("addTabButton");
  add_tab_button->setIcon(chrome_icon(ChromeIcon::plus));
  add_tab_button->setIconSize(QSize(18, 18));
  add_tab_button->setAccessibleName("New tab");
  add_tab_button->setFixedSize(34, 34);
  tab_layout->addWidget(add_tab_button);
  tab_layout->addStretch(1);
  shell_layout->addWidget(tab_strip);

  toolbar_ = new QWidget(shell);
  toolbar_->setObjectName("navigationToolbar");
  toolbar_->setFixedHeight(58);
  auto* navigation_layout = new QHBoxLayout(toolbar_);
  navigation_layout->setContentsMargins(8, 6, 8, 6);
  navigation_layout->setSpacing(4);
  back_ = new QAction(chrome_icon(ChromeIcon::back), QString{}, this);
  forward_ = new QAction(chrome_icon(ChromeIcon::forward), QString{}, this);
  reload_ = new QAction(chrome_icon(ChromeIcon::reload), QString{}, this);
  home_ = new QAction(chrome_icon(ChromeIcon::home), QString{}, this);
  navigation_layout->addWidget(action_button(back_, toolbar_));
  navigation_layout->addWidget(action_button(forward_, toolbar_));
  navigation_layout->addWidget(action_button(reload_, toolbar_));
  navigation_layout->addWidget(action_button(home_, toolbar_));
  security_badge_ = new QToolButton(toolbar_);
  security_badge_->setObjectName("securityBadge");
  security_badge_->setIcon(chrome_icon(ChromeIcon::shield));
  security_badge_->setIconSize(QSize(18, 18));
  address_ = new QLineEdit(toolbar_);
  address_->setObjectName("addressBar");
  address_->setClearButtonEnabled(true);
  address_->setMinimumWidth(320);
  address_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  navigation_layout->addWidget(security_badge_);
  navigation_layout->addWidget(address_, 1);
  bookmark_ = new QAction(chrome_icon(ChromeIcon::star), QString{}, this);
  navigation_layout->addWidget(action_button(bookmark_, toolbar_));
  downloads_ = new QAction(chrome_icon(ChromeIcon::download), QString{}, this);
  navigation_layout->addWidget(action_button(downloads_, toolbar_));
  menu_button_ = new QToolButton(toolbar_);
  menu_button_->setObjectName("mainMenuButton");
  menu_button_->setIcon(chrome_icon(ChromeIcon::menu));
  menu_button_->setIconSize(QSize(19, 19));
  menu_button_->setPopupMode(QToolButton::InstantPopup);
  menu_button_->setFixedSize(34, 34);
  navigation_layout->addWidget(menu_button_);
  shell_layout->addWidget(toolbar_);

  bookmarks_bar_ = new QToolBar(shell);
  bookmarks_bar_->setObjectName("bookmarksToolbar");
  bookmarks_bar_->setMovable(false);
  bookmarks_bar_->setIconSize(QSize(16, 16));
  shell_layout->addWidget(bookmarks_bar_);

  back_->setShortcut(QKeySequence("Alt+Left"));
  forward_->setShortcut(QKeySequence("Alt+Right"));
  reload_->setShortcut(QKeySequence("F5"));
  home_->setShortcut(QKeySequence("Alt+Home"));
  bookmark_->setShortcut(QKeySequence("Ctrl+D"));

  tab_stack_ = new QStackedWidget(shell);
  tab_stack_->setObjectName("pageStack");
  shell_layout->addWidget(tab_stack_, 1);
  setCentralWidget(shell);
  menuBar()->hide();
  statusBar()->hide();

  main_menu_ = new QMenu(this);
  menu_button_->setMenu(main_menu_);
  file_menu_ = new QMenu(this);
  new_tab_ = file_menu_->addAction(QString{});
  new_tab_->setIcon(chrome_icon(ChromeIcon::plus));
  new_tab_->setShortcut(QKeySequence::AddTab);
  connect(add_tab_button, &QToolButton::clicked, new_tab_, &QAction::trigger);
  new_private_window_ = file_menu_->addAction(QString{});
  new_private_window_->setShortcut(QKeySequence("Ctrl+Shift+N"));
  new_profile_window_ = file_menu_->addAction(QString{});
  close_current_ = file_menu_->addAction(QString{});
  close_current_->setShortcut(QKeySequence::Close);
  reopen_closed_ = file_menu_->addAction(QString{});
  reopen_closed_->setShortcut(QKeySequence("Ctrl+Shift+T"));
  history_ = file_menu_->addAction(QString{});
  history_->setShortcut(QKeySequence("Ctrl+H"));
  file_menu_->addSeparator();
  quit_ = file_menu_->addAction(QString{});
  quit_->setShortcut(QKeySequence::Quit);
  bookmarks_menu_ = new QMenu(this);
  import_bookmarks_ = new QAction(this);
  bookmarks_menu_->addAction(import_bookmarks_);
  bookmarks_menu_->addSeparator();
  view_menu_ = new QMenu(this);
  zoom_in_ = view_menu_->addAction(QString{});
  zoom_in_->setShortcut(QKeySequence::ZoomIn);
  zoom_out_ = view_menu_->addAction(QString{});
  zoom_out_->setShortcut(QKeySequence::ZoomOut);
  zoom_reset_ = view_menu_->addAction(QString{});
  zoom_reset_->setShortcut(QKeySequence("Ctrl+0"));
  find_ = view_menu_->addAction(QString{});
  find_->setShortcut(QKeySequence::Find);
  print_ = view_menu_->addAction(QString{});
  print_->setShortcut(QKeySequence::Print);
  view_menu_->addAction(downloads_);
  downloads_->setShortcut(QKeySequence("Ctrl+J"));
  developer_tools_ = view_menu_->addAction(QString{});
  developer_tools_->setShortcut(QKeySequence("F12"));
  view_menu_->addSeparator();
  fullscreen_ = view_menu_->addAction(QString{});
  fullscreen_->setShortcut(QKeySequence("F11"));
  fullscreen_->setCheckable(true);
  kitten_engine_ = view_menu_->addAction(QString{});
  safe_view_ = view_menu_->addAction(QString{});
  settings_menu_ = new QMenu(this);
  open_settings_ = settings_menu_->addAction(QString{});
  open_settings_->setShortcut(QKeySequence("Ctrl+,"));
  install_extension_ = settings_menu_->addAction(QString{});
  export_profile_ = settings_menu_->addAction(QString{});
  import_profile_ = settings_menu_->addAction(QString{});
  settings_menu_->addSeparator();
  language_menu_ = settings_menu_->addMenu(QString{});
  auto* languages = new QActionGroup(this);
  languages->setExclusive(true);
  for (UiLanguage language : supported_languages()) {
    auto* action = language_menu_->addAction(language_name(language));
    action->setCheckable(true);
    action->setData(language_code(language));
    languages->addAction(action);
    language_actions_.append(action);
    connect(action, &QAction::triggered, this, [this, language] {
      set_language(language);
    });
  }
  search_engine_menu_ = settings_menu_->addMenu(QString{});
  duckduckgo_ = search_engine_menu_->addAction("DuckDuckGo");
  brave_search_ = search_engine_menu_->addAction("Brave Search");
  qwant_ = search_engine_menu_->addAction("Qwant");
  duckduckgo_->setCheckable(true);
  brave_search_->setCheckable(true);
  qwant_->setCheckable(true);
  auto* search_engines = new QActionGroup(this);
  search_engines->setExclusive(true);
  search_engines->addAction(duckduckgo_);
  search_engines->addAction(brave_search_);
  search_engines->addAction(qwant_);
  help_menu_ = new QMenu(this);
  about_ = help_menu_->addAction(QString{});
  main_menu_->addMenu(file_menu_);
  main_menu_->addMenu(bookmarks_menu_);
  main_menu_->addMenu(view_menu_);
  main_menu_->addMenu(settings_menu_);
  main_menu_->addSeparator();
  main_menu_->addMenu(help_menu_);

  auto* focus_address = new QAction(this);
  focus_address->setShortcut(QKeySequence("Ctrl+L"));
  addAction(focus_address);

  connect(back_, &QAction::triggered, this, [this] {
    if (current_tab()) current_tab()->go_back();
  });
  connect(forward_, &QAction::triggered, this, [this] {
    if (current_tab()) current_tab()->go_forward();
  });
  connect(reload_, &QAction::triggered, this, [this] {
    if (!current_tab()) return;
    current_tab()->is_loading() ? current_tab()->stop() : current_tab()->reload();
  });
  connect(home_, &QAction::triggered, this, [this] {
    if (current_tab()) current_tab()->navigate("kitten://home");
  });
  connect(bookmark_, &QAction::triggered, this, [this] { add_bookmark(); });
  connect(security_badge_, &QToolButton::clicked, this,
          [this] { show_protection_panel(); });
  connect(address_, &QLineEdit::returnPressed, this, [this] {
    if (current_tab()) current_tab()->navigate(address_->text());
  });
  connect(new_tab_, &QAction::triggered, this, [this] { add_tab(); });
  connect(new_private_window_, &QAction::triggered, this, [this] {
    auto* window = new BrowserWindow(true, profile_name_);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->show();
  });
  connect(new_profile_window_, &QAction::triggered, this, [this] {
    bool accepted = false;
    const QString profile = QInputDialog::getText(
        this, text("Nouveau profil", "New profile"),
        text("Nom du profil", "Profile name"), QLineEdit::Normal,
        QString{}, &accepted);
    if (!accepted || profile.trimmed().isEmpty()) return;
    auto* window = new BrowserWindow(false, profile);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->show();
  });
  connect(close_current_, &QAction::triggered, this,
          [this] { close_tab(tab_bar_->currentIndex()); });
  connect(reopen_closed_, &QAction::triggered, this,
          [this] { reopen_closed_tab(); });
  connect(history_, &QAction::triggered, this, [this] { show_history(); });
  connect(open_settings_, &QAction::triggered, this,
          [this] { show_settings_dialog(); });
  connect(import_bookmarks_, &QAction::triggered, this,
          [this] { import_bookmarks(); });
  connect(install_extension_, &QAction::triggered, this,
          [this] { install_extension(); });
  connect(export_profile_, &QAction::triggered, this,
          [this] { export_profile(); });
  connect(import_profile_, &QAction::triggered, this,
          [this] { import_profile(); });
  connect(zoom_in_, &QAction::triggered, this, [this] { change_zoom(1); });
  connect(zoom_out_, &QAction::triggered, this, [this] { change_zoom(-1); });
  connect(zoom_reset_, &QAction::triggered, this, [this] { change_zoom(0); });
  connect(find_, &QAction::triggered, this, [this] {
    bool accepted = false;
    const QString query = QInputDialog::getText(
        this, text("Rechercher dans la page", "Find in page"),
        text("Texte à rechercher", "Search text"), QLineEdit::Normal,
        QString{}, &accepted);
    if (accepted && current_tab()) current_tab()->find_text(query);
  });
  connect(print_, &QAction::triggered, this, [this] {
    if (current_tab()) current_tab()->print_page();
  });
  connect(downloads_, &QAction::triggered, this, [this] {
    if (current_tab()) current_tab()->show_downloads();
  });
  connect(developer_tools_, &QAction::triggered, this, [this] {
    if (current_tab()) current_tab()->open_developer_tools();
  });
  connect(fullscreen_, &QAction::toggled, this, [this](bool enabled) {
    if (enabled) showFullScreen();
    else showNormal();
  });
  connect(safe_view_, &QAction::triggered, this, [this] {
    if (current_tab()) current_tab()->use_safe_view();
  });
  connect(kitten_engine_, &QAction::triggered, this, [this] {
    prefer_safe_view_ = false;
    settings_.setValue("engine/preferSafeView", false);
    if (current_tab()) current_tab()->use_kitten_engine();
  });
  connect(quit_, &QAction::triggered, qApp, &QApplication::quit);
  connect(tab_bar_, &QTabBar::tabCloseRequested, this,
          [this](int index) { close_tab(index); });
  connect(tab_bar_, &QTabBar::currentChanged, this, [this](int index) {
    tab_stack_->setCurrentIndex(index);
    update_chrome();
  });
  connect(tab_bar_, &QTabBar::tabMoved, this, [this](int from, int to) {
    QWidget* page = tab_stack_->widget(from);
    if (!page) return;
    tab_stack_->removeWidget(page);
    tab_stack_->insertWidget(to, page);
    tab_stack_->setCurrentIndex(tab_bar_->currentIndex());
  });
  connect(tab_bar_, &QTabBar::customContextMenuRequested, this,
          [this](const QPoint& position) {
    const int index = tab_bar_->tabAt(position);
    if (index < 0) return;
    QMenu menu(this);
    QAction* reload_tab = menu.addAction(text("Actualiser", "Reload"));
    QAction* duplicate_tab = menu.addAction(text("Dupliquer l’onglet", "Duplicate tab"));
    auto* selected_tab = static_cast<BrowserTab*>(tab_stack_->widget(index));
    QAction* pin_tab = menu.addAction(pinned_tabs_.contains(selected_tab)
        ? text("Désépingler l’onglet", "Unpin tab")
        : text("Épingler l’onglet", "Pin tab"));
    QAction* group_tab = menu.addAction(text("Ajouter à un groupe…", "Add to group…"));
    QAction* remove_group = menu.addAction(text("Retirer du groupe", "Remove from group"));
    remove_group->setEnabled(tab_groups_.contains(selected_tab));
    menu.addSeparator();
    QAction* close_selected = menu.addAction(text("Fermer l’onglet", "Close tab"));
    QAction* close_others = menu.addAction(text("Fermer les autres onglets", "Close other tabs"));
    QAction* close_right = menu.addAction(text("Fermer les onglets à droite", "Close tabs to the right"));
    close_others->setEnabled(tab_bar_->count() > 1);
    close_right->setEnabled(index + 1 < tab_bar_->count());
    QAction* selected = menu.exec(tab_bar_->mapToGlobal(position));
    if (selected == reload_tab) {
      if (auto* tab = static_cast<BrowserTab*>(tab_stack_->widget(index))) tab->reload();
    } else if (selected == duplicate_tab) {
      if (auto* tab = static_cast<BrowserTab*>(tab_stack_->widget(index))) {
        add_tab(tab->url(), true);
      }
    } else if (selected == pin_tab) {
      set_tab_pinned(selected_tab, !pinned_tabs_.contains(selected_tab));
    } else if (selected == group_tab) {
      bool accepted = false;
      const QString group = QInputDialog::getText(
          this, text("Groupe d’onglets", "Tab group"),
          text("Nom du groupe", "Group name"), QLineEdit::Normal,
          tab_groups_.value(selected_tab), &accepted).trimmed();
      if (accepted && !group.isEmpty()) {
        tab_groups_.insert(selected_tab, group.left(24));
        update_tab_presentation(selected_tab);
      }
    } else if (selected == remove_group) {
      tab_groups_.remove(selected_tab);
      update_tab_presentation(selected_tab);
    } else if (selected == close_selected) {
      close_tab(index);
    } else if (selected == close_others) {
      for (int candidate = tab_bar_->count() - 1; candidate >= 0; --candidate) {
        if (candidate != index) close_tab(candidate);
      }
    } else if (selected == close_right) {
      for (int candidate = tab_bar_->count() - 1; candidate > index; --candidate) {
        close_tab(candidate);
      }
    }
  });
  connect(about_, &QAction::triggered, this, [this] {
    QMessageBox::about(this, "Open Kitten",
      text("Open Kitten 2.0.0\nKitten Engine 2.0.0\n\n"
           "Navigateur natif C++20 avec protections Rust.\n"
           "Chromium pour le Web moderne et Kitten Safe View.",
           "Open Kitten 2.0.0\nKitten Engine 2.0.0\n\n"
           "Native C++20 browser with Rust protections.\n"
           "Chromium for the modern Web and Kitten Safe View."));
  });
  connect(duckduckgo_, &QAction::triggered, this,
          [this] { set_search_engine("duckduckgo"); });
  connect(brave_search_, &QAction::triggered, this,
          [this] { set_search_engine("brave"); });
  connect(qwant_, &QAction::triggered, this,
          [this] { set_search_engine("qwant"); });
  connect(focus_address, &QAction::triggered, this, [this] {
    address_->setFocus();
    address_->selectAll();
  });

  apply_theme();
  apply_language();
  if (private_mode_) add_tab();
  else restore_session();
  const QByteArray geometry = settings_.value("window/geometry").toByteArray();
  if (!geometry.isEmpty()) restoreGeometry(geometry);
  update_chrome();
}

BrowserWindow::~BrowserWindow() { delete security_; }

bool BrowserWindow::modern_engine_ready() const {
  return current_tab() && current_tab()->modern_engine_ready();
}

BrowserTab* BrowserWindow::current_tab() const {
  return tab_stack_->currentWidget()
      ? static_cast<BrowserTab*>(tab_stack_->currentWidget()) : nullptr;
}

BrowserTab* BrowserWindow::add_tab(const QString& url, bool activate) {
  auto* tab = new BrowserTab(security_, language_, private_mode_,
                             search_engine_, profile_name_,
                             settings_.value("privacy/passwordAutosave", true).toBool(),
                             settings_.value("privacy/strictTracking", false).toBool(),
                             prefer_safe_view_,
                             tab_stack_);
  const int index = tab_stack_->addWidget(tab);
  tab_bar_->insertTab(index, text("Nouvel onglet", "New tab"));
  auto* close_button = new QToolButton(tab_bar_);
  close_button->setObjectName("tabCloseButton");
  close_button->setIcon(chrome_icon(ChromeIcon::close));
  close_button->setIconSize(QSize(12, 12));
  close_button->setToolTip(text("Fermer l’onglet", "Close tab"));
  connect(close_button, &QToolButton::clicked, this, [this, tab] {
    close_tab(tab_stack_->indexOf(tab));
  });
  tab_bar_->setTabButton(index, QTabBar::RightSide, close_button);
  tab->set_state_handler([this, tab] {
    const int tab_index = tab_stack_->indexOf(tab);
    if (tab_index >= 0) update_tab_presentation(tab);
    if (tab == current_tab()) {
      record_history(tab);
      update_chrome();
    }
  });
  tab->set_new_tab_handler([this](const QString& target) {
    add_tab(target, true);
  });
  if (activate) tab_bar_->setCurrentIndex(index);
  tab->navigate(url);
  return tab;
}

void BrowserWindow::close_tab(int index) {
  if (index < 0 || index >= tab_bar_->count()) return;
  auto* page = static_cast<BrowserTab*>(tab_stack_->widget(index));
  if (!page->url().isEmpty()) {
    closed_tabs_.prepend(page->url());
    while (closed_tabs_.size() > 10) closed_tabs_.removeLast();
  }
  pinned_tabs_.remove(page);
  tab_groups_.remove(page);
  tab_stack_->removeWidget(page);
  tab_bar_->removeTab(index);
  page->deleteLater();
  if (tab_bar_->count() == 0) add_tab();
}

void BrowserWindow::update_tab_presentation(BrowserTab* tab) {
  const int index = tab_stack_->indexOf(tab);
  if (index < 0) return;
  QString label = tab->title().simplified();
  if (label.isEmpty()) label = text("Nouvel onglet", "New tab");
  const QString group = tab_groups_.value(tab);
  if (!group.isEmpty()) label = group + " · " + label;
  tab_bar_->setTabText(index, label.left(pinned_tabs_.contains(tab) ? 22 : 40));
  tab_bar_->setTabToolTip(index, group.isEmpty() ? tab->title()
                                                 : group + " — " + tab->title());
  QWidget* close = tab_bar_->tabButton(index, QTabBar::RightSide);
  if (close) close->setVisible(!pinned_tabs_.contains(tab));
  if (group.isEmpty()) {
    tab_bar_->setTabTextColor(index, QColor("#afb4c0"));
  } else {
    const uint hue = qHash(group) % 360U;
    tab_bar_->setTabTextColor(index, QColor::fromHsv(static_cast<int>(hue), 105, 245));
  }
}

void BrowserWindow::set_tab_pinned(BrowserTab* tab, bool pinned) {
  const int index = tab_stack_->indexOf(tab);
  if (index < 0) return;
  if (pinned) {
    pinned_tabs_.insert(tab);
    tab_bar_->moveTab(index, qMax(0, pinned_tabs_.size() - 1));
  } else {
    pinned_tabs_.remove(tab);
    tab_bar_->moveTab(index, pinned_tabs_.size());
  }
  update_tab_presentation(tab);
}

void BrowserWindow::update_chrome() {
  BrowserTab* tab = current_tab();
  if (!tab) return;
  address_->setText(tab->url());
  back_->setEnabled(tab->can_go_back());
  forward_->setEnabled(tab->can_go_forward());
  reload_->setIcon(chrome_icon(tab->is_loading()
      ? ChromeIcon::stop : ChromeIcon::reload));
  reload_->setToolTip(tab->is_loading()
      ? text("Arrêter le chargement", "Stop loading")
      : text("Actualiser", "Reload"));
  statusBar()->showMessage(tab->is_loading()
      ? text("Chargement…", "Loading…")
      : security_->available()
          ? (tab->uses_modern_engine()
              ? text("Moteur Chromium avec Kitten Security Core",
                     "Chromium engine with Kitten Security Core")
              : text("Kitten Safe View actif", "Kitten Safe View active"))
          : text("Kitten Security Core indisponible", "Kitten Security Core unavailable"));
  const QUrl current(tab->url());
  const QString scheme = current.scheme().toLower();
  const QString security_state = scheme == "https" ? "secure"
      : scheme == "http" ? "insecure" : "local";
  security_badge_->setProperty("state", security_state);
  security_badge_->setProperty("protection", tab->protection_enabled());
  const int blocked = tab->blocked_resource_count();
  const QString connection = scheme == "https"
      ? text("Connexion chiffrée", "Encrypted connection")
      : scheme == "http" ? text("Connexion non chiffrée", "Unencrypted connection")
                           : text("Page interne", "Internal page");
  security_badge_->setToolTip(tab->protection_enabled()
      ? connection + text(" — Protection active — Éléments bloqués : ",
                          " — Protection active — Blocked items: ") + QString::number(blocked)
      : connection + text(" — Protection désactivée pour cet onglet",
                          " — Protection disabled for this tab"));
  security_badge_->style()->unpolish(security_badge_);
  security_badge_->style()->polish(security_badge_);
  const QStringList bookmarks = settings_.value("bookmarks/urls").toStringList();
  const bool internal_page = tab->url().startsWith("kitten://");
  const bool bookmarked = bookmarks.contains(tab->url());
  security_badge_->setEnabled(!internal_page);
  bookmark_->setEnabled(!internal_page);
  bookmark_->setIcon(chrome_icon(bookmarked
      ? ChromeIcon::star_filled : ChromeIcon::star));
  bookmark_->setToolTip(bookmarked
      ? text("Retirer des favoris", "Remove bookmark")
      : text("Ajouter aux favoris", "Add bookmark"));
  reopen_closed_->setEnabled(!closed_tabs_.isEmpty());
  const QString profile_suffix = profile_name_ == "Default"
      ? QString{} : " [" + profile_name_ + "]";
  setWindowTitle(tab->title() + profile_suffix + (private_mode_
      ? text(" — Navigation privée — Open Kitten",
             " — Private browsing — Open Kitten")
      : " — Open Kitten"));
}

void BrowserWindow::reopen_closed_tab() {
  if (closed_tabs_.isEmpty()) return;
  add_tab(closed_tabs_.takeFirst());
  update_chrome();
}

void BrowserWindow::change_zoom(int step) {
  BrowserTab* tab = current_tab();
  if (!tab) return;
  if (step > 0) tab->zoom_in();
  else if (step < 0) tab->zoom_out();
  else tab->reset_zoom();
  statusBar()->showMessage(text("Zoom : ", "Zoom: ") +
                           QString::number(tab->zoom_percent()) + "%", 2000);
}

void BrowserWindow::add_bookmark() {
  BrowserTab* tab = current_tab();
  if (!tab || tab->url().startsWith("kitten://")) return;
  QStringList bookmarks = settings_.value("bookmarks/urls").toStringList();
  if (bookmarks.contains(tab->url())) bookmarks.removeAll(tab->url());
  else bookmarks.append(tab->url());
  settings_.setValue("bookmarks/urls", bookmarks);
  rebuild_bookmarks_menu();
  rebuild_bookmarks_bar();
  update_chrome();
}

void BrowserWindow::remove_bookmark(const QString& url) {
  QStringList bookmarks = settings_.value("bookmarks/urls").toStringList();
  bookmarks.removeAll(url);
  settings_.setValue("bookmarks/urls", bookmarks);
  rebuild_bookmarks_menu();
}

void BrowserWindow::rebuild_bookmarks_menu() {
  bookmarks_menu_->clear();
  bookmarks_menu_->addAction(import_bookmarks_);
  bookmarks_menu_->addSeparator();
  QAction* add = bookmarks_menu_->addAction(
      text("Ajouter ou retirer la page actuelle", "Add or remove current page"));
  connect(add, &QAction::triggered, this, [this] { add_bookmark(); });
  const QStringList bookmarks = settings_.value("bookmarks/urls").toStringList();
  if (!bookmarks.isEmpty()) bookmarks_menu_->addSeparator();
  for (const QString& url : bookmarks) {
    QMenu* item = bookmarks_menu_->addMenu(url.left(80));
    QAction* open = item->addAction(text("Ouvrir", "Open"));
    QAction* remove = item->addAction(text("Supprimer", "Remove"));
    connect(open, &QAction::triggered, this, [this, url] {
      if (current_tab()) current_tab()->navigate(url);
    });
    connect(remove, &QAction::triggered, this, [this, url] {
      remove_bookmark(url);
    });
  }
}

void BrowserWindow::rebuild_bookmarks_bar() {
  bookmarks_bar_->clear();
  const QStringList bookmarks = settings_.value("bookmarks/urls").toStringList();
  for (const QString& url : bookmarks.mid(0, 10)) {
    const QUrl parsed(url);
    QString label = parsed.host().isEmpty() ? url : parsed.host();
    QAction* action = bookmarks_bar_->addAction(label.left(24));
    action->setToolTip(url);
    connect(action, &QAction::triggered, this, [this, url] {
      if (current_tab()) current_tab()->navigate(url);
    });
  }
  bookmarks_bar_->setVisible(!bookmarks.isEmpty() &&
      settings_.value("bookmarks/showBar", true).toBool());
}

QString BrowserWindow::text(const char* french, const char* english) const {
  return ui_text(language_, french, english);
}

void BrowserWindow::set_language(UiLanguage language) {
  if (language_ == language) return;
  language_ = language;
  settings_.setValue("ui/language", language_code(language_));
  for (int index = 0; index < tab_stack_->count(); ++index) {
    static_cast<BrowserTab*>(tab_stack_->widget(index))->set_language(language_);
  }
  apply_language();
}

void BrowserWindow::set_search_engine(const QString& search_engine) {
  if (search_engine != "duckduckgo" && search_engine != "brave" &&
      search_engine != "qwant") return;
  search_engine_ = search_engine;
  settings_.setValue("search/engine", search_engine_);
  for (int index = 0; index < tab_stack_->count(); ++index) {
    static_cast<BrowserTab*>(tab_stack_->widget(index))
        ->set_search_engine(search_engine_);
  }
  apply_language();
}

void BrowserWindow::show_protection_panel() {
  BrowserTab* tab = current_tab();
  if (!tab || tab->url().startsWith("kitten://")) return;
  const QUrl current(tab->url());
  const QString host = current.host().isEmpty() ? tab->url() : current.host();
  const bool secure = current.scheme().compare("https", Qt::CaseInsensitive) == 0;

  QMenu panel(this);
  panel.setObjectName("protectionPanel");
  QAction* heading = panel.addAction("Kitten Shields — " + host);
  heading->setEnabled(false);
  QAction* connection = panel.addAction(secure
      ? text("Connexion sécurisée (HTTPS)", "Secure connection (HTTPS)")
      : text("Connexion non sécurisée (HTTP)", "Unsecured connection (HTTP)"));
  connection->setEnabled(false);
  QAction* blocked = panel.addAction(
      text("Éléments bloqués sur cette page : ", "Items blocked on this page: ") +
      QString::number(tab->blocked_resource_count()));
  blocked->setEnabled(false);
  panel.addSeparator();
  QAction* toggle = panel.addAction(
      text("Protection activée pour cet onglet", "Protection enabled for this tab"));
  toggle->setCheckable(true);
  toggle->setChecked(tab->protection_enabled());
  QAction* safe = panel.addAction(
      text("Ouvrir dans Kitten Safe View", "Open in Kitten Safe View"));
  panel.addSeparator();
  QAction* details = panel.addAction(
      text("Afficher les détails de protection", "View protection details"));

  QAction* selected = panel.exec(
      security_badge_->mapToGlobal(QPoint(0, security_badge_->height() + 4)));
  if (selected == toggle) {
    tab->set_protection_enabled(toggle->isChecked());
  } else if (selected == safe) {
    tab->use_safe_view();
  } else if (selected == details) {
    tab->navigate("kitten://privacy");
  }
  update_chrome();
}

void BrowserWindow::record_history(BrowserTab* tab) {
  if (private_mode_ || !tab || tab->is_loading()) return;
  const QString url = tab->url();
  if (!url.startsWith("http://") && !url.startsWith("https://")) return;
  QJsonArray entries = read_history(settings_);
  if (!entries.isEmpty() && entries.first().toObject().value("url").toString() == url) {
    return;
  }
  QJsonObject entry;
  entry.insert("url", url);
  entry.insert("title", tab->title());
  entry.insert("visited", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
  entries.prepend(entry);
  while (entries.size() > 1000) entries.removeLast();
  write_history(settings_, entries);
}

void BrowserWindow::show_history() {
  QDialog dialog(this);
  dialog.setWindowTitle(text("Historique", "History"));
  dialog.resize(760, 540);
  auto* layout = new QVBoxLayout(&dialog);
  auto* search = new QLineEdit(&dialog);
  search->setPlaceholderText(text("Rechercher dans l’historique", "Search history"));
  auto* list = new QListWidget(&dialog);
  const QJsonArray entries = read_history(settings_);
  for (const QJsonValue& value : entries) {
    const QJsonObject entry = value.toObject();
    const QString url = entry.value("url").toString();
    const QString title = entry.value("title").toString();
    const QDateTime visited = QDateTime::fromString(
        entry.value("visited").toString(), Qt::ISODate).toLocalTime();
    auto* item = new QListWidgetItem(
        visited.toString("dd/MM/yyyy HH:mm") + "   " + title + "\n" + url, list);
    item->setData(Qt::UserRole, url);
  }
  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
  auto* open = buttons->addButton(text("Ouvrir", "Open"),
                                  QDialogButtonBox::ActionRole);
  auto* clear = buttons->addButton(text("Effacer l’historique", "Clear history"),
                                   QDialogButtonBox::DestructiveRole);
  layout->addWidget(search);
  layout->addWidget(list, 1);
  layout->addWidget(buttons);
  connect(search, &QLineEdit::textChanged, list, [list](const QString& query) {
    for (int index = 0; index < list->count(); ++index) {
      list->item(index)->setHidden(
          !list->item(index)->text().contains(query, Qt::CaseInsensitive));
    }
  });
  const auto open_selected = [this, list, &dialog] {
    if (!list->currentItem()) return;
    add_tab(list->currentItem()->data(Qt::UserRole).toString(), true);
    dialog.accept();
  };
  connect(open, &QPushButton::clicked, &dialog, open_selected);
  connect(list, &QListWidget::itemDoubleClicked, &dialog,
          [open_selected](QListWidgetItem*) { open_selected(); });
  connect(clear, &QPushButton::clicked, &dialog, [this, list] {
    if (QMessageBox::question(
            this, text("Effacer l’historique", "Clear history"),
            text("Supprimer tout l’historique de ce profil ?",
                 "Delete all history for this profile?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) !=
        QMessageBox::Yes) return;
    write_history(settings_, QJsonArray{});
    list->clear();
  });
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  dialog.exec();
}

void BrowserWindow::show_settings_dialog() {
  QDialog dialog(this);
  dialog.setWindowTitle(text("Paramètres Open Kitten", "Open Kitten settings"));
  dialog.setMinimumWidth(480);
  auto* layout = new QVBoxLayout(&dialog);
  auto* form = new QFormLayout();
  auto* profile = new QLabel(profile_name_, &dialog);
  auto* language = new QComboBox(&dialog);
  for (UiLanguage item : supported_languages()) language->addItem(language_name(item));
  language->setCurrentIndex(supported_languages().indexOf(language_));
  auto* search = new QComboBox(&dialog);
  search->addItems({"DuckDuckGo", "Brave Search", "Qwant"});
  search->setCurrentIndex(search_engine_ == "brave" ? 1 : search_engine_ == "qwant" ? 2 : 0);
  auto* renderer = new QComboBox(&dialog);
  renderer->addItem("Kitten Engine (Chromium)");
  renderer->addItem("Kitten Safe View");
  renderer->setCurrentIndex(prefer_safe_view_ ? 1 : 0);
  auto* restore = new QCheckBox(text("Restaurer les onglets au démarrage",
                                     "Restore tabs at startup"), &dialog);
  restore->setChecked(settings_.value("session/restore", true).toBool());
  auto* bookmarks = new QCheckBox(text("Afficher la barre de favoris",
                                       "Show bookmarks bar"), &dialog);
  bookmarks->setChecked(settings_.value("bookmarks/showBar", true).toBool());
  auto* passwords = new QCheckBox(text("Proposer l’enregistrement des mots de passe",
                                       "Offer to save passwords"), &dialog);
  passwords->setChecked(settings_.value("privacy/passwordAutosave", true).toBool());
  passwords->setEnabled(!private_mode_);
  auto* strict = new QCheckBox(text("Protection anti-pistage stricte",
                                    "Strict tracking prevention"), &dialog);
  strict->setChecked(settings_.value("privacy/strictTracking", false).toBool());
  auto* clear_data = new QPushButton(text("Effacer cookies, cache et stockage…",
                                          "Clear cookies, cache and storage…"), &dialog);
  form->addRow(text("Profil", "Profile"), profile);
  form->addRow(text("Langue", "Language"), language);
  form->addRow(text("Moteur de recherche", "Search engine"), search);
  form->addRow(text("Moteur d’affichage", "Rendering engine"), renderer);
  form->addRow(QString{}, restore);
  form->addRow(QString{}, bookmarks);
  form->addRow(QString{}, passwords);
  form->addRow(QString{}, strict);
  form->addRow(text("Données de navigation", "Browsing data"), clear_data);
  auto* buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  layout->addLayout(form);
  layout->addWidget(buttons);
  connect(clear_data, &QPushButton::clicked, this, [this] {
    if (!current_tab()) return;
    if (QMessageBox::question(
            this, text("Effacer les données", "Clear data"),
            text("Effacer les cookies, le cache, les autorisations et le stockage de ce profil ?",
                 "Clear cookies, cache, permissions and storage for this profile?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) !=
        QMessageBox::Yes) return;
    current_tab()->clear_browsing_data([this](bool success) {
      QMessageBox::information(
          this, text("Données de navigation", "Browsing data"),
          success ? text("Les données ont été effacées.", "Browsing data was cleared.")
                  : text("Impossible d’effacer les données.", "Browsing data could not be cleared."));
    });
  });
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  if (dialog.exec() != QDialog::Accepted) return;

  const QVector<UiLanguage> languages = supported_languages();
  if (language->currentIndex() >= 0 && language->currentIndex() < languages.size()) {
    set_language(languages.at(language->currentIndex()));
  }
  set_search_engine(search->currentIndex() == 1 ? "brave"
                    : search->currentIndex() == 2 ? "qwant" : "duckduckgo");
  prefer_safe_view_ = renderer->currentIndex() == 1;
  settings_.setValue("engine/preferSafeView", prefer_safe_view_);
  settings_.setValue("session/restore", restore->isChecked());
  settings_.setValue("bookmarks/showBar", bookmarks->isChecked());
  settings_.setValue("privacy/passwordAutosave", passwords->isChecked());
  settings_.setValue("privacy/strictTracking", strict->isChecked());
  for (int index = 0; index < tab_stack_->count(); ++index) {
    auto* tab = static_cast<BrowserTab*>(tab_stack_->widget(index));
    tab->set_password_autosave(passwords->isChecked());
    tab->set_tracking_prevention(strict->isChecked());
    tab->set_prefer_safe_view(prefer_safe_view_);
  }
  rebuild_bookmarks_bar();
}

void BrowserWindow::import_bookmarks() {
  const QString file_name = QFileDialog::getOpenFileName(
      this, text("Importer des favoris", "Import bookmarks"), QString{},
      text("Favoris de navigateur (*.html *.htm *.json);;Tous les fichiers (*)",
           "Browser bookmarks (*.html *.htm *.json);;All files (*)"));
  if (file_name.isEmpty()) return;
  QFile file(file_name);
  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, text("Importation", "Import"), file.errorString());
    return;
  }
  const QByteArray file_data = file.readAll();
  QStringList imported;
  if (file_name.endsWith(".json", Qt::CaseInsensitive)) {
    collect_bookmark_urls(QJsonDocument::fromJson(file_data).object(), imported);
  } else {
    const QString html = QString::fromUtf8(file_data);
    const QRegularExpression links(
        R"REGEX(href\s*=\s*["']([^"']+)["'])REGEX",
        QRegularExpression::CaseInsensitiveOption);
    auto matches = links.globalMatch(html);
    while (matches.hasNext()) {
      const QString url = matches.next().captured(1);
      if (url.startsWith("http://") || url.startsWith("https://")) imported.append(url);
    }
  }
  QStringList bookmarks = settings_.value("bookmarks/urls").toStringList();
  const qsizetype previous_count = bookmarks.size();
  for (const QString& url : imported) {
    if (!bookmarks.contains(url)) bookmarks.append(url);
  }
  settings_.setValue("bookmarks/urls", bookmarks);
  rebuild_bookmarks_menu();
  rebuild_bookmarks_bar();
  QMessageBox::information(
      this, text("Importation terminée", "Import complete"),
      text("Favoris ajoutés : ", "Bookmarks added: ") +
      QString::number(bookmarks.size() - previous_count));
}

void BrowserWindow::export_profile() {
  const QString file_name = QFileDialog::getSaveFileName(
      this, text("Exporter le profil", "Export profile"),
      "open-kitten-" + profile_name_.toLower() + ".json",
      text("Profil Open Kitten (*.json)", "Open Kitten profile (*.json)"));
  if (file_name.isEmpty()) return;

  QJsonObject preferences;
  preferences.insert("language", settings_.value("ui/language", "fr").toString());
  preferences.insert("searchEngine",
                     settings_.value("search/engine", "duckduckgo").toString());
  preferences.insert("restoreSession", settings_.value("session/restore", true).toBool());
  preferences.insert("showBookmarksBar",
                     settings_.value("bookmarks/showBar", true).toBool());
  preferences.insert("strictTracking",
                     settings_.value("privacy/strictTracking", false).toBool());
  preferences.insert("preferSafeView", prefer_safe_view_);

  QJsonArray bookmarks;
  for (const QString& url : settings_.value("bookmarks/urls").toStringList()) {
    bookmarks.append(url);
  }
  QJsonObject root;
  root.insert("format", "open-kitten-profile");
  root.insert("version", 1);
  root.insert("profile", profile_name_);
  root.insert("preferences", preferences);
  root.insert("bookmarks", bookmarks);
  root.insert("history", read_history(settings_));

  QSaveFile file(file_name);
  if (!file.open(QIODevice::WriteOnly) ||
      file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0 ||
      !file.commit()) {
    QMessageBox::warning(this, text("Export impossible", "Export failed"),
                         file.errorString());
    return;
  }
  QMessageBox::information(
      this, text("Profil exporté", "Profile exported"),
      text("Les favoris, l’historique et les préférences ont été exportés. Les mots de passe et les cookies restent protégés sur cet appareil.",
           "Bookmarks, history and preferences were exported. Passwords and cookies remain protected on this device."));
}

void BrowserWindow::import_profile() {
  const QString file_name = QFileDialog::getOpenFileName(
      this, text("Importer un profil", "Import profile"), QString{},
      text("Profil Open Kitten (*.json)", "Open Kitten profile (*.json)"));
  if (file_name.isEmpty()) return;
  QFile file(file_name);
  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, text("Import impossible", "Import failed"),
                         file.errorString());
    return;
  }
  QJsonParseError parse_error;
  const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse_error);
  const QJsonObject root = document.object();
  if (parse_error.error != QJsonParseError::NoError ||
      root.value("format").toString() != "open-kitten-profile" ||
      root.value("version").toInt() != 1) {
    QMessageBox::warning(
        this, text("Profil non valide", "Invalid profile"),
        text("Ce fichier n’est pas un profil Open Kitten compatible.",
             "This file is not a compatible Open Kitten profile."));
    return;
  }

  QStringList bookmarks;
  for (const QJsonValue& value : root.value("bookmarks").toArray()) {
    const QString url = value.toString();
    if ((url.startsWith("http://") || url.startsWith("https://")) &&
        !bookmarks.contains(url)) bookmarks.append(url);
  }
  const QJsonObject preferences = root.value("preferences").toObject();
  settings_.setValue("bookmarks/urls", bookmarks);
  settings_.setValue("history/items", QJsonDocument(root.value("history").toArray())
                                          .toJson(QJsonDocument::Compact));
  settings_.setValue("session/restore",
                     preferences.value("restoreSession").toBool(true));
  settings_.setValue("bookmarks/showBar",
                     preferences.value("showBookmarksBar").toBool(true));
  settings_.setValue("privacy/strictTracking",
                     preferences.value("strictTracking").toBool(false));
  prefer_safe_view_ = preferences.value("preferSafeView").toBool(false);
  settings_.setValue("engine/preferSafeView", prefer_safe_view_);
  set_language(language_from_code(preferences.value("language").toString("fr")));
  set_search_engine(preferences.value("searchEngine").toString("duckduckgo"));
  const bool strict = settings_.value("privacy/strictTracking", false).toBool();
  for (int index = 0; index < tab_stack_->count(); ++index) {
    static_cast<BrowserTab*>(tab_stack_->widget(index))
        ->set_tracking_prevention(strict);
    static_cast<BrowserTab*>(tab_stack_->widget(index))
        ->set_prefer_safe_view(prefer_safe_view_);
  }
  rebuild_bookmarks_menu();
  rebuild_bookmarks_bar();
  QMessageBox::information(
      this, text("Profil importé", "Profile imported"),
      text("Les favoris, l’historique et les préférences ont été restaurés.",
           "Bookmarks, history and preferences were restored."));
}

void BrowserWindow::install_extension() {
  const QString folder = QFileDialog::getExistingDirectory(
      this, text("Choisir une extension Chromium décompressée",
                 "Choose an unpacked Chromium extension"));
  if (folder.isEmpty()) return;
  if (!QFile::exists(folder + "/manifest.json")) {
    QMessageBox::warning(
        this, text("Extension non valide", "Invalid extension"),
        text("Le dossier doit contenir un fichier manifest.json.",
             "The folder must contain a manifest.json file."));
    return;
  }
  if (!current_tab()) return;
  current_tab()->install_extension(
      folder, [this](bool success, const QString& error) {
        if (success) {
          QMessageBox::information(
              this, text("Extension installée", "Extension installed"),
              text("L’extension est active dans ce profil. Les extensions qui exigent une icône de barre peuvent avoir une interface limitée.",
                   "The extension is active in this profile. Extensions that require a toolbar icon can have limited UI."));
        } else {
          QMessageBox::warning(this, text("Échec de l’extension", "Extension failed"),
                               error);
        }
      });
}

void BrowserWindow::apply_language() {
  file_menu_->setTitle(text("Fichier", "File"));
  bookmarks_menu_->setTitle(text("Favoris", "Bookmarks"));
  view_menu_->setTitle(text("Affichage", "View"));
  settings_menu_->setTitle(text("Paramètres", "Settings"));
  language_menu_->setTitle(text("Langue", "Language"));
  search_engine_menu_->setTitle(text("Moteur de recherche", "Search engine"));
  help_menu_->setTitle(text("Aide", "Help"));
  new_tab_->setText(text("Nouvel onglet", "New tab"));
  new_private_window_->setText(text("Nouvelle fenêtre privée", "New private window"));
  new_profile_window_->setText(text("Nouvelle fenêtre avec un autre profil…",
                                     "New window with another profile…"));
  close_current_->setText(text("Fermer l’onglet", "Close tab"));
  reopen_closed_->setText(text("Rouvrir l’onglet fermé", "Reopen closed tab"));
  history_->setText(text("Historique", "History"));
  quit_->setText(text("Quitter", "Quit"));
  about_->setText(text("À propos", "About"));
  zoom_in_->setText(text("Agrandir", "Zoom in"));
  zoom_out_->setText(text("Réduire", "Zoom out"));
  zoom_reset_->setText(text("Taille réelle", "Actual size"));
  find_->setText(text("Rechercher dans la page", "Find in page"));
  print_->setText(text("Imprimer", "Print"));
  downloads_->setText(text("Téléchargements", "Downloads"));
  developer_tools_->setText(text("Outils de développement", "Developer tools"));
  fullscreen_->setText(text("Plein écran", "Full screen"));
  kitten_engine_->setText("Kitten Engine");
  safe_view_->setText(text("Ouvrir dans Kitten Safe View",
                           "Open in Kitten Safe View"));
  open_settings_->setText(text("Paramètres complets…", "Full settings…"));
  import_bookmarks_->setText(text("Importer des favoris…", "Import bookmarks…"));
  install_extension_->setText(text("Installer une extension locale…",
                                    "Install local extension…"));
  export_profile_->setText(text("Exporter le profil…", "Export profile…"));
  import_profile_->setText(text("Importer un profil…", "Import profile…"));
  address_->setPlaceholderText(text("Adresse ou recherche", "Address or search"));
  back_->setToolTip(text("Page précédente", "Back"));
  forward_->setToolTip(text("Page suivante", "Forward"));
  home_->setToolTip(text("Accueil", "Home"));
  security_badge_->setAccessibleName(text("Protection de la page", "Page protection"));
  downloads_->setToolTip(text("Téléchargements", "Downloads"));
  menu_button_->setToolTip(text("Menu Open Kitten", "Open Kitten menu"));
  for (QAction* action : language_actions_) {
    action->setChecked(action->data().toString() == language_code(language_));
  }
  duckduckgo_->setChecked(search_engine_ == "duckduckgo");
  brave_search_->setChecked(search_engine_ == "brave");
  qwant_->setChecked(search_engine_ == "qwant");
  rebuild_bookmarks_menu();
  rebuild_bookmarks_bar();
  update_chrome();
}

void BrowserWindow::apply_theme() {
  setStyleSheet(R"QSS(
    QMainWindow, QWidget#browserShell { background: #17191f; color: #f4f5f8; }
    QWidget#tabStrip { background: #0f1116; border-bottom: 1px solid #252936; }
    QMenu { background: #242731; color: #f4f5f8; border: 1px solid #3a3f4e; padding: 5px; }
    QMenu::item { padding: 8px 30px 8px 12px; border-radius: 5px; }
    QMenu::item:selected { background: #353a49; color: #ffffff; }
    QMenu#protectionPanel { min-width: 320px; padding: 8px; background: #1d2028; border: 1px solid #454b5d; }
    QMenu#protectionPanel::item { padding: 10px 14px; }
    QMenu#protectionPanel::item:disabled { color: #aeb4c2; }
    QWidget#navigationToolbar { background: #15171d; border: 0; border-bottom: 1px solid #292d38; }
    QToolBar#bookmarksToolbar { background: #111318; border: 0; border-bottom: 1px solid #292d38; padding: 2px 10px; spacing: 2px; }
    QToolBar#bookmarksToolbar QToolButton { min-height: 25px; padding: 0 7px; color: #d8dbe4; }
    QToolButton { color: #e8eaf0; background: transparent; border: 1px solid transparent; border-radius: 7px; min-width: 32px; min-height: 32px; }
    QToolButton#mainMenuButton::menu-indicator { image: none; width: 0px; height: 0px; }
    QToolButton:hover { background: #2d313d; border-color: #3a3f4d; }
    QToolButton:pressed { background: #383d4b; }
    QToolButton:disabled { background: transparent; border-color: transparent; }
    QLineEdit#addressBar { background: #252832; color: #f7f8fb; border: 1px solid #3b404e; border-radius: 10px; min-height: 34px; max-height: 34px; padding: 0 12px; selection-background-color: #5865f2; }
    QLineEdit#addressBar:focus { border: 1px solid #747cff; background: #2e323d; }
    QToolButton#securityBadge { background: #252936; border: 1px solid #3c4252; border-radius: 9px; min-width: 34px; }
    QToolButton#securityBadge:hover { background: #303544; border-color: #596273; }
    QToolButton#securityBadge[protection="false"] { background: #34282a; border-color: #70474b; }
    QToolButton#tabCloseButton { min-width: 18px; max-width: 18px; min-height: 18px; max-height: 18px; border: 0; border-radius: 4px; }
    QToolButton#tabCloseButton:hover { background: #4a4f5d; }
    QTabBar { background: transparent; border: 0; }
    QTabBar::tab { background: #20232b; color: #afb4c0; border: 0; border-radius: 8px; padding: 8px 14px; margin: 1px 4px 1px 0; min-width: 135px; max-width: 230px; min-height: 24px; }
    QTabBar::tab:selected { background: #353945; color: #ffffff; }
    QTabBar::tab:hover:!selected { background: #292d37; color: #e9ebf1; }
    QStackedWidget#pageStack { background: #101116; border: 0; }
  )QSS");
}

void BrowserWindow::restore_session() {
  if (!settings_.value("session/restore", true).toBool()) {
    add_tab();
    return;
  }
  const QStringList urls = settings_.value("session/urls").toStringList();
  const QVariantList pinned = settings_.value("session/pinned").toList();
  const QStringList groups = settings_.value("session/groups").toStringList();
  const QStringList limited_urls = urls.mid(0, 20);
  for (int index = 0; index < limited_urls.size(); ++index) {
    BrowserTab* tab = add_tab(limited_urls.at(index), false);
    if (index < pinned.size() && pinned.at(index).toBool()) pinned_tabs_.insert(tab);
    if (index < groups.size() && !groups.at(index).isEmpty()) {
      tab_groups_.insert(tab, groups.at(index));
    }
    update_tab_presentation(tab);
  }
  if (tab_bar_->count() == 0) add_tab();
  const int active = settings_.value("session/active", 0).toInt();
  tab_bar_->setCurrentIndex(qBound(0, active, tab_bar_->count() - 1));
}

void BrowserWindow::save_session() {
  QStringList urls;
  QVariantList pinned;
  QStringList groups;
  for (int index = 0; index < tab_stack_->count(); ++index) {
    auto* tab = static_cast<BrowserTab*>(tab_stack_->widget(index));
    urls.append(tab->url());
    pinned.append(pinned_tabs_.contains(tab));
    groups.append(tab_groups_.value(tab));
  }
  settings_.setValue("session/urls", urls);
  settings_.setValue("session/pinned", pinned);
  settings_.setValue("session/groups", groups);
  settings_.setValue("session/active", tab_bar_->currentIndex());
  settings_.setValue("window/geometry", saveGeometry());
  settings_.sync();
}

void BrowserWindow::closeEvent(QCloseEvent* event) {
  if (!private_mode_) save_session();
  QMainWindow::closeEvent(event);
}
