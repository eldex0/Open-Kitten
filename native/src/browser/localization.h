#pragma once

#include <QHash>
#include <QLocale>
#include <QString>
#include <QVector>

enum class UiLanguage {
  french, english, spanish, german, italian, portuguese, dutch, polish,
  japanese, chinese_simplified
};

inline QVector<UiLanguage> supported_languages() {
  return {UiLanguage::french, UiLanguage::english, UiLanguage::spanish,
          UiLanguage::german, UiLanguage::italian, UiLanguage::portuguese,
          UiLanguage::dutch, UiLanguage::polish, UiLanguage::japanese,
          UiLanguage::chinese_simplified};
}

inline QString language_code(UiLanguage language) {
  switch (language) {
    case UiLanguage::french: return "fr";
    case UiLanguage::english: return "en";
    case UiLanguage::spanish: return "es";
    case UiLanguage::german: return "de";
    case UiLanguage::italian: return "it";
    case UiLanguage::portuguese: return "pt";
    case UiLanguage::dutch: return "nl";
    case UiLanguage::polish: return "pl";
    case UiLanguage::japanese: return "ja";
    case UiLanguage::chinese_simplified: return "zh-CN";
  }
  return "en";
}

inline QString language_name(UiLanguage language) {
  switch (language) {
    case UiLanguage::french: return "Français";
    case UiLanguage::english: return "English";
    case UiLanguage::spanish: return "Español";
    case UiLanguage::german: return "Deutsch";
    case UiLanguage::italian: return "Italiano";
    case UiLanguage::portuguese: return "Português";
    case UiLanguage::dutch: return "Nederlands";
    case UiLanguage::polish: return "Polski";
    case UiLanguage::japanese: return "日本語";
    case UiLanguage::chinese_simplified: return "简体中文";
  }
  return "English";
}

inline UiLanguage language_from_code(const QString& code) {
  for (UiLanguage language : supported_languages()) {
    if (language_code(language).compare(code, Qt::CaseInsensitive) == 0) return language;
  }
  return UiLanguage::english;
}

inline UiLanguage language_from_locale(QLocale::Language language) {
  switch (language) {
    case QLocale::French: return UiLanguage::french;
    case QLocale::Spanish: return UiLanguage::spanish;
    case QLocale::German: return UiLanguage::german;
    case QLocale::Italian: return UiLanguage::italian;
    case QLocale::Portuguese: return UiLanguage::portuguese;
    case QLocale::Dutch: return UiLanguage::dutch;
    case QLocale::Polish: return UiLanguage::polish;
    case QLocale::Japanese: return UiLanguage::japanese;
    case QLocale::Chinese: return UiLanguage::chinese_simplified;
    default: return UiLanguage::english;
  }
}

inline QString ui_text(UiLanguage language, const char* french,
                       const char* english) {
  if (language == UiLanguage::french) return QString::fromUtf8(french);
  const QString key = QString::fromUtf8(english);
  if (language == UiLanguage::english) return key;

  using Catalog = QHash<QString, QString>;
  static const QHash<int, Catalog> catalogs = {
    {static_cast<int>(UiLanguage::spanish), {
      {"File", "Archivo"}, {"Bookmarks", "Marcadores"}, {"View", "Ver"},
      {"Settings", "Configuración"}, {"Language", "Idioma"},
      {"Search engine", "Motor de búsqueda"}, {"Help", "Ayuda"},
      {"New tab", "Nueva pestaña"}, {"New private window", "Nueva ventana privada"},
      {"Close tab", "Cerrar pestaña"}, {"History", "Historial"}, {"Quit", "Salir"},
      {"About", "Acerca de"}, {"Zoom in", "Ampliar"}, {"Zoom out", "Reducir"},
      {"Actual size", "Tamaño real"}, {"Find in page", "Buscar en la página"},
      {"Print", "Imprimir"}, {"Downloads", "Descargas"},
      {"Developer tools", "Herramientas de desarrollo"}, {"Full screen", "Pantalla completa"},
      {"Full settings…", "Configuración completa…"}, {"Address or search", "Dirección o búsqueda"},
      {"Back", "Atrás"}, {"Home", "Inicio"}, {"Reload", "Recargar"},
      {"Loading…", "Cargando…"}, {"Open", "Abrir"}, {"Remove", "Eliminar"},
      {"Search", "Buscar"}, {"Protected browsing", "Navegación protegida"}
    }},
    {static_cast<int>(UiLanguage::german), {
      {"File", "Datei"}, {"Bookmarks", "Lesezeichen"}, {"View", "Ansicht"},
      {"Settings", "Einstellungen"}, {"Language", "Sprache"},
      {"Search engine", "Suchmaschine"}, {"Help", "Hilfe"},
      {"New tab", "Neuer Tab"}, {"New private window", "Neues privates Fenster"},
      {"Close tab", "Tab schließen"}, {"History", "Verlauf"}, {"Quit", "Beenden"},
      {"About", "Über"}, {"Zoom in", "Vergrößern"}, {"Zoom out", "Verkleinern"},
      {"Find in page", "Auf Seite suchen"}, {"Print", "Drucken"},
      {"Downloads", "Downloads"}, {"Developer tools", "Entwicklertools"},
      {"Full screen", "Vollbild"}, {"Full settings…", "Alle Einstellungen…"},
      {"Address or search", "Adresse oder Suche"}, {"Back", "Zurück"},
      {"Home", "Startseite"}, {"Reload", "Neu laden"}, {"Search", "Suchen"}
    }},
    {static_cast<int>(UiLanguage::italian), {
      {"File", "File"}, {"Bookmarks", "Preferiti"}, {"View", "Visualizza"},
      {"Settings", "Impostazioni"}, {"Language", "Lingua"},
      {"Search engine", "Motore di ricerca"}, {"Help", "Aiuto"},
      {"New tab", "Nuova scheda"}, {"New private window", "Nuova finestra privata"},
      {"Close tab", "Chiudi scheda"}, {"History", "Cronologia"}, {"Quit", "Esci"},
      {"Find in page", "Trova nella pagina"}, {"Print", "Stampa"},
      {"Downloads", "Download"}, {"Full screen", "Schermo intero"},
      {"Address or search", "Indirizzo o ricerca"}, {"Back", "Indietro"},
      {"Home", "Home"}, {"Reload", "Ricarica"}, {"Search", "Cerca"}
    }},
    {static_cast<int>(UiLanguage::portuguese), {
      {"File", "Ficheiro"}, {"Bookmarks", "Favoritos"}, {"View", "Ver"},
      {"Settings", "Definições"}, {"Language", "Idioma"},
      {"Search engine", "Motor de pesquisa"}, {"Help", "Ajuda"},
      {"New tab", "Novo separador"}, {"New private window", "Nova janela privada"},
      {"Close tab", "Fechar separador"}, {"History", "Histórico"}, {"Quit", "Sair"},
      {"Find in page", "Localizar na página"}, {"Print", "Imprimir"},
      {"Downloads", "Transferências"}, {"Full screen", "Ecrã inteiro"},
      {"Address or search", "Endereço ou pesquisa"}, {"Back", "Voltar"},
      {"Home", "Início"}, {"Reload", "Recarregar"}, {"Search", "Pesquisar"}
    }},
    {static_cast<int>(UiLanguage::dutch), {
      {"File", "Bestand"}, {"Bookmarks", "Bladwijzers"}, {"View", "Beeld"},
      {"Settings", "Instellingen"}, {"Language", "Taal"},
      {"Search engine", "Zoekmachine"}, {"Help", "Help"},
      {"New tab", "Nieuw tabblad"}, {"New private window", "Nieuw privévenster"},
      {"Close tab", "Tabblad sluiten"}, {"History", "Geschiedenis"}, {"Quit", "Afsluiten"},
      {"Find in page", "Zoeken op pagina"}, {"Print", "Afdrukken"},
      {"Downloads", "Downloads"}, {"Full screen", "Volledig scherm"},
      {"Address or search", "Adres of zoekopdracht"}, {"Back", "Terug"},
      {"Home", "Start"}, {"Reload", "Herladen"}, {"Search", "Zoeken"}
    }},
    {static_cast<int>(UiLanguage::polish), {
      {"File", "Plik"}, {"Bookmarks", "Zakładki"}, {"View", "Widok"},
      {"Settings", "Ustawienia"}, {"Language", "Język"},
      {"Search engine", "Wyszukiwarka"}, {"Help", "Pomoc"},
      {"New tab", "Nowa karta"}, {"New private window", "Nowe okno prywatne"},
      {"Close tab", "Zamknij kartę"}, {"History", "Historia"}, {"Quit", "Zakończ"},
      {"Find in page", "Znajdź na stronie"}, {"Print", "Drukuj"},
      {"Downloads", "Pobrane"}, {"Full screen", "Pełny ekran"},
      {"Address or search", "Adres lub wyszukiwanie"}, {"Back", "Wstecz"},
      {"Home", "Start"}, {"Reload", "Odśwież"}, {"Search", "Szukaj"}
    }},
    {static_cast<int>(UiLanguage::japanese), {
      {"File", "ファイル"}, {"Bookmarks", "ブックマーク"}, {"View", "表示"},
      {"Settings", "設定"}, {"Language", "言語"}, {"Search engine", "検索エンジン"},
      {"Help", "ヘルプ"}, {"New tab", "新しいタブ"},
      {"New private window", "新しいプライベートウィンドウ"}, {"Close tab", "タブを閉じる"},
      {"History", "履歴"}, {"Quit", "終了"}, {"Find in page", "ページ内検索"},
      {"Print", "印刷"}, {"Downloads", "ダウンロード"}, {"Full screen", "全画面"},
      {"Address or search", "アドレスまたは検索"}, {"Back", "戻る"},
      {"Home", "ホーム"}, {"Reload", "再読み込み"}, {"Search", "検索"}
    }},
    {static_cast<int>(UiLanguage::chinese_simplified), {
      {"File", "文件"}, {"Bookmarks", "书签"}, {"View", "查看"},
      {"Settings", "设置"}, {"Language", "语言"}, {"Search engine", "搜索引擎"},
      {"Help", "帮助"}, {"New tab", "新标签页"}, {"New private window", "新建隐私窗口"},
      {"Close tab", "关闭标签页"}, {"History", "历史记录"}, {"Quit", "退出"},
      {"Find in page", "在页面中查找"}, {"Print", "打印"},
      {"Downloads", "下载"}, {"Full screen", "全屏"},
      {"Address or search", "输入网址或搜索"}, {"Back", "后退"},
      {"Home", "主页"}, {"Reload", "重新加载"}, {"Search", "搜索"}
    }}
  };
  return catalogs.value(static_cast<int>(language)).value(key, key);
}
