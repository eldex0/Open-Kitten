(() => {
    const options = typeof minibrowserOptions === 'undefined' ? {} : minibrowserOptions;
    if (document.getElementById('minibrowser-dark-v2')) return;
    const root = document.documentElement;
    const body = document.body;
    if (!body) return;
    const bodyStyle = getComputedStyle(body);
    const rootStyle = getComputedStyle(root);
    const color = bodyStyle.backgroundColor === 'rgba(0, 0, 0, 0)'
        ? rootStyle.backgroundColor : bodyStyle.backgroundColor;
    const channels = color.match(/[\d.]+/g)?.map(Number) || [];
    const opaque = channels.length === 3 || (channels.length === 4 && channels[3] > 0.5);
    const darkBackground = opaque && channels.slice(0, 3).every(value => value < 90);
    if (options.respectDark !== false &&
        (rootStyle.colorScheme.trim() === 'dark' || darkBackground)) return;
    const warm = options.palette === 'warm';
    const bg = warm ? '#24201c' : '#181b21';
    const panel = warm ? '#302a24' : '#242832';
    const text = warm ? '#eee4d4' : '#e5e8ef';
    const style = document.createElement('style');
    style.id = 'minibrowser-dark-v2';
    style.textContent = `
        :root { color-scheme: dark; }
        html, body, main, article, section, header, footer, aside, nav {
            background-color: ${bg} !important; color: ${text} !important;
        }
        p, h1, h2, h3, h4, h5, h6, li, label { color: ${text} !important; }
        input:not([type=checkbox]):not([type=radio]), textarea, select, pre, code {
            background-color: ${panel} !important; color: ${text} !important;
            border-color: #626775 !important;
        }
        a { color: #a9c4ff !important; }
        a:visited { color: #d0b8ec !important; }
    `;
    (document.head || document.documentElement).appendChild(style);
})();
