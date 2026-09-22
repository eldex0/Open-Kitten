from core.i18n import tr
"""Validated, data-only installation of MiniBrowser extension folders."""
import hashlib
import json
import os
import re
import tempfile
from pathlib import Path


def read_package(folder):
    root = Path(folder).resolve(strict=True)
    manifest = root / 'manifest.json'
    if manifest.is_symlink() or manifest.resolve().parent != root:
        raise ValueError(tr('Le manifeste doit être dans le dossier sélectionné.'))
    if manifest.stat().st_size > 65536:
        raise ValueError(tr('Manifeste trop volumineux (64 Ko maximum).'))
    data = json.loads(manifest.read_text(encoding='utf-8-sig'))
    if not isinstance(data, dict):
        raise ValueError(tr('Le manifeste doit être un objet JSON.'))
    allowed = {'name', 'version', 'description', 'script', 'blocked_hosts', 'enabled_by_default'}
    if set(data) - allowed:
        raise ValueError(tr('Format non pris en charge. Utilisez un manifeste MiniBrowser, pas Chrome/Firefox.'))
    for key, limit in [('name', 100), ('version', 40), ('description', 1000)]:
        value = data.get(key, '' if key == 'description' else None)
        if not isinstance(value, str) or len(value) > limit or (key != 'description' and not value.strip()):
            raise ValueError(tr('Champ invalide : ') + key)
    if not isinstance(data.get('enabled_by_default', False), bool):
        raise ValueError(tr('enabled_by_default doit être un booléen.'))
    hosts = data.get('blocked_hosts', [])
    if not isinstance(hosts, list) or len(hosts) > 10000:
        raise ValueError(tr('Liste de domaines invalide.'))
    for host in hosts:
        if not isinstance(host, str) or len(host) > 253 or not re.fullmatch(
                r'[a-z0-9](?:[a-z0-9-]*[a-z0-9])?(?:\.[a-z0-9](?:[a-z0-9-]*[a-z0-9])?)+', host):
            raise ValueError(tr('Domaine invalide : utiliser un domaine en minuscules, sans URL ni joker.'))
    name = data.get('script', '')
    if not isinstance(name, str):
        raise ValueError(tr('Nom de script invalide.'))
    source = None
    if name:
        if not re.fullmatch(r'[A-Za-z0-9_-][A-Za-z0-9_.-]*\.js', name):
            raise ValueError(tr('Le script doit être un fichier .js directement dans le dossier.'))
        path = root / name
        if path.is_symlink() or path.resolve().parent != root:
            raise ValueError(tr('Le script ne peut pas être un lien externe.'))
        if path.stat().st_size > 1048576:
            raise ValueError(tr('Script trop volumineux (1 Mo maximum).'))
        source = path.read_text(encoding='utf-8-sig')
    if not name and not hosts:
        raise ValueError(tr('Ajoutez un script ou des domaines à bloquer.'))
    return data, source


def install_package(folder, destination):
    data, source = read_package(folder)
    # Content identity prevents collisions with bundled extension names.
    data['enabled_by_default'] = False
    encoded = json.dumps(data, sort_keys=True, ensure_ascii=False)
    identity = 'user-' + hashlib.sha256((encoded + '\0' + (source or '')).encode()).hexdigest()
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=True)
    target = destination / identity
    if target.exists():
        raise ValueError(tr('Cette version de cette extension est déjà installée.'))
    with tempfile.TemporaryDirectory(prefix='.install-', dir=destination) as staging:
        payload = Path(staging) / 'package'
        payload.mkdir()
        (payload / 'manifest.json').write_text(encoded, encoding='utf-8')
        if source is not None:
            (payload / data['script']).write_text(source, encoding='utf-8')
        os.rename(payload, target)
    return identity
