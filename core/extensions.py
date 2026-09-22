import json
import os
from core.extension_packages import read_package, install_package
from core.json_store import atomic_write
from core.extension_options import parse_sites, should_block, configure_script

from PyQt6.QtWebEngineCore import (
    QWebEngineScript,
    QWebEngineUrlRequestInterceptor,
    QWebEngineUrlRequestInfo,
)


class TrackingInterceptor(QWebEngineUrlRequestInterceptor):

    def __init__(self, groups, parent=None):
        super().__init__(parent)
        self.groups = tuple(groups)

    def interceptRequest(self, info):
        host = info.requestUrl().host().lower()

        first_party = info.firstPartyUrl().host().lower()
        if info.resourceType() == QWebEngineUrlRequestInfo.ResourceType.ResourceTypeMainFrame:
            first_party = host
        if should_block(host, first_party, self.groups):
            info.block(True)


class ExtensionManager:

    def __init__(self, extensions_dir, state_file=None, settings=None):
        self.settings = settings if settings is not None else {}
        self.extensions_dir = extensions_dir
        self.state_file = state_file or os.path.join(
            extensions_dir,
            "state.json"
        )
        self.interceptor = None
        self.user_extensions_dir = os.path.join(os.path.dirname(self.state_file), 'installed-extensions')

        os.makedirs(os.path.dirname(self.state_file), exist_ok=True)
        # Preserve existing development preferences, without shipping them.
        legacy = os.path.join(extensions_dir, 'state.json')
        if state_file and not os.path.exists(state_file) and os.path.isfile(legacy):
            import shutil
            shutil.copy2(legacy, state_file)

    def load_state(self):
        if not os.path.exists(self.state_file):
            return {}

        try:
            with open(self.state_file, "r", encoding="utf-8") as file:
                state = json.load(file)
                return state if isinstance(state, dict) else {}
        except (OSError, json.JSONDecodeError):
            return {}

    def save_state(self, state):
        atomic_write(self.state_file, state)

    def list_extensions(self):
        extensions = []
        state = self.load_state()

        candidates = []
        for directory in (self.extensions_dir, self.user_extensions_dir):
            if os.path.isdir(directory):
                candidates.extend((name, os.path.join(directory, name))
                                  for name in os.listdir(directory) if not name.startswith('.'))

        for extension_id, path in candidates:
            manifest = os.path.join(path, "manifest.json")

            if not os.path.isdir(path) or not os.path.exists(manifest):
                continue

            try:
                data, _ = read_package(path)
            except (OSError, ValueError, UnicodeError):
                continue

            if not isinstance(data, dict):
                continue
            hosts = data.get('blocked_hosts', [])
            if not isinstance(hosts, list) or not all(isinstance(h, str) and h for h in hosts):
                continue

            extensions.append({
                "id": extension_id,
                "name": data.get("name", extension_id),
                "version": data.get("version", "1.0"),
                "description": data.get("description", ""),
                "enabled": state.get(
                    extension_id,
                    data.get("enabled_by_default", False)
                ),
                "path": path,
                "script": data.get("script", ""),
                "blocked_hosts": data.get("blocked_hosts", []),
            })

        return sorted(
            extensions,
            key=lambda extension: extension["name"].lower()
        )

    def install_from_folder(self, folder):
        return install_package(folder, self.user_extensions_dir)

    def set_enabled(self, extension_id, enabled):
        state = self.load_state()
        state[extension_id] = enabled
        self.save_state(state)

    def apply_to_profile(self, profile):
        scripts = profile.scripts()
        for previous in scripts.toList():
            if previous.name().startswith('minibrowser-'):
                scripts.remove(previous)

        groups = []

        for extension in self.list_extensions():
            if not extension["enabled"]:
                continue

            if extension['blocked_hosts']:
                groups.append((frozenset(extension['blocked_hosts']),
                               parse_sites(self.settings.get(extension['id'] + '_exceptions', ''))))

            script_name = extension["script"]

            if script_name:
                script_path = os.path.join(
                    extension["path"],
                    script_name
                )
                extension_root = os.path.realpath(extension['path'])
                if os.path.commonpath([extension_root, os.path.realpath(script_path)]) != extension_root:
                    continue

                try:
                    with open(
                        script_path,
                        "r",
                        encoding="utf-8"
                    ) as file:
                        source = file.read()
                except OSError:
                    continue

                script = QWebEngineScript()
                script.setName(
                    "minibrowser-" + extension["id"]
                )
                script.setSourceCode(configure_script(extension['id'], source, self.settings))
                script.setInjectionPoint(
                    QWebEngineScript.InjectionPoint.DocumentReady
                )
                script.setWorldId(
                    QWebEngineScript.ScriptWorldId.ApplicationWorld
                )
                scripts.insert(script)

        old_interceptor = self.interceptor
        if groups:
            self.interceptor = TrackingInterceptor(
                groups,
                profile
            )
            profile.setUrlRequestInterceptor(self.interceptor)
        else:
            self.interceptor = None
            profile.setUrlRequestInterceptor(None)
        if old_interceptor is not None:
            old_interceptor.deleteLater()
