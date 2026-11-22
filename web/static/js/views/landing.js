import { h, mount, fmtNum, fmtDec, STATUSES, statusLabel } from '../dom.js';
import { api, getGraph } from '../api.js';
import { logo, store, defaultRoute } from '../app.js';
import { createCityMap } from '../map.js';
import { rankedBars, segmentedBar } from '../charts.js';
import { loading } from '../ui.js';

export const STATUS_RAMP = {
  PENDING: { color: 'var(--st-1)', ink: 'var(--st-ink-1)' },
  REVIEWED: { color: 'var(--st-2)', ink: 'var(--st-ink-2)' },
  ASSIGNED: { color: 'var(--st-3)', ink: 'var(--st-ink-3)' },
  IN_PROGRESS: { color: 'var(--st-4)', ink: 'var(--st-ink-4)' },
  COMPLETED: { color: 'var(--st-5)', ink: 'var(--st-ink-5)' },
};

export function statusParts(totals) {
  const key = { PENDING: 'pending', REVIEWED: 'reviewed', ASSIGNED: 'assigned', IN_PROGRESS: 'in_progress', COMPLETED: 'completed' };
  return STATUSES.map((st) => ({ name: statusLabel(st), value: Number(totals[key[st]] || 0), ...STATUS_RAMP[st] }));
}

export async function landingView(ctx) {
  const user = store.user;
  const cta = user
    ? h('a', { class: 'btn btn-signal btn-lg', href: user.role === 'USER' ? '#/submit' : defaultRoute(user) }, user.role === 'USER' ? 'Report a problem' : 'Open your desk')
    : h('a', { class: 'btn btn-signal btn-lg', href: '#/signup' }, 'Report a problem');
  const second = user
    ? h('a', { class: 'btn btn-ghost btn-lg', href: defaultRoute(user) }, `Continue as ${user.full_name || user.username}`)
    : h('a', { class: 'btn btn-ghost btn-lg', href: '#/login' }, 'Sign in');

  const facts = h('div', { class: 'hero-facts' }, loading('Counting complaints…'));
  const mapSlot = h('div', { class: 'hero-map' }, loading('Drawing the city map…'));
  const insights = h('div', { class: 'landing-insights' });

  const page = h('div', { class: 'landing' },
    h('header', { class: 'landing-bar' },
      h('a', { href: '#/', class: 'landing-brand' }, logo()),
      h('nav', { class: 'landing-nav', 'aria-label': 'Account' },
        user
          ? h('a', { href: defaultRoute(user), class: 'btn btn-secondary' }, 'Open your desk')
          : [h('a', { href: '#/login', class: 'btn btn-ghost' }, 'Sign in'), h('a', { href: '#/signup', class: 'btn btn-secondary' }, 'Create account')])),
    h('main', { id: 'main', class: 'landing-main', tabindex: '-1' },
      h('section', { class: 'hero' },
        h('div', { class: 'hero-copy' },
          h('h1', { class: 'hero-title' }, 'Report a civic problem. Follow it until it is fixed.'),
          h('p', { class: 'hero-lede' },
            'Water cuts, dead streetlights, potholes, overflowing garbage, unsafe corners. Describe the problem in your own words: the analyzer estimates how urgent it is, checks whether a neighbour already reported it, and sends it to the department that handles it.'),
          h('div', { class: 'hero-cta' }, cta, second),
          facts),
        mapSlot),
      h('section', { class: 'landing-section', 'aria-labelledby': 'flow-title' },
        h('h2', { id: 'flow-title', class: 'section-title' }, 'From report to repair'),
        h('ol', { class: 'flow' },
          flowStep('Submitted', 'You describe the problem and pick the location. The text is scored for severity and checked against earlier reports.'),
          flowStep('Reviewed', 'Administrators work through pending complaints in priority order, most severe first.'),
          flowStep('Assigned', 'The complaint goes to its department, and an officer assigns a field worker.'),
          flowStep('In progress', 'The worker gets the shortest route across the city’s road graph.'),
          flowStep('Completed', 'The worker closes the task and the completion date appears on your complaint.'))),
      insights,
      h('section', { class: 'landing-section engine', 'aria-labelledby': 'engine-title' },
        h('h2', { id: 'engine-title', class: 'section-title' }, 'How the analysis works'),
        h('dl', { class: 'engine-list' },
          h('div', null, h('dt', null, 'Severity estimate'), h('dd', null, 'Each word is looked up in a dictionary of weighted civic terms. Negations such as “not broken” soften a term, amplifiers such as “extremely” strengthen it, and the result is a score from 1 to 10.')),
          h('div', null, h('dt', null, 'Duplicate check'), h('dd', null, 'Word overlap (Jaccard) and edit distance (Levenshtein) are combined with category and location. Reports from different people at different places need a closer match before they are linked.')),
          h('div', null, h('dt', null, 'Priority queue'), h('dd', null, 'A max-heap keeps pending complaints ordered by severity, so the most serious ones reach staff first.')),
          h('div', null, h('dt', null, 'Routing'), h('dd', null, 'Dijkstra’s algorithm finds the shortest path for field workers over an 18-location road graph of Dehradun.'))),
        h('p', { class: 'engine-note' }, 'These are decision aids built on rules, not a trained model. Staff review each result before acting on it.'))),
    h('footer', { class: 'landing-foot' },
      h('p', null, 'Smart Public Complaint Analyzer. A C engine with a C++ console and this web interface, sharing one data store.')));

  mount(ctx.root, page);

  const [summary, graph] = await Promise.allSettled([api('/public/summary'), getGraph()]);
  if (!ctx.alive()) return;

  if (summary.status === 'fulfilled') {
    const sm = summary.value || {};
    const t = sm.totals || {};
    const res = sm.resolution || {};
    mount(facts,
      fact(fmtNum(t.open ?? (t.total - t.completed)), 'open complaints'),
      fact(fmtNum(t.completed), 'fixed so far'),
      fact(res.median_days !== undefined && res.median_days !== null ? `${fmtDec(res.median_days, 0)} days` : '–', 'median time to fix'));

    const cats = (sm.by_category || []).filter((c) => c.name).slice().sort((a, b) => b.total - a.total);
    mount(insights,
      h('section', { class: 'landing-section split', 'aria-labelledby': 'what-title' },
        h('div', null,
          h('h2', { id: 'what-title', class: 'section-title' }, 'What residents report'),
          h('p', { class: 'section-lede' }, `${fmtNum(t.total)} complaints on record, by category.`),
          rankedBars(cats.map((c) => ({ label: c.name, value: c.total, open: c.open, completed: c.completed })), {
            detail: (r) => [{ value: fmtNum(r.open), label: 'still open' }, { value: fmtNum(r.completed), label: 'completed' }],
          })),
        h('div', null,
          h('h2', { class: 'section-title' }, 'Where they stand'),
          h('p', { class: 'section-lede' }, 'Every complaint by its current step.'),
          segmentedBar(statusParts(t), { ariaLabel: 'Complaints by status' }))));
  } else {
    mount(facts, h('p', { class: 'muted' }, summary.reason.message));
  }

  if (graph.status === 'fulfilled') {
    const counts = {};
    if (summary.status === 'fulfilled') {
      for (const l of summary.value.by_location || []) counts[l.id] = l.open ?? l.total;
    }
    const map = createCityMap(graph.value, { counts: summary.status === 'fulfilled' ? counts : null, countLabel: 'open complaints' });
    mount(mapSlot, map.el);
  } else {
    mount(mapSlot, h('p', { class: 'muted' }, graph.reason.message));
  }
}

function fact(value, label) {
  return h('div', { class: 'fact' }, h('span', { class: 'fact-value' }, value), h('span', { class: 'fact-label' }, label));
}

function flowStep(title, body) {
  return h('li', { class: 'flow-step' }, h('h3', null, title), h('p', null, body));
}
