// DOM helpers. Every piece of user-provided text goes through text nodes,
// never innerHTML, so complaint text can't inject markup.

const SVG_NS = 'http://www.w3.org/2000/svg';

function applyAttrs(el, attrs, isSvg) {
  if (!attrs) return;
  for (const [key, value] of Object.entries(attrs)) {
    if (value === null || value === undefined || value === false) continue;
    if (key === 'class' || key === 'className') {
      if (isSvg) el.setAttribute('class', value);
      else el.className = value;
    } else if (key === 'style' && typeof value === 'object') {
      for (const [prop, v] of Object.entries(value)) {
        if (v === null || v === undefined) continue;
        if (prop.startsWith('--')) el.style.setProperty(prop, v);
        else el.style[prop] = v;
      }
    } else if (key === 'dataset') {
      for (const [k, v] of Object.entries(value)) el.dataset[k] = v;
    } else if (key.startsWith('on') && typeof value === 'function') {
      el.addEventListener(key.slice(2).toLowerCase(), value);
    } else if (key === 'text') {
      el.textContent = value;
    } else if (key === 'value' && !isSvg) {
      el.value = value;
    } else if (key === 'checked' || key === 'selected' || key === 'disabled' || key === 'hidden' || key === 'open') {
      if (!isSvg) el[key] = !!value;
      if (value) el.setAttribute(key, '');
    } else if (value === true) {
      el.setAttribute(key, '');
    } else {
      el.setAttribute(key, String(value));
    }
  }
}

function appendChildren(el, children) {
  for (const child of children) {
    if (child === null || child === undefined || child === false || child === true) continue;
    if (Array.isArray(child)) { appendChildren(el, child); continue; }
    if (child instanceof Node) el.appendChild(child);
    else el.appendChild(document.createTextNode(String(child)));
  }
}

export function h(tag, attrs, ...children) {
  const el = document.createElement(tag);
  if (attrs && (attrs instanceof Node || Array.isArray(attrs) || typeof attrs !== 'object')) {
    children.unshift(attrs);
    attrs = null;
  }
  applyAttrs(el, attrs, false);
  appendChildren(el, children);
  return el;
}

export function s(tag, attrs, ...children) {
  const el = document.createElementNS(SVG_NS, tag);
  if (attrs && (attrs instanceof Node || Array.isArray(attrs) || typeof attrs !== 'object')) {
    children.unshift(attrs);
    attrs = null;
  }
  applyAttrs(el, attrs, true);
  appendChildren(el, children);
  return el;
}

export function clear(el) {
  while (el.firstChild) el.removeChild(el.firstChild);
  return el;
}

export function mount(el, ...children) {
  clear(el);
  appendChildren(el, children);
  return el;
}

// ── Formatting ──────────────────────────────────────────────

const numFmt = new Intl.NumberFormat('en-IN');
export function fmtNum(n) {
  if (n === null || n === undefined || Number.isNaN(n)) return '–';
  return numFmt.format(n);
}

export function fmtPct(x, digits = 0) {
  if (x === null || x === undefined || Number.isNaN(x)) return '–';
  return `${(x * 100).toFixed(digits)}%`;
}

export function fmtDec(x, digits = 1) {
  if (x === null || x === undefined || Number.isNaN(Number(x))) return '–';
  return Number(x).toFixed(digits);
}

const MONTHS = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];

export function parseDate(str) {
  if (!str || typeof str !== 'string') return null;
  const m = str.match(/^(\d{4})-(\d{2})-(\d{2})(?:[ T](\d{2}):(\d{2}))?/);
  if (!m) return null;
  return new Date(+m[1], +m[2] - 1, +m[3], +(m[4] || 0), +(m[5] || 0));
}

export function fmtDate(str, withTime = true) {
  const d = parseDate(str);
  if (!d) return str || '–';
  const base = `${d.getDate()} ${MONTHS[d.getMonth()]} ${d.getFullYear()}`;
  if (!withTime) return base;
  const hh = String(d.getHours()).padStart(2, '0');
  const mm = String(d.getMinutes()).padStart(2, '0');
  return `${base}, ${hh}:${mm}`;
}

export function fmtMonth(key) {
  const m = /^(\d{4})-(\d{2})$/.exec(key || '');
  if (!m) return key;
  return `${MONTHS[+m[2] - 1]} ’${m[1].slice(2)}`;
}

export function daysBetween(a, b) {
  const da = parseDate(a);
  const db = parseDate(b);
  if (!da || !db) return null;
  return (db - da) / 86400000;
}

export function ago(str) {
  const d = parseDate(str);
  if (!d) return '';
  const days = Math.floor((Date.now() - d.getTime()) / 86400000);
  if (days < 0) return fmtDate(str, false);
  if (days === 0) return 'today';
  if (days === 1) return 'yesterday';
  if (days < 31) return `${days} days ago`;
  const months = Math.floor(days / 30.4);
  if (months < 12) return `${months} month${months > 1 ? 's' : ''} ago`;
  const years = Math.floor(months / 12);
  return `${years} year${years > 1 ? 's' : ''} ago`;
}

export const STATUSES = ['PENDING', 'REVIEWED', 'ASSIGNED', 'IN_PROGRESS', 'COMPLETED'];
export const STATUS_LABEL = {
  PENDING: 'Pending',
  REVIEWED: 'Reviewed',
  ASSIGNED: 'Assigned',
  IN_PROGRESS: 'In progress',
  COMPLETED: 'Completed',
};
export const CATEGORIES = ['Water Supply', 'Electricity', 'Roads & Transport', 'Sanitation', 'Public Safety'];
export const ROLE_LABEL = {
  USER: 'Citizen',
  ADMIN: 'Administrator',
  DEPT_OFFICER: 'Department officer',
  WORKER: 'Field worker',
};

export function statusLabel(s) { return STATUS_LABEL[s] || s || '–'; }

export function sevLabel(n) {
  if (n >= 9) return 'Critical';
  if (n >= 7) return 'High';
  if (n >= 5) return 'Medium';
  if (n >= 3) return 'Low';
  return 'Minor';
}
export function sevKey(n) {
  if (n >= 9) return 'critical';
  if (n >= 7) return 'high';
  if (n >= 5) return 'medium';
  if (n >= 3) return 'low';
  return 'minor';
}

export function isUnassigned(c) {
  return c.worker_id === null || c.worker_id === undefined || Number(c.worker_id) <= 0;
}

// ── Toasts ─────────────────────────────────────────────────

export function toast(message, kind = 'info', timeout = 4500) {
  const host = document.getElementById('toasts');
  if (!host) return;
  const icon = kind === 'error' ? '!' : kind === 'success' ? '✓' : 'i';
  const el = h('div', { class: `toast toast-${kind}` },
    h('span', { class: 'toast-icon', 'aria-hidden': 'true' }, icon),
    h('span', { class: 'toast-text' }, message),
    h('button', { class: 'toast-close', type: 'button', 'aria-label': 'Dismiss', onclick: () => el.remove() }, '×'));
  host.appendChild(el);
  if (timeout) setTimeout(() => el.remove(), timeout);
}

// ── Confirm dialog ─────────────────────────────────────────

export function confirmDialog({ title, body, confirmLabel = 'Confirm', cancelLabel = 'Cancel', danger = false }) {
  return new Promise((resolve) => {
    const dlg = h('dialog', { class: 'dialog', 'aria-labelledby': 'dlg-title' });
    const done = (val) => { dlg.close(); dlg.remove(); resolve(val); };
    const form = h('form', { method: 'dialog', class: 'dialog-body', onsubmit: (e) => { e.preventDefault(); done(true); } },
      h('h2', { id: 'dlg-title', class: 'dialog-title' }, title),
      body ? h('p', { class: 'dialog-text' }, body) : null,
      h('div', { class: 'dialog-actions' },
        h('button', { type: 'button', class: 'btn btn-ghost', onclick: () => done(false) }, cancelLabel),
        h('button', { type: 'submit', class: `btn ${danger ? 'btn-danger' : 'btn-primary'}`, autofocus: true }, confirmLabel)));
    dlg.appendChild(form);
    dlg.addEventListener('cancel', (e) => { e.preventDefault(); done(false); });
    document.body.appendChild(dlg);
    dlg.showModal();
  });
}

// ── Tooltip ────────────────────────────────────────────────

export function showTip(content, x, y) {
  const tip = document.getElementById('tooltip');
  if (!tip) return;
  clear(tip);
  appendChildren(tip, Array.isArray(content) ? content : [content]);
  tip.hidden = false;
  const pad = 14;
  const rect = tip.getBoundingClientRect();
  let left = x + pad;
  let top = y + pad;
  if (left + rect.width > window.innerWidth - 8) left = x - rect.width - pad;
  if (top + rect.height > window.innerHeight - 8) top = y - rect.height - pad;
  tip.style.left = `${Math.max(8, left)}px`;
  tip.style.top = `${Math.max(8, top)}px`;
}

export function hideTip() {
  const tip = document.getElementById('tooltip');
  if (tip) tip.hidden = true;
}

// Attach hover + keyboard-focus tooltip to an element (HTML or SVG).
export function bindTip(el, contentFn) {
  el.addEventListener('pointermove', (e) => showTip(contentFn(), e.clientX, e.clientY));
  el.addEventListener('pointerleave', hideTip);
  el.addEventListener('focus', () => {
    const r = el.getBoundingClientRect();
    showTip(contentFn(), r.left + r.width / 2, r.top + r.height / 2);
  });
  el.addEventListener('blur', hideTip);
}

export function tipRows(title, rows) {
  // rows: [{value, label, key?}] — value leads, label follows.
  return [
    title ? h('div', { class: 'tip-title' }, title) : null,
    ...rows.map((r) => h('div', { class: 'tip-row' },
      r.key ? h('span', { class: 'tip-key', style: { background: r.key }, 'aria-hidden': 'true' }) : null,
      h('strong', null, r.value),
      h('span', { class: 'tip-label' }, r.label))),
  ];
}

export function debounce(fn, ms) {
  let t;
  return (...args) => {
    clearTimeout(t);
    t = setTimeout(() => fn(...args), ms);
  };
}

export function prefersReducedMotion() {
  return window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches;
}
