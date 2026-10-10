# Open Kitten and Brave feature parity

Open Kitten 2.x is a native C++/Qt browser shell with a Rust security component.
Kitten Engine combines a Chromium WebView2 backend for normal browsing with the
restricted, text-first Kitten Safe View.

## Available now

- Tabs, session restore and reopening the last ten closed tabs
- Address and search field, navigation history, reload and stop
- HTTP and HTTPS status indicator
- Site protection panel with per-tab controls and blocked-resource count
- Modern Chromium rendering with JavaScript, WebAssembly and media
- Restricted Safe View with manual redirects and a five MiB limit
- Page zoom from 50 to 200 percent
- Add, open and remove bookmarks
- French and English interface
- DuckDuckGo, Brave Search and Qwant selection persisted across sessions
- Privacy dashboard and engine-details page on the protected new tab
- Private browsing windows, downloads, printing and developer tools
- Explicit camera, microphone, location, notification, clipboard and file
  permission prompts
- New-window links routed to tabs, plus duplicate and bulk-close tab actions
- Native C++20 UI and engine, with Rust URL and tracking protection
- Searchable per-profile history and a complete settings dialog
- Isolated named profiles, WebView2 password saving and strict tracking controls
- Pinned tabs and lightweight color-coded tab groups
- Chromium/Firefox bookmark import and portable Open Kitten profile export/import
- Local unpacked Chromium extension installation
- Clearing cookies, cache, permissions and site storage per profile

## Still different from Brave

Open Kitten now has a maintained Chromium backend, but it is not a fork of the
Brave source tree. Its extension support is limited to local unpacked extensions,
and profile export/import is manual rather than a hosted encrypted Sync service.
Automatic application updates still require a signed release feed. Wallet,
Rewards, VPN, Talk and Tor are intentionally excluded from this focused build.
These differences must not be represented as complete Brave parity.
