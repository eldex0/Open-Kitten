from core.i18n import tr
import json
import zipfile
from pathlib import Path, PurePosixPath


def inspect_webextension(source):
    path = Path(source)
    if path.is_dir():
        manifest = path / 'manifest.json'
        if manifest.stat().st_size > 1048576:
            raise ValueError(tr('Manifeste trop volumineux.'))
        text = manifest.read_text(encoding='utf-8-sig')
    elif path.suffix.lower() == '.zip':
        with zipfile.ZipFile(path) as archive:
            entries = archive.infolist()
            if len(entries) > 10000 or sum(e.file_size for e in entries) > 100 * 1024 * 1024:
                raise ValueError(tr('Archive trop volumineuse (100 Mo décompressés maximum).'))
            for entry in entries:
                parts = PurePosixPath(entry.filename.replace('\\', '/'))
                if parts.is_absolute() or '..' in parts.parts or ':' in entry.filename:
                    raise ValueError(tr('Chemin non autorisé dans l’archive.'))
                if (entry.external_attr >> 16) & 0o170000 == 0o120000:
                    raise ValueError(tr('Les liens symboliques ne sont pas autorisés.'))
            if sum(e.filename == 'manifest.json' for e in entries) != 1:
                raise ValueError(tr('Un manifest.json est requis à la racine du ZIP.'))
            if archive.getinfo('manifest.json').file_size > 1048576:
                raise ValueError(tr('Manifeste trop volumineux.'))
            text = archive.read('manifest.json').decode('utf-8-sig')
    else:
        raise ValueError(tr('Choisissez un dossier ou un ZIP de la version Chrome/Chromium. CRX et XPI non pris en charge.'))
    data = json.loads(text)
    if not isinstance(data, dict) or data.get('manifest_version') != 3:
        raise ValueError(tr('Le moteur accepte uniquement les extensions Manifest V3. Manifest V2 non pris en charge.'))
    if not all(isinstance(data.get(key), str) and data[key] for key in ('name', 'version')):
        raise ValueError(tr('Nom ou version manquant dans le manifeste.'))
    if 'browser_specific_settings' in data or 'applications' in data:
        raise ValueError(tr('Ce paquet cible Firefox. Utilisez sa version Chrome/Chromium Manifest V3.'))
    if not all(isinstance(data.get(key, []), list) for key in ('permissions', 'host_permissions')):
        raise ValueError(tr('Permissions invalides.'))
    permissions = data.get('permissions', []) + data.get('host_permissions', [])
    if not all(isinstance(p, str) for p in permissions):
        raise ValueError(tr('Permissions invalides.'))
    return data['name'], permissions
