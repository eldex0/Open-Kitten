# Build from the repository root: python -m PyInstaller packaging/MiniBrowser.spec
from pathlib import Path
root = Path(SPECPATH).parent
datas = [(str(root / 'assets' / 'icon.svg'), 'assets')]
for name in ('ad_blocker', 'privacy_guard', 'dark_mode'):
    for filename in ('manifest.json', 'script.js'):
        source = root / 'extensions' / name / filename
        if source.is_file():
            datas.append((str(source), 'extensions/' + name))
a = Analysis([str(root / 'main.py')], pathex=[str(root)], datas=datas,
             binaries=[], hiddenimports=[], excludes=[], noarchive=False)
pyz = PYZ(a.pure)
exe = EXE(pyz, a.scripts, [], exclude_binaries=True, name='OpenKitten',
          debug=False, strip=False, upx=False, console=False)
coll = COLLECT(exe, a.binaries, a.datas, strip=False, upx=False, name='OpenKitten')
