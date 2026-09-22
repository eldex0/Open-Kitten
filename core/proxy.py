import os


def configure_proxy_environment(settings):

    enabled = settings.get(
        "proxy_enabled",
        False
    )

    if not enabled:
        return

    proxy_type = settings.get(
        "proxy_type",
        "HTTP"
    )

    host = settings.get(
        "proxy_host",
        ""
    )

    port = settings.get(
        "proxy_port",
        ""
    )

    if not host or not port:
        return

    proxy_type = proxy_type.upper()

    if proxy_type in ("HTTP", "HTTPS"):
        scheme = "http"

    elif proxy_type == "SOCKS5":
        scheme = "socks5"

    else:
        scheme = "http"

    proxy = f"{scheme}://{host}:{port}"

    existing_flags = os.environ.get(
        "QTWEBENGINE_CHROMIUM_FLAGS",
        ""
    )

    proxy_flag = f"--proxy-server={proxy}"

    if proxy_flag not in existing_flags:
        existing_flags = (
            existing_flags + " " + proxy_flag
        ).strip()

    os.environ[
        "QTWEBENGINE_CHROMIUM_FLAGS"
    ] = existing_flags