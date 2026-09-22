(() => {
    const options = typeof minibrowserOptions === 'undefined' ? {} : minibrowserOptions;
    if (options.cosmetic === false || document.getElementById('minibrowser-adblock-v2')) return;
    const style = document.createElement('style');
    style.id = 'minibrowser-adblock-v2';
    // CSS handles later insertions without scanning the DOM or polling.
    // Deliberately avoid generic .ad, iframe, video or sponsored-content selectors.
    style.textContent = `ins.adsbygoogle, [data-ad-slot][data-ad-client],
        iframe[src*="//googleads.g.doubleclick.net/"],
        iframe[src*="//tpc.googlesyndication.com/"] { display: none !important; }`;
    (document.head || document.documentElement).appendChild(style);
})();
