#pragma once

#include <QMainWindow>
#include <QHash>
#include <QSettings>
#include <QSet>
#include <QVector>

#include "browser/localization.h"

class BrowserTab;
class QAction;
class QCloseEvent;
class QLineEdit;
class QMenu;
class QStackedWidget;
class QTabBar;
class QToolBar;
class QToolButton;
class SecurityBridge;

class BrowserWindow final : public QMainWindow {
 public:
  explicit BrowserWindow(bool private_mode = false,
                         const QString& profile_name = "Default");
  ~BrowserWindow() override;
  [[nodiscard]] bool modern_engine_ready() const;

 protected:
  void closeEvent(QCloseEvent* event) override;

 private:
  BrowserTab* current_tab() const;
  BrowserTab* add_tab(const QString& url = "kitten://home", bool activate = true);
  void close_tab(int index);
  void update_chrome();
  void add_bookmark();
  void remove_bookmark(const QString& url);
  void reopen_closed_tab();
  void change_zoom(int step);
  void rebuild_bookmarks_menu();
  void rebuild_bookmarks_bar();
  void apply_language();
  void set_language(UiLanguage language);
  void set_search_engine(const QString& search_engine);
  void show_protection_panel();
  void show_history();
  void show_settings_dialog();
  void import_bookmarks();
  void export_profile();
  void import_profile();
  void install_extension();
  void record_history(BrowserTab* tab);
  void update_tab_presentation(BrowserTab* tab);
  void set_tab_pinned(BrowserTab* tab, bool pinned);
  void apply_theme();
  void restore_session();
  void save_session();
  [[nodiscard]] QString text(const char* french, const char* english) const;

  QLineEdit* address_{nullptr};
  QToolButton* security_badge_{nullptr};
  QTabBar* tab_bar_{nullptr};
  QStackedWidget* tab_stack_{nullptr};
  QWidget* toolbar_{nullptr};
  QToolBar* bookmarks_bar_{nullptr};
  QToolButton* menu_button_{nullptr};
  QAction* back_{nullptr};
  QAction* forward_{nullptr};
  QAction* reload_{nullptr};
  QAction* home_{nullptr};
  QAction* bookmark_{nullptr};
  QAction* new_tab_{nullptr};
  QAction* close_current_{nullptr};
  QAction* new_private_window_{nullptr};
  QAction* new_profile_window_{nullptr};
  QAction* reopen_closed_{nullptr};
  QAction* history_{nullptr};
  QAction* zoom_in_{nullptr};
  QAction* zoom_out_{nullptr};
  QAction* zoom_reset_{nullptr};
  QAction* fullscreen_{nullptr};
  QAction* safe_view_{nullptr};
  QAction* kitten_engine_{nullptr};
  QAction* find_{nullptr};
  QAction* print_{nullptr};
  QAction* downloads_{nullptr};
  QAction* developer_tools_{nullptr};
  QAction* quit_{nullptr};
  QAction* about_{nullptr};
  QVector<QAction*> language_actions_;
  QAction* duckduckgo_{nullptr};
  QAction* brave_search_{nullptr};
  QAction* qwant_{nullptr};
  QAction* open_settings_{nullptr};
  QAction* import_bookmarks_{nullptr};
  QAction* export_profile_{nullptr};
  QAction* import_profile_{nullptr};
  QAction* install_extension_{nullptr};
  QMenu* file_menu_{nullptr};
  QMenu* bookmarks_menu_{nullptr};
  QMenu* view_menu_{nullptr};
  QMenu* settings_menu_{nullptr};
  QMenu* language_menu_{nullptr};
  QMenu* search_engine_menu_{nullptr};
  QMenu* help_menu_{nullptr};
  QMenu* main_menu_{nullptr};
  SecurityBridge* security_{nullptr};
  QSettings settings_;
  QStringList closed_tabs_;
  QSet<BrowserTab*> pinned_tabs_;
  QHash<BrowserTab*, QString> tab_groups_;
  UiLanguage language_{UiLanguage::french};
  QString search_engine_{"duckduckgo"};
  QString profile_name_{"Default"};
  bool prefer_safe_view_{false};
  bool private_mode_{false};
};
