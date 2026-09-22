"""Shared address/search resolution for normal and private navigation."""
from ipaddress import ip_address
from urllib.parse import quote, urlsplit
from core.search_engines import ENGINES, QUERY_KEYS


def resolve_address(text, engine='Google'):
    text = text.strip()
    if not text:
        return ''
    try:
        explicit = text.lower().startswith(('http://', 'https://'))
        parsed = urlsplit(text if explicit else '//' + text)
        host = parsed.hostname or ''
        _ = parsed.port  # Reject malformed ports instead of crashing Qt slots.
        try:
            ip_address(host)
            local = True
        except ValueError:
            local = host.lower() == 'localhost'
        domain = '.' in host and all(part and not any(c.isspace() for c in part)
                                     for part in host.split('.'))
        if not any(c.isspace() for c in text) and host and (explicit or local or domain):
            if parsed.username is None and parsed.password is None:
                return text if explicit else ('http://' if local else 'https://') + text
    except ValueError:
        pass
    return ENGINES.get(engine, ENGINES['Google']) + '?' + QUERY_KEYS.get(engine, 'q') + '=' + quote(text, safe='')
