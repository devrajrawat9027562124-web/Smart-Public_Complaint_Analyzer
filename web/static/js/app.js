// App bootstrap: session, hash router, role-aware shell.

import { h, s, mount, toast, ROLE_LABEL } from './dom.js';
import { api, onUnauthorized, resetCaches } from './api.js';
import { loading, errorState } from './ui.js';
import { landingView } from './views/landing.js';
import { loginView, signupView } from './views/auth.js';
import { dashboardView } from './views/dashboard.js';
import { complaintListView, complaintDetailView } from './views/complaints.js';
import { submitView } from './views/submit.js';
import { queueView } from './views/queue.js';
import { departmentsView, workersView, activityView } from './views/manage.js';
import { tasksView } from './views/tasks.js';
import { mapView } from './views/mapview.js';
import { profileView } from './views/profile.js';

export const store = { user: null };

const ALL = ['USER', 'ADMIN', 'DEPT_OFFICER', 'WORKER'];

const ROUTES = [
  { re: /^\/?$/, view: landingView, bare: true, title: 'Home' },
  { re: /^\/login$/, view: loginView, bare: true, title: 'Sign in' },
  { re: /^\/signup$/, view: signupView, bare: true, title: 'Create account' },
  { re: /^\/dashboard$/, view: dashboardView, roles: ALL, title: 'Overview' },
  { re: /^\/complaints$/, view: complaintListView, roles: ALL, title: 'Complaints' },
  { re: /^\/complaints\/(\d+)$/, view: complaintDetailView, roles: ALL, title: 'Complaint' },
  { re: /^\/submit$/, view: submitView, roles: ['USER'], title: 'Report a problem' },
  { re: /^\/queue$/, view: queueView, roles: ['ADMIN'], title: 'Review queue' },
  { re: /^\/map$/, view: mapView, roles: ALL, title: 'City map' },
  { re: /^\/departments$/, view: departmentsView, roles: ['ADMIN', 'DEPT_OFFICER'], title: 'Departments' },
  { re: /^\/workers$/, view: workersView, roles: ['ADMIN', 'DEPT_OFFICER'], title: 'Workers' },
  { re: /^\/activity$/, view: activityView, roles: ['ADMIN'], title: 'Activity log' },
  { re: /^\/tasks$/, view: tasksView, roles: ['WORKER'], title: 'My tasks' },
  { re: /^\/profile$/, view: profileView, roles: ALL, title: 'Profile' },
];

const ICONS = {
  overview: 'M3 3h6v8H3zM11 3h6v4h-6zM11 9h6v8h-6zM3 13h6v4H3z',
  report: 'M10 3v14M3 10h14',
  list: 'M7 5h10M7 10h10M7 15h10M3 5h.01M3 10h.01M3 15h.01',
  map: 'M3 5l4-2 6 2 4-2v12l-4 2-6-2-4 2zM7 3v12M13 5v12',
  user: 'M10 10a3.5 3.5 0 1 0 0-7 3.5 3.5 0 0 0 0 7zM3 17c1.2-3 4-4.5 7-4.5s5.8 1.5 7 4.5',
  queue: 'M4 4h12M4 8h12M4 12h8M4 16h5M15 12v5M13 15l2 2 2-2',
  dept: 'M3 17h14M4 17V8l6-4 6 4v9M8 17v-5h4v5',
  workers: 'M7 9a3 3 0 1 0 0-6 3 3 0 0 0 0 6zM2 17c.8-2.6 2.7-4 5-4s4.2 1.4 5 4M14 9a2.5 2.5 0 1 0 0-5M15 13c1.6.4 2.6 1.6 3 4',
  activity: 'M2 10h4l2-5 4 10 2-5h4',
  tasks: 'M4 10l3 3 6-7M4 16h12',
};

const NAV = {
  USER: [['#/dashboard', 'Overview', 'overview'], ['#/submit', 'Report a problem', 'report'], ['#/complaints', 'My complaints', 'list'], ['#/map', 'City map', 'map'], ['#/profile', 'Profile', 'user']],
  ADMIN: [['#/dashboard', 'Overview', 'overview'], ['#/queue', 'Review queue', 'queue'], ['#/complaints', 'All complaints', 'list'], ['#/departments', 'Departments', 'dept'], ['#/workers', 'Workers', 'workers'], ['#/map', 'City map', 'map'], ['#/activity', 'Activity log', 'activity'], ['#/profile', 'Profile', 'user']],
  DEPT_OFFICER: [['#/dashboard', 'Overview', 'overview'], ['#/complaints', 'Department complaints', 'list'], ['#/workers', 'Workers', 'workers'], ['#/departments', 'Departments', 'dept'], ['#/map', 'City map', 'map'], ['#/profile', 'Profile', 'user']],
  WORKER: [['#/tasks', 'My tasks', 'tasks'], ['#/dashboard', 'Overview', 'overview'], ['#/map', 'City map', 'map'], ['#/profile', 'Profile', 'user']],
};

function icon(name) {
  return s('svg', { viewBox: '0 0 20 20', class: 'icon', 'aria-hidden': 'true' },
    s('path', { d: ICONS[name] || '', fill: 'none', stroke: 'currentColor', 'stroke-width': '1.7', 'stroke-linecap': 'round', 'stroke-linejoin': 'round' }));
}

export function logo({ small = false } = {}) {
  return h('span', { class: `logo${small ? ' logo-sm' : ''}` },
    s('svg', { viewBox: '0 0 32 32', class: 'logo-mark', 'aria-hidden': 'true' },
      s('rect', { width: 32, height: 32, rx: 8, class: 'logo-bg' }),
      s('path', { d: 'M7 23 L14 13 L19 18 L25 9', fill: 'none', stroke: '#fff', 'stroke-width': 3, 'stroke-linecap': 'round', 'stroke-linejoin': 'round' }),
      s('circle', { cx: 25, cy: 9, r: 3.5, class: 'logo-dot' })),
    h('span', { class: 'logo-text' },
      h('span', { class: 'logo-name' }, 'Smart Complaint Analyzer'),
      h('span', { class: 'logo-city' }, 'Dehradun')));
}

export function defaultRoute(user) {
  if (!user) return '#/';
  return user.role === 'WORKER' ? '#/tasks' : '#/dashboard';
}

// Whether `user` may open the page at `hash` (a stale ?next= link from
// someone else's session is then ignored after signing in)
export function canOpen(hash, user) {
  const path = String(hash).replace(/^#/, '').split('?')[0];
  const route = ROUTES.find((r) => r.re.test(path));
  return !!route && (!route.roles || (!!user && route.roles.includes(user.role)));
}

export function navigate(hash) {
  if (location.hash === hash) render();
  else location.hash = hash;
}

function parseHash() {
  const raw = location.hash.replace(/^#/, '') || '/';
  const [path, query = ''] = raw.split('?');
  return { path: path || '/', query: new URLSearchParams(query) };
}

// ── Theme toggle (Auto → Light → Dark) ─────────────────────

function currentTheme() {
  return document.documentElement.getAttribute('data-theme') || 'auto';
}
function themeButton() {
  const labels = { auto: 'Theme: match system', light: 'Theme: light', dark: 'Theme: dark' };
  const btn = h('button', { type: 'button', class: 'theme-btn' });
  const paint = () => { btn.textContent = labels[currentTheme()]; };
  btn.addEventListener('click', () => {
    const next = { auto: 'light', light: 'dark', dark: 'auto' }[currentTheme()];
    if (next === 'auto') document.documentElement.removeAttribute('data-theme');
    else document.documentElement.setAttribute('data-theme', next);
    try { localStorage.setItem('spca-theme', next); } catch { /* storage unavailable */ }
    paint();
  });
  paint();
  return btn;
}

// ── Shell ──────────────────────────────────────────────────

let shell = null; // { root, main, navLinks, userKey }

function buildShell(user) {
  const appEl = document.getElementById('app');
  const main = h('main', { id: 'main', class: 'main', tabindex: '-1' });
  const navLinks = [];
  const nav = h('nav', { class: 'nav', 'aria-label': 'Main' },
    h('ul', null, (NAV[user.role] || NAV.USER).map(([href, label, ic]) => {
      const a = h('a', { href, class: 'nav-link' }, icon(ic), h('span', null, label));
      navLinks.push(a);
      return h('li', null, a);
    })));
  const signOut = h('button', { type: 'button', class: 'btn btn-ghost btn-block', onclick: logout }, 'Sign out');
  const menuBtn = h('button', { type: 'button', class: 'menu-btn', 'aria-expanded': 'false', 'aria-controls': 'sidebar', 'aria-label': 'Open menu' },
    h('span', { 'aria-hidden': 'true' }), h('span', { 'aria-hidden': 'true' }), h('span', { 'aria-hidden': 'true' }));
  const side = h('aside', { id: 'sidebar', class: 'sidebar' },
    h('a', { href: defaultRoute(user), class: 'sidebar-brand' }, logo()),
    h('div', { class: 'whoami' },
      h('span', { class: 'avatar', 'aria-hidden': 'true' }, initials(user.full_name || user.username)),
      h('span', { class: 'whoami-text' },
        h('strong', null, user.full_name || user.username),
        h('span', null, ROLE_LABEL[user.role] || user.role),
        user.role === 'DEPT_OFFICER' || user.role === 'WORKER' ? h('span', { class: 'whoami-dept' }, user.department) : null)),
    nav,
    h('div', { class: 'sidebar-foot' }, themeButton(), signOut));
  const root = h('div', { class: 'shell' },
    h('div', { class: 'topbar' }, menuBtn, h('a', { href: defaultRoute(user), class: 'topbar-brand' }, logo({ small: true }))),
    side,
    h('div', { class: 'scrim', onclick: () => setNav(false) }),
    main);
  const setNav = (open) => {
    root.classList.toggle('nav-open', open);
    menuBtn.setAttribute('aria-expanded', String(open));
    menuBtn.setAttribute('aria-label', open ? 'Close menu' : 'Open menu');
  };
  menuBtn.addEventListener('click', () => setNav(!root.classList.contains('nav-open')));
  side.addEventListener('click', (e) => { if (e.target.closest('a')) setNav(false); });
  mount(appEl, root);
  shell = { root, main, navLinks, userKey: `${user.id}:${user.role}` };
}

function initials(name) {
  return String(name).split(/\s+/).filter(Boolean).slice(0, 2).map((w) => w[0].toUpperCase()).join('');
}

async function logout() {
  try { await api('/logout', { method: 'POST' }); } catch { /* already signed out */ }
  store.user = null;
  shell = null;
  resetCaches();
  toast('Signed out.', 'success');
  navigate('#/');
}

export function setUser(user) {
  store.user = user;
  if (!user) shell = null;
  resetCaches();
}

// ── Router ─────────────────────────────────────────────────

let renderSeq = 0;

export async function render() {
  const seq = ++renderSeq;
  const { path, query } = parseHash();
  const route = ROUTES.find((r) => r.re.test(path));
  const appEl = document.getElementById('app');
  appEl.removeAttribute('aria-busy');

  if (!route) {
    navigate(store.user ? defaultRoute(store.user) : '#/');
    return;
  }
  const params = path.match(route.re).slice(1);

  if (route.roles) {
    if (!store.user) {
      navigate(`#/login?next=${encodeURIComponent(location.hash)}`);
      return;
    }
    if (!route.roles.includes(store.user.role)) {
      toast('That page is not available for your role.', 'error');
      navigate(defaultRoute(store.user));
      return;
    }
  }

  document.title = `${route.title} – Smart Complaint Analyzer`;
  const ctx = {
    params,
    query,
    user: store.user,
    alive: () => seq === renderSeq,
    navigate,
    setTitle: (t) => { document.title = `${t} – Smart Complaint Analyzer`; },
    setQuery: (obj) => {
      const q = new URLSearchParams();
      for (const [k, v] of Object.entries(obj)) if (v !== '' && v !== null && v !== undefined) q.set(k, v);
      const str = q.toString();
      navigate(`#${path}${str ? `?${str}` : ''}`);
    },
    refreshUser: async () => {
      const me = await api('/me');
      store.user = me.user;
      ctx.user = me.user;
      return me.user;
    },
  };

  let target;
  if (route.bare) {
    shell = null;
    target = h('div', { class: 'bare' });
    mount(appEl, target);
  } else {
    const key = `${store.user.id}:${store.user.role}`;
    if (!shell || shell.userKey !== key || !document.body.contains(shell.root)) buildShell(store.user);
    for (const a of shell.navLinks) {
      const href = a.getAttribute('href').slice(1);
      const active = path === href || (href !== '/dashboard' && path.startsWith(`${href}/`));
      a.classList.toggle('active', active);
      if (active) a.setAttribute('aria-current', 'page'); else a.removeAttribute('aria-current');
    }
    target = shell.main;
    mount(target, loading());
  }
  ctx.root = target;

  try {
    await route.view(ctx);
  } catch (err) {
    if (seq !== renderSeq) return;
    if (err && err.status === 401) return;
    console.error(err);
    mount(target, errorState(err, () => render()));
  }
  if (seq === renderSeq && !route.bare) {
    window.scrollTo(0, 0);
    target.focus({ preventScroll: true });
  } else if (seq === renderSeq) {
    window.scrollTo(0, 0);
  }
}

onUnauthorized(() => {
  const wasIn = !!store.user;
  store.user = null;
  shell = null;
  const { path } = parseHash();
  const route = ROUTES.find((r) => r.re.test(path));
  if (route && route.roles) {
    if (wasIn) toast('Your session ended. Sign in again.', 'error');
    navigate(`#/login?next=${encodeURIComponent(location.hash)}`);
  }
});

async function boot() {
  // The skip link must not change the hash (the router owns it).
  const skip = document.querySelector('.skip-link');
  if (skip) {
    skip.addEventListener('click', (e) => {
      e.preventDefault();
      const main = document.getElementById('main');
      if (main) main.focus();
    });
  }
  try {
    const me = await api('/me', { quiet401: true });
    store.user = me && me.user ? me.user : null;
  } catch {
    store.user = null;
  }
  window.addEventListener('hashchange', render);
  render();
}

boot();
