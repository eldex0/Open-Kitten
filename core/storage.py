import json
import os
import time
from copy import deepcopy
from core.json_store import atomic_write


class Storage:

    def __init__(self, profile_dir):
        self.profile_dir = profile_dir
        self._cache = {}

        os.makedirs(self.profile_dir, exist_ok=True)

        self.bookmarks_file = os.path.join(
            self.profile_dir,
            "bookmarks.json"
        )

        self.history_file = os.path.join(
            self.profile_dir,
            "history.json"
        )

        self.session_file = os.path.join(
            self.profile_dir,
            "session.json"
        )

    # =========================================================
    # JSON
    # =========================================================

    def load_json(self, path, default):
        if path in self._cache:
            return deepcopy(self._cache[path])
        if not os.path.exists(path):
            return default

        try:
            with open(path, "r", encoding="utf-8") as file:
                value = json.load(file)
                if not isinstance(value, type(default)):
                    return deepcopy(default)
                self._cache[path] = value
                return deepcopy(value)

        except Exception:
            return default

    def save_json(self, path, data):
        try:
            if path in self._cache and self._cache[path] == data:
                return
            atomic_write(path, data)
            self._cache[path] = deepcopy(data)

        except Exception as error:
            print("Erreur sauvegarde :", error)

    # =========================================================
    # BOOKMARKS
    # =========================================================

    @staticmethod
    def valid_entries(items):
        if not isinstance(items, list):
            return []
        return [dict(item, title=str(item.get('title') or item['url']))
                for item in items if isinstance(item, dict)
                and isinstance(item.get('url'), str) and item['url']]

    def load_bookmarks(self):
        data = self.load_json(self.bookmarks_file, {"Unsorted": []})
        return {folder: self.valid_entries(items) for folder, items in data.items()
                if isinstance(folder, str)}

    def save_bookmarks(self, bookmarks):
        self.save_json(
            self.bookmarks_file,
            bookmarks
        )

    def add_bookmark(
        self,
        title,
        url,
        folder="Unsorted"
    ):
        bookmarks = self.load_bookmarks()

        if folder not in bookmarks:
            bookmarks[folder] = []

        for item in bookmarks[folder]:
            if item.get("url") == url:
                return

        bookmarks[folder].append({
            "title": title,
            "url": url
        })

        self.save_bookmarks(bookmarks)

    def remove_bookmark(self, url):
        bookmarks = self.load_bookmarks()

        for folder in list(bookmarks.keys()):

            bookmarks[folder] = [
                item
                for item in bookmarks[folder]
                if item.get("url") != url
            ]

        self.save_bookmarks(bookmarks)

    def is_bookmarked(self, url):
        bookmarks = self.load_bookmarks()

        for items in bookmarks.values():
            for item in items:
                if item.get("url") == url:
                    return True

        return False

    # =========================================================
    # HISTORY
    # =========================================================

    def load_history(self):
        return self.valid_entries(self.load_json(self.history_file, []))

    def save_history(self, history):
        self.save_json(
            self.history_file,
            history
        )

    def add_history(self, title, url):
        if not url:
            return

        history = self.load_history()

        # Un rechargement ne doit pas générer une nouvelle écriture disque ni
        # une nouvelle entrée identique en tête de l'historique.
        if history and history[0].get("url") == url:
            return

        history.insert(
            0,
            {
                "title": title or url,
                "url": url,
                "timestamp": time.time()
            }
        )

        # Éviter une histoire infinie
        history = history[:5000]

        self.save_history(history)

    def remove_history_item(self, url):
        history = self.load_history()

        history = [
            item
            for item in history
            if item.get("url") != url
        ]

        self.save_history(history)

    def clear_history(self):
        self.save_history([])

    # =========================================================
    # SESSION
    # =========================================================

    def save_session(self, urls):
        self.save_json(
            self.session_file,
            {
                "tabs": urls
            }
        )

    def load_session(self):
        data = self.load_json(
            self.session_file,
            {
                "tabs": []
            }
        )

        tabs = data.get("tabs", [])
        if not isinstance(tabs, list):
            return []
        return [url for url in tabs if isinstance(url, str)
                and url.startswith(('http://', 'https://'))]

    def clear_session(self):
        self.save_session([])
