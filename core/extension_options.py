from core.i18n import tr
"""Validated settings shared by built-in network filters and scripts."""
import json
import re
from core.host_filter import is_blocked

BUILTINS = ('ad_blocker', 'privacy_guard', 'dark_mode')


def parse_sites(value):
    sites = set()
    for entry in re.split(r'[,;\s]+', value.strip().lower()):
        if not entry:
            continue
        if len(entry) > 253 or not all(re.fullmatch(r'[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?', part)
                                       for part in entry.split('.')):
            raise ValueError(tr('Exceptions : saisir des domaines sans https://, chemin ni joker.'))
        sites.add(entry)
    if len(sites) > 100:
        raise ValueError(tr('Maximum 100 domaines par liste d’exceptions.'))
    return frozenset(sites)


def should_block(host, first_party, groups):
    return any(not is_blocked(first_party, exceptions) and is_blocked(host, hosts)
               for hosts, exceptions in groups)


def configure_script(extension_id, source, settings):
    if extension_id not in BUILTINS:
        return source
    options = {
        'cosmetic': settings.get('ad_cosmetic', True),
        'linkProtection': settings.get('privacy_links', True),
        'respectDark': settings.get('dark_respect_native', True),
        'palette': settings.get('dark_palette', 'neutral'),
    }
    sites = sorted(parse_sites(settings.get(extension_id + '_exceptions', '')))
    return """(() => {
        if (!/^https?:$/.test(location.protocol)) return;
        const excluded = %s;
        const host = location.hostname.toLowerCase().replace(/\\.$/, '');
        if (excluded.some(site => host === site || host.endsWith('.' + site))) return;
        const minibrowserOptions = %s;
        %s
    })();""" % (json.dumps(sites), json.dumps(options), source)
