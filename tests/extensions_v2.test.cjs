const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

function page(options = {}, background = 'rgb(255, 255, 255)') {
    const nodes = new Map(), events = {};
    const root = {appendChild(node) { nodes.set(node.id, node); }};
    const document = { head: root, documentElement: root, body: {},
        createElement: () => ({}), getElementById: id => nodes.get(id),
        addEventListener: (name, fn) => { events[name] = fn; }};
    const context = vm.createContext({document, minibrowserOptions: options, URL,
        location: {href: 'https://example.com/', origin: 'https://example.com'},
        getComputedStyle: () => ({backgroundColor: background, colorScheme: 'normal'})});
    return {context, nodes, events};
}
function run(name, env) {
    vm.runInContext(fs.readFileSync(path.join(__dirname, '../extensions', name, 'script.js'), 'utf8'), env.context);
}
test('ad filter installs once and avoids generic media selectors', () => {
    const env = page(); run('ad_blocker', env); run('ad_blocker', env);
    assert.equal(env.nodes.size, 1);
    assert.match(env.nodes.get('minibrowser-adblock-v2').textContent, /adsbygoogle/);
    assert.doesNotMatch(env.nodes.get('minibrowser-adblock-v2').textContent, /video|canvas/);
});
test('cosmetic filtering is optional', () => {
    const env = page({cosmetic: false}); run('ad_blocker', env); assert.equal(env.nodes.size, 0);
});
test('dark mode keeps media uninverted and supports warm palette', () => {
    const env = page({palette: 'warm'}); run('dark_mode', env); run('dark_mode', env);
    assert.equal(env.nodes.size, 1);
    const css = env.nodes.get('minibrowser-dark-v2').textContent;
    assert.match(css, /#24201c/); assert.doesNotMatch(css, /invert\(|filter:|img|video|canvas/);
});
test('native dark pages are preserved unless requested otherwise', () => {
    const env = page({}, 'rgb(20, 20, 20)'); run('dark_mode', env); assert.equal(env.nodes.size, 0);
    env.context.minibrowserOptions.respectDark = false;
    run('dark_mode', env); assert.equal(env.nodes.size, 1);
});
test('link protection preserves URL and other rel values', () => {
    const env = page(); run('privacy_guard', env);
    const rel = new Set(['nofollow']), removed = [];
    const anchor = {href: 'https://other.test/?token=untouched', target: '_blank',
        relList: {add: item => rel.add(item)}, removeAttribute: key => removed.push(key)};
    env.events.click({target: {closest: () => anchor}});
    assert.equal(anchor.href, 'https://other.test/?token=untouched');
    assert.deepEqual(removed, ['ping']);
    assert.ok(rel.has('nofollow') && rel.has('noreferrer') && rel.has('noopener'));
});
test('link protection is optional', () => {
    const env = page({linkProtection: false}); run('privacy_guard', env);
    assert.equal(Object.keys(env.events).length, 0);
});
