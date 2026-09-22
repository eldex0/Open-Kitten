# Open Kitten

**Open Kitten 1.0.0** is a small, open-source desktop web browser for Windows.
It is an experimental project made for fun and learning, developed with Python,
PyQt6, QtWebEngine, and the help of AI.

> Open Kitten is not intended to replace mature production browsers yet. Use it
> as an experimental project and report anything that can be improved (for a more serious version.).

Copyright (C) 2026 eldex0. Open Kitten is released under the
[GNU GPL v3.0-or-later](LICENSE).

## Highlights

- Tabbed browsing with a combined address and search bar
- Multiple search engines, including Google, Bing, DuckDuckGo, Brave Search,
  Ecosia, Qwant, and Startpage
- Private browsing windows and separate browser profiles
- Bookmarks, history, downloads, zoom controls, and keyboard shortcuts
- Light, dark, and system appearance modes
- Automatic interface language detection with several language options
- Built-in extension support and three optional privacy-oriented extensions
- A customizable interface inspired by privacy-focused browsers
- Windows packaging support with PyInstaller and Inno Setup

## Project status

Open Kitten is actively experimental. The codebase is evolving and some browser
features depend on the QtWebEngine version and the codecs available on the host
system. DRM, proprietary media codecs, automatic updates, and Chrome Web Store
compatibility are not guaranteed.

The built-in extensions use Open Kitten's extension format. They are not a full
implementation of Chrome or Firefox extension APIs.

## Run from source

Python 3.10 or newer is recommended on Windows.

```powershell
git clone https://github.com/eldex0/open-kitten.git
cd open-kitten
python -m venv .venv
.\\.venv\\Scripts\\Activate.ps1
python -m pip install -r requirements.txt
python main.py
```

If PowerShell blocks activation, run the project with the Python executable
inside `.venv` directly or adjust the local PowerShell execution policy.

## Run the tests

```powershell
python -m unittest discover -s tests
```

## Build the Windows application

Install PyInstaller and run:

```powershell
python -m pip install pyinstaller
python -m PyInstaller packaging\\MiniBrowser.spec
```

The application is generated in `dist\\OpenKitten\\OpenKitten.exe`.

To create the installer, install Inno Setup 6 and compile
`packaging\\installer.iss` with the Inno Setup compiler (`ISCC.exe`).

## Repository layout

```text
core/          Application services, settings, storage, search, and i18n
ui/            Browser windows, tabs, dialogs, and interface components
extensions/    Built-in Open Kitten extensions
packaging/     PyInstaller and Windows installer configuration
tests/         Automated tests
docs/          Project and release documentation
```

Do not commit personal browsing data such as cookies, history, profiles,
extension state, logs, or local build output.

## Contributing

Bug reports, ideas, translations, and pull requests are welcome. Please explain
how to reproduce a problem and include the relevant Windows, Python, PyQt6, and
QtWebEngine versions when reporting a bug.

## License

Open Kitten's original source code is distributed under the
[GNU General Public License v3.0 or later](LICENSE). Qt, PyQt6, QtWebEngine,
Chromium, Python, and other third-party components remain under their own
licenses. See the relevant upstream project for each dependency's terms.
