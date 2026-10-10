# Open Kitten 2.0 release definition

Open Kitten 2.0 is the first dual-engine native release:

1. C++20 and Qt own the browser shell and Kitten Safe View.
2. WebView2 supplies an evergreen Chromium backend for modern websites.
3. Rust supplies memory-safe tracking-host and URL-parameter filtering.
4. TypeScript owns internal WebUI sources.

The Windows package contains `OpenKitten.exe`, `WebView2Loader.dll`, the Qt
runtime, the Rust security library and application assets. User profiles and
caches are created only after launch and are never included in release archives.

The version includes tabs, sessions, bookmarks, private windows, downloads,
printing, find-in-page, developer tools, zoom and Kitten Safe View. The desktop
shell uses a compact dark tab strip, unified navigation controls, bookmark bar,
privacy indicator and a Chromium-rendered new-tab page. It does not claim that
Brave-specific hosted services are reimplemented.
