import { h, mount, toast, fmtNum } from '../dom.js';
import { api, qs, getGraph } from '../api.js';
import { pageHeader, panel, locationSelect, withBusy, field } from '../ui.js';
import { createCityMap, routeSteps } from '../map.js';

export async function mapView(ctx) {
  const [graph, stats] = await Promise.all([getGraph(), api('/stats').catch(() => null)]);
  if (!ctx.alive()) return;

  const counts = {};
  for (const l of (stats && stats.by_location) || []) counts[l.id] = l.open ?? l.total;
  const label = {
    USER: 'your open complaints',
    WORKER: 'your open tasks',
    DEPT_OFFICER: 'open department complaints',
    ADMIN: 'open complaints',
  }[ctx.user.role];

  const start = ctx.user.role === 'WORKER' ? ctx.user.location_id : '';
  const from = locationSelect(graph, start, { required: true, placeholder: 'Starting point' });
  const to = locationSelect(graph, '', { required: true, placeholder: 'Destination' });
  const go = h('button', { type: 'submit', class: 'btn btn-primary' }, 'Find shortest route');
  const result = h('div', { class: 'route-slot', 'aria-live': 'polite' },
    h('p', { class: 'muted small' }, 'Pick two places, or select them on the map: the first point you select is the start, the second the destination.'));

  let picking = 'from';
  const map = createCityMap(graph, {
    counts: stats ? counts : null,
    countLabel: label,
    onNodeClick: (id) => {
      if (picking === 'from') { from.value = String(id); picking = 'to'; } else { to.value = String(id); picking = 'from'; }
      map.setSelected([from.value, to.value].filter((v) => v !== '').map(Number));
      if (from.value !== '' && to.value !== '') find();
    },
  });
  if (start !== '' && start !== undefined && start !== null) { map.setSelected([Number(start)]); picking = 'to'; }

  const find = () => withBusy(go, 'Finding…', async () => {
    if (from.value === '' || to.value === '') { toast('Choose both a start and a destination.', 'error'); return; }
    try {
      const r = await api(`/route${qs({ from: from.value, to: to.value })}`);
      map.setRoute(r);
      map.setSelected([Number(from.value), Number(to.value)]);
      mount(result, routeSteps(r, graph));
    } catch (err) { toast(err.message, 'error'); }
  });

  const form = h('form', { class: 'route-form', onsubmit: (e) => { e.preventDefault(); find(); } },
    field('From', from), field('To', to), go,
    h('button', { type: 'button', class: 'btn btn-ghost', onclick: () => { map.setRoute(null); map.setSelected([]); from.value = ''; to.value = ''; picking = 'from'; mount(result, h('p', { class: 'muted small' }, 'Route cleared.')); } }, 'Clear'));

  const total = Object.values(counts).reduce((a, b) => a + Number(b || 0), 0);
  mount(ctx.root,
    pageHeader('City map', `The 18 locations and roads the engine routes over. Circle size shows ${label}${stats ? ` (${fmtNum(total)} in all)` : ''}.`),
    h('div', { class: 'map-page' },
      panel(null, map.el, { className: 'map-panel' }),
      h('aside', null, panel('Shortest route', [form, result], { sub: 'Dijkstra’s algorithm in the C engine, over road distances in km.' }))));
}
