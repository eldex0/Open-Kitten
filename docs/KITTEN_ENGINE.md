# Kitten Engine 2.0 contract

Kitten Engine 2.0 is the official dual-mode Open Kitten engine.

## Modern Web mode

The default path embeds the evergreen Microsoft Edge WebView2 Chromium runtime.
It provides modern HTML and CSS, JavaScript, WebAssembly, cookies, Web storage,
audio, video, forms, printing, downloads and Chromium developer tools. The
browser owns the surrounding tabs, address bar, session, bookmarks, private
windows and Rust URL policy.

## Kitten Safe View

Safe View is the independent restricted renderer. Its supported contract is:

- HTTP and HTTPS documents up to 5 MiB
- no downgrade redirect from HTTPS to HTTP
- tolerant HTML parsing with hidden script/style/head content
- named and numeric HTML entity decoding with bounded tree depth and node count
- inline CSS with tag, class, ID and compound selectors such as `p.note.active`
- inherited color, font size, font weights, italic and underline styles
- `display: none` and `visibility: hidden` content suppression
- block layout, headings, paragraphs, lists, links and image alternative text
- native font measurement and painting through Qt
- safe relative-link resolution including query, fragment and `..` segments
- tabs, independent history, reload/stop, home, bookmarks and session restore
- tracking-query cleanup supplied by the Rust security core
- a browser-facing protection panel with per-page blocked-resource counts
- no remote JavaScript, WebAssembly, cookies or Web storage in Safe View

The last item is the Safe View security boundary. Modern Web mode executes
scripts inside the Chromium process sandbox supplied by the WebView2 runtime.

## Version policy

Patch releases preserve both modes and fix defects. Changes to process
isolation, profile persistence or scripting require a security review.
