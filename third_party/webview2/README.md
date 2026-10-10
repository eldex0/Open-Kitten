# Microsoft Edge WebView2 SDK

This directory contains the minimal native x64 subset of Microsoft.Web.WebView2
SDK 1.0.4258.31 required to compile Open Kitten on Windows:

- `build/native/include/WebView2.h`
- `build/native/include/WebView2EnvironmentOptions.h`
- `build/native/x64/WebView2Loader.dll`
- `build/native/x64/WebView2Loader.dll.lib`

The files originate from the official NuGet package
`Microsoft.Web.WebView2` version `1.0.4258.31`. The Microsoft license and notice
are retained in `LICENSE.txt` and `NOTICE.txt`.

The evergreen WebView2 Runtime itself is not committed to this repository.
Windows must have that runtime installed to use Modern Web mode.
