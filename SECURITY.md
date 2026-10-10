# Security policy

Open Kitten 2.0 has two security domains. Modern Web mode runs active content in
the installed WebView2 Chromium runtime and its multiprocess sandbox. Kitten
Safe View accepts only HTTP and HTTPS navigation, rejects unsafe URLs, limits
documents to 5 MiB and does not execute remote JavaScript or WebAssembly.

The Rust `kitten-security-core` library removes common tracking parameters and
recognizes known tracking hosts through a narrow C ABI. The browser reports at
runtime whether that component loaded successfully.

## Important limits

Kitten Safe View parsing, networking, layout and painting run inside the browser
process and are not a Chromium security boundary. Modern Web mode depends on the
security updates of the evergreen WebView2 Runtime. TLS and certificate handling
come from Chromium in Modern mode and Qt Network in Safe View.

## Reporting

Do not publish exploit details before maintainers have had a reasonable chance
to respond. Include the Open Kitten version, Windows version, target URL or a
minimal local reproduction, and whether Kitten Security Core was active. Never
attach private profiles, cookies or browsing history.
