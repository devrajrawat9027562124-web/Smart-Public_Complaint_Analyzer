import { h, mount, fmtNum, fmtDec, fmtPct, fmtMonth, ROLE_LABEL } from '../dom.js';
import { api, getGraph, qs } from '../api.js';
import { pageHeader, panel, complaintTable, emptyState, sevBadge, statusChip } from '../ui.js';
import { statTile, segmentedBar, rankedBars, columnChart, lineChart, emptyChart } from '../charts.js';
import { createCityMap } from '../map.js';
import { statusParts } from './landing.js';

export async function dashboardView(ctx) {
  const role = ctx.user.role;
  if (role === 'USER') return citizenDashboard(ctx);
  if (role === 'WORKER') return workerDashboard(ctx);
  return staffDashboard(ctx);
}

function openOf(t) {
  if (t.open !== undefined && t.open !== null) return t.open;
  return (t.total || 0) - (t.completed || 0);
}

function kpis(stats, { flags = true } = {}) {
  const t = stats.totals || {};
  const res = stats.resolution || {};
  return h('div', { class: flags ? 'kpis kpis-6' : 'kpis' },
    statTile({ label: 'Complaints on record', value: fmtNum(t.total) }),
    statTile({ label: 'Open', value: fmtNum(openOf(t)), sub: `${fmtNum(t.pending)} waiting for review` }),
    statTile({ label: 'Completed', value: fmtNum(t.completed), sub: t.total ? `${fmtPct(t.completed / t.total)} of all complaints` : null }),
    statTile({ label: 'Median time to fix', value: res.median_days !== null && res.median_days !== undefined ? `${fmtDec(res.median_days, 1)} days` : '–', sub: res.avg_days !== null && res.avg_days !== undefined ? `Average ${fmtDec(res.avg_days, 1)} days over ${fmtNum(res.count)} fixes` : null }),
    flags ? statTile({ label: 'Flagged as duplicate', value: fmtNum(t.duplicates), sub: 'By the duplicate detector' }) : null,
    flags ? statTile({ label: 'Possible spam', value: fmtNum(t.irrelevant), sub: 'Kept on record for review' }) : null);
}

function monthlyChart(stats) {
  const monthly = stats.monthly || [];
  if (!monthly.length) return emptyChart('No dated complaints yet.');
  // The current month is still running; label it so its lower counts are
  // not read as a drop
  const partial = new Set(monthly.filter((m) => m.partial).map((m) => m.month));
  return lineChart(
    [
      { name: 'Submitted', short: 'submitted', color: 'var(--series-1)', values: monthly.map((m) => m.submitted || 0) },
      { name: 'Completed', short: 'completed', color: 'var(--series-2)', values: monthly.map((m) => m.completed || 0) },
    ],
    monthly.map((m) => m.month),
    { labelFormat: (k) => (partial.has(k) ? `${fmtMonth(k)} (so far)` : fmtMonth(k)) },
  );
}

function severityChart(stats) {
  const hist = stats.severity_histogram || [];
  return columnChart(hist.map((b) => ({ label: String(b.severity), title: `Severity ${b.severity}`, value: b.count || 0 })), { xTitle: 'Severity (1 = minor, 10 = critical)' });
}

function categoryBars(stats) {
  const cats = (stats.by_category || []).slice().sort((a, b) => b.total - a.total);
  return rankedBars(cats.map((c) => ({ label: c.name, value: c.total, open: c.open, completed: c.completed, avg: c.avg_severity })), {
    detail: (r) => [
      { value: fmtNum(r.open), label: 'open' },
      { value: fmtNum(r.completed), label: 'completed' },
      { value: fmtDec(r.avg, 1), label: 'average severity' },
    ],
    onSelect: (r) => { location.hash = `#/complaints${qs({ category: r.label })}`; },
  });
}

function nlpBlock(stats) {
  const n = stats.nlp_agreement;
  if (!n || !n.n) return null;
  return panel('NLP score against stored severity', [
    h('div', { class: 'kpis kpis-tight' },
      statTile({ label: 'Exact match', value: fmtPct(n.exact) }),
      statTile({ label: 'Within one point', value: fmtPct(n.within_1) }),
      statTile({ label: 'Mean absolute difference', value: fmtDec(n.mae, 2), sub: 'points on the 1–10 scale' }),
      statTile({ label: 'Complaints compared', value: fmtNum(n.n) })),
    h('p', { class: 'caption' },
      'Compares the rule-based engine’s nlp_score with the severity stored for each complaint, which the dataset or an administrator set. It is a consistency check of the keyword rules, not a validated accuracy figure: the stored severities are not independent expert labels.'),
  ]);
}

async function staffDashboard(ctx) {
  const isAdmin = ctx.user.role === 'ADMIN';
  const [stats, graph] = await Promise.all([api('/stats'), getGraph()]);
  if (!ctx.alive()) return;
  const t = stats.totals || {};
  const scopeName = isAdmin ? 'Dehradun' : ctx.user.department;

  const counts = {};
  for (const l of stats.by_location || []) counts[l.id] = l.open ?? l.total;
  const map = createCityMap(graph, { counts, countLabel: 'open complaints', caption: true });
  const topLocations = (stats.by_location || []).slice().sort((a, b) => (b.open ?? b.total) - (a.open ?? a.total)).slice(0, 8);

  mount(ctx.root,
    pageHeader(isAdmin ? 'City overview' : `${scopeName} overview`,
      isAdmin ? 'Live figures from the shared complaint records.' : `Complaints routed to the ${scopeName} department.`,
      isAdmin
        ? h('a', { href: '#/queue', class: 'btn btn-primary' }, `Review queue (${fmtNum(t.pending)} pending)`)
        : h('a', { href: '#/complaints', class: 'btn btn-primary' }, 'Assign workers')),
    kpis(stats),
    h('div', { class: 'grid-2' },
      panel('Where complaints stand', segmentedBar(statusParts(t), { ariaLabel: 'Complaints by status' }), { sub: 'Each complaint by its current workflow step.' }),
      panel('Submitted and completed by month', monthlyChart(stats))),
    h('div', { class: 'grid-2' },
      panel('By category', categoryBars(stats), { sub: isAdmin ? 'Select a category to see its complaints.' : null }),
      panel('Severity spread', severityChart(stats), { sub: 'How many complaints sit at each severity score.' })),
    panel('Open complaints across the city', h('div', { class: 'map-split' },
      map.el,
      h('div', null,
        h('h3', { class: 'subhead' }, 'Most open complaints'),
        rankedBars(topLocations.map((l) => ({ label: l.name, value: l.open ?? l.total, total: l.total, avg: l.avg_severity, id: l.id })), {
          unit: 'open complaints',
          detail: (r) => [{ value: fmtNum(r.total), label: 'total' }, { value: fmtDec(r.avg, 1), label: 'average severity' }],
          onSelect: (r) => { location.hash = `#/complaints${qs({ location_id: r.id })}`; },
        })))),
    nlpBlock(stats));
}

async function citizenDashboard(ctx) {
  const [stats, recent] = await Promise.all([
    api('/stats').catch(() => null),
    api(`/complaints${qs({ page_size: 5, sort: 'submitted_desc' })}`),
  ]);
  if (!ctx.alive()) return;
  const first = (ctx.user.full_name || ctx.user.username).split(' ')[0];
  const t = stats ? stats.totals || {} : {};
  const has = recent.total > 0;

  mount(ctx.root,
    pageHeader(`Hello, ${first}`, 'Track the problems you reported, or report a new one.',
      h('a', { href: '#/submit', class: 'btn btn-signal' }, 'Report a problem')),
    stats ? h('div', { class: 'kpis' },
      statTile({ label: 'Your complaints', value: fmtNum(t.total) }),
      statTile({ label: 'Open', value: fmtNum(openOf(t)) }),
      statTile({ label: 'Completed', value: fmtNum(t.completed) })) : null,
    has && stats ? panel('Where your complaints stand', segmentedBar(statusParts(t), { ariaLabel: 'Your complaints by status' })) : null,
    panel('Recent complaints', has
      ? [complaintTable(recent.items), h('p', { class: 'panel-more' }, h('a', { href: '#/complaints' }, `See all ${fmtNum(recent.total)}`))]
      : emptyState('You have not reported anything yet', 'Describe a problem and choose where it is. The analyzer suggests a category and severity before you submit.',
        h('a', { href: '#/submit', class: 'btn btn-primary' }, 'Report a problem'))));
}

async function workerDashboard(ctx) {
  const [stats, list] = await Promise.all([
    api('/stats').catch(() => null),
    api(`/complaints${qs({ page_size: 200, sort: 'severity_desc' })}`),
  ]);
  if (!ctx.alive()) return;
  const active = list.items.filter((c) => c.status !== 'COMPLETED');
  const next = active[0];
  const t = stats ? stats.totals || {} : {};
  const res = stats ? stats.resolution || {} : {};
  mount(ctx.root,
    pageHeader('Your work', `${ROLE_LABEL.WORKER}, ${ctx.user.department}. Currently at ${ctx.user.location}.`,
      h('a', { href: '#/tasks', class: 'btn btn-primary' }, 'Open my tasks')),
    h('div', { class: 'kpis' },
      statTile({ label: 'Active tasks', value: fmtNum(active.length) }),
      statTile({ label: 'Completed', value: fmtNum(t.completed ?? list.items.length - active.length) }),
      statTile({ label: 'Median time to fix', value: res.median_days !== undefined && res.median_days !== null ? `${fmtDec(res.median_days, 1)} days` : '–' })),
    panel('Most severe active task', next
      ? h('a', { href: `#/complaints/${next.id}`, class: 'next-task' },
        sevBadge(next.severity),
        h('span', { class: 'next-task-text' }, h('strong', null, next.title), h('span', null, `${next.location}, complaint #${next.id}`)),
        statusChip(next.status))
      : emptyState('No active tasks', 'New tasks appear here when your department officer assigns them.')),
    stats ? panel('Your tasks by category', categoryBars(stats)) : null);
}
