# Native architecture

Open Kitten 2.0 has one native production path:

- C++20 owns the browser lifecycle and Kitten Engine.
- Qt 6 provides windows, fonts, painting, TLS and asynchronous networking.
- WebView2 embeds the installed Chromium runtime for modern sites.
- Rust owns tracking-host and tracking-parameter filtering behind a C ABI.
- TypeScript owns internal WebUI sources.

There is no Python fallback. CMake builds the Rust dynamic library with Cargo,
copies it beside `OpenKitten.exe`, deploys `WebView2Loader.dll`, and deploys the required Qt runtime during
installation. The security bridge remains observable at runtime so a missing or
invalid Rust library cannot silently claim protection.

The engine core is Qt-independent and testable without the UI. Browser policy
and persistence stay in the Qt adapter rather than leaking into the parser or
layout library.
