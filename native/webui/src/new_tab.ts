type Shortcut = Readonly<{ label: string; url: string }>;

const shortcuts: readonly Shortcut[] = [
  { label: "Documentation", url: "https://github.com/" },
  { label: "Vie privée", url: "kitten://privacy" },
];

function createShortcut(shortcut: Shortcut): HTMLAnchorElement {
  const link = document.createElement("a");
  link.className = "shortcut";
  link.href = shortcut.url;
  link.textContent = shortcut.label;
  return link;
}

export function renderNewTab(root: HTMLElement): void {
  const title = document.createElement("h1");
  title.textContent = "Open Kitten";
  const grid = document.createElement("nav");
  grid.ariaLabel = "Raccourcis";
  grid.append(...shortcuts.map(createShortcut));
  root.replaceChildren(title, grid);
}

const root = document.querySelector<HTMLElement>("#app");
if (root) renderNewTab(root);
