"""Self-contained start page: no external assets or privileged JavaScript bridge."""
from html import escape
from urllib.parse import urlsplit
from core.appearance import accent_color
from core.search_engines import ENGINES, QUERY_KEYS
from core.i18n import tr, resolve_language


def render_homepage(settings, storage, logo):
    engine = settings.get('search_engine', 'Google')
    if engine not in ENGINES:
        engine = 'Google'
    query_key = QUERY_KEYS.get(engine, 'q')
    language = resolve_language(settings.get('ui_language', 'auto'))
    search_label = escape(tr('Rechercher', language))
    direction = 'rtl' if language == 'ar' else 'ltr'
    entries = [item for items in storage.load_bookmarks().values() for item in items]
    if not entries and settings.get('home_history_suggestions', True):
        entries = storage.load_history()
    limit = settings.get('home_shortcut_count', 8)
    cards = []
    seen = set()
    for item in entries:
        if len(cards) >= limit:
            break
        if not isinstance(item, dict):
            continue
        url = str(item.get('url', ''))
        try:
            parsed = urlsplit(url)
            if parsed.scheme not in ('http', 'https') or not parsed.hostname or url in seen:
                continue
        except ValueError:
            continue
        seen.add(url)
        title = str(item.get('title') or url)
        cards.append(f'<a class="card" href="{escape(url, quote=True)}">'
                     f'<span class="initial">{escape(title[:1].upper())}</span>'
                     f'<span>{escape(title[:28])}</span></a>')
    content = ''.join(cards) or '<p class="empty">' + escape(tr('Vos favoris apparaîtront ici.', language)) + '</p>'
    if limit == 0:
        content = ''
    logo_html = f'<div class="logo" aria-hidden="true">{logo}</div>' if settings.get('home_show_logo', True) else ''
    accent = accent_color(settings)
    button_text = '#17191e' if settings.get('theme', 'dark') == 'dark' else '#ffffff'
    theme = 'light' if settings.get('theme', 'dark') == 'light' else 'dark'
    return f'''<!doctype html>
<html lang="{language}" dir="{direction}" class="{theme}"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Open Kitten</title>
<style>
* {{ box-sizing: border-box; }}
:root {{ color-scheme: dark; --bg:#17191e; --panel:#21242b; --text:#edf0f5; --muted:#a3a9b7; --line:#363b46; --accent:#92aaff; }}
.light {{ color-scheme:light; --bg:#f6f7f9; --panel:#fff; --text:#242831; --muted:#646b79; --line:#dce0e7; --accent:#315bdb; }}
html.dark, html.light {{ --accent:{accent}; --button-text:{button_text}; }}
body {{ margin:0; min-height:100vh; background:var(--bg); color:var(--text); font:16px 'Segoe UI',Arial,sans-serif; padding:clamp(48px,12vh,120px) 24px; }}
main {{ max-width:680px; margin:auto; text-align:center; }}
.logo {{ width:72px; height:72px; margin:auto; }} .logo svg {{ width:100%;height:100%; }}
h1 {{ font-size:26px; letter-spacing:-1px; margin:12px 0 8px; }}
.intro {{ color:var(--muted); margin:0 0 30px; }}
form {{ display:flex; align-items:center; gap:8px; padding:8px; background:var(--panel); border:1px solid var(--line); border-radius:10px; }}
form:focus-within {{ outline:2px solid var(--accent); outline-offset:3px; }}
input {{ width:100%; min-width:0; background:transparent; color:var(--text); border:0; outline:0; padding:14px; font:inherit; }}
input::placeholder {{ color:var(--muted); }}
button {{ background:var(--accent); color:var(--button-text); border:0; border-radius:10px; padding:14px 18px; font:inherit; cursor:pointer; }}
button:hover {{ opacity:.9; }} button:focus-visible,a:focus-visible {{ outline:3px solid var(--accent); outline-offset:4px; }}
.hint {{ font-size:13px; color:var(--muted); text-align:left; margin:12px 8px 32px; }}
.grid {{ display:grid; grid-template-columns:repeat(auto-fit,minmax(150px,1fr)); gap:14px; }}
.card {{ padding:16px 12px; border:1px solid var(--line); border-radius:8px; background:var(--panel); color:var(--text); text-decoration:none; display:flex; align-items:center; flex-direction:column; gap:12px; overflow-wrap:anywhere; }}
.card:hover {{ border-color:var(--accent); }}
.initial {{ width:40px; height:40px; display:grid; place-items:center; border-radius:12px; background:#315bdb14; color:var(--accent); font-weight:700; }}
.empty {{ grid-column:1/-1; color:var(--muted); }}
@media(max-width:480px) {{ body {{ padding:28px 16px; }} button {{ padding:14px 10px; }} }}
</style></head><body><main>
{logo_html}
<h1>Open Kitten</h1><p class="intro"></p>
<form action="{ENGINES[engine]}" method="get" accept-charset="UTF-8">
<input type="search" name="{query_key}" aria-label="{search_label} · {engine}" placeholder="{search_label} · {engine}…" required autocomplete="off" maxlength="2048">
<button type="submit">{search_label}</button></form>
<p class="hint">{escape(tr('Ctrl + L pour la barre d’adresse · Ctrl + T pour un nouvel onglet', language))}</p>
<div class="grid">{content}</div></main></body></html>'''
