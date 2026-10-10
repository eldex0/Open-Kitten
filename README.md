# Open Kitten 2.0

Open Kitten is a native Windows browser built with the same primary language
families used by Brave: C++ for the browser and rendering engine, Rust for
security-sensitive URL filtering, and TypeScript for internal WebUI sources.
Python and PyQt are not part of the application or build.

## Kitten Engine 2.0

Kitten Engine 2.0 combines two C++ rendering paths. Modern websites use the
installed Microsoft Edge WebView2 Chromium runtime, providing current HTML,
CSS, JavaScript, WebAssembly, media, forms, cookies and Web storage. Kitten Safe
View remains an independent text-first renderer for restricted reading without
remote scripts. Rust supplies URL cleanup and tracking-host protection.

The browser includes tabs, Chromium history, session restore, bookmarks,
private windows, downloads, printing, find-in-page, developer tools, zoom,
full screen, reopening closed tabs, connection status and French/English UI.
Profiles isolate Chromium storage and Open Kitten preferences. The browser also
supports searchable profile history, pinned and grouped tabs, importing browser
bookmarks, installing local unpacked Chromium extensions, WebView2 password
autofill controls, strict tracking prevention and a portable profile export for
moving non-secret data between devices.
Its site-protection panel reports blocked resources and provides direct access
to the isolated Safe View. DuckDuckGo, Brave Search and Qwant can be selected
as the persistent address-bar search provider.
Its compact dark shell places the tab strip above a unified address bar, adds a
bookmark bar, a downloads control and a privacy-dashboard new-tab page. Links that
request a new window open in a new tab, sensitive site permissions require an
explicit decision, and the tab context menu supports reload, duplicate and
bulk close actions.

The interface can be switched between French, English, Spanish, German,
Italian, Portuguese, Dutch, Polish, Japanese and Simplified Chinese. The
settings also provide a persistent rendering choice between Kitten Engine for
full Chromium compatibility and the restricted Kitten Safe View.

Kitten Safe View remains deliberately limited. The normal rendering path is
Chromium-compatible, but Brave-specific hosted services are separate products.
The exact gap is documented in `docs/BRAVE_PARITY.md`.

## Build on Windows

Required tools:

- CMake 3.24 or newer (Ninja or Visual Studio 2022 generator)
- a C++20 compiler (MSVC or Clang)
- Qt 6.6 or newer: Core, Gui, Network and Widgets
- stable Rust with Cargo
- Node.js 20 or newer with npm or pnpm
- Inno Setup 6 for the Windows installer
- Microsoft Edge WebView2 Runtime (evergreen)

The minimal native WebView2 SDK required for compilation is retained under
`third_party/webview2`; .NET, x86, ARM and code-generation files are excluded.

```powershell
./packaging/build.ps1
```

The portable application is installed into `dist/OpenKitten`; a portable ZIP
and the Windows installer are written to `release`. The build runs the C++, Rust
and TypeScript checks plus packaged self-tests for the Rust bridge, the normal UI
and private mode before creating the release artifacts. The same native quality
gates run in Windows GitHub Actions.

## Source layout

```text
native/include/       Kitten Engine public C++ interfaces
native/src/engine/    HTML, CSS, URL and layout implementation
native/src/browser/   Qt browser shell, tabs and Rust bridge
native/rust/          Memory-safe privacy component
native/webui/         Internal TypeScript WebUI sources
native/tests/         Native engine tests
assets/               Application icons
packaging/            Native Windows build and installer files
```

## License

Open Kitten source code is distributed under the GNU GPL v3.0 or later. Qt and
other third-party components remain under their respective licenses.
