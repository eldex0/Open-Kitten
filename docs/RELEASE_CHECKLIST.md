# Open Kitten 2.0 native release checklist

- C++20 Release build succeeds with warnings treated as actionable.
- `ctest` passes the HTML, CSS, URL and layout checks.
- `cargo test --locked` passes and the Rust library is copied beside the executable.
- TypeScript strict checking and build pass.
- The installed folder contains the Qt runtime and starts on a clean Windows VM.
- `OpenKitten.exe --self-test` confirms that the packaged Rust bridge loads.
- `OpenKitten.exe --ui-smoke-test` confirms that the Chromium controller starts.
- Tabs, back/forward, reload, address search, bookmarks and session restore work.
- HTTPS certificate failures remain visible and HTTP downgrade redirects are blocked.
- A missing Rust library is reported instead of silently claiming protection.
- `WebView2Loader.dll` is packaged and the evergreen WebView2 Runtime is detected.
- No Python, PyQt, QtWebEngine, user profile, cache or old logo draft is packaged.
- SHA-256 hashes are generated for every packaged file.
- The x64 installer compiles, installs, passes `--self-test` and uninstalls cleanly.
