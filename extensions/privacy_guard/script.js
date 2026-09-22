(() => {
    const options = typeof minibrowserOptions === 'undefined' ? {} : minibrowserOptions;
    if (options.linkProtection === false || globalThis.__minibrowserPrivacyV2) return;
    globalThis.__minibrowserPrivacyV2 = true;
    const protect = anchor => {
        if (!anchor) return;
        anchor.removeAttribute('ping');
        try {
            const url = new URL(anchor.href, location.href);
            if (/^https?:$/.test(url.protocol) && url.origin !== location.origin) {
                anchor.relList.add('noreferrer');
                if (anchor.target === '_blank') anchor.relList.add('noopener');
            }
        } catch (_) { /* Ignore malformed or non-web links. */ }
    };
    // Event delegation also covers links inserted after page load.
    const onLink = event => protect(event.target?.closest?.('a[href]'));
    document.addEventListener('pointerdown', onLink, true);
    document.addEventListener('click', onLink, true);
    document.addEventListener('auxclick', onLink, true);
    document.addEventListener('keydown', event => {
        if (event.key === 'Enter') onLink(event);
    }, true);
})();
