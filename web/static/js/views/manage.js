import { h, mount, fmtNum, fmtPct, fmtDate, CATEGORIES } from '../dom.js';
import { api, qs, getGraph, getDepartments } from '../api.js';
import { pageHeader, panel, emptyState, loading } from '../ui.js';
import { createCityMap } from '../map.js';

export async function departmentsView(ctx) {
  const depts = await getDepartments(true);
  if (!ctx.alive()) return;
  const mine = ctx.user.role === 'DEPT_OFFICER' ? ctx.user.department : null;
  mount(ctx.root,
    pageHeader('Departments', 'Live counts from the complaint records. Each department has an officer who assigns its field workers.'),
    depts.length
      ? panel(null, h('div', { class: 'table-scroll table-cards' }, h('table', { class: 'table' },
        h('thead', null, h('tr', null,
          h('th', { scope: 'col' }, 'Department'),
          h('th', { scope: 'col' }, 'Officer'),
          h('th', { scope: 'col', class: 'num' }, 'Total'),
          h('th', { scope: 'col', class: 'num' }, 'Open'),
          h('th', { scope: 'col', class: 'num' }, 'Completed'),
          h('th', { scope: 'col' }, 'Share completed'),
          h('th', { scope: 'col', class: 'num' }, 'Workers'))),
        h('tbody', null, depts.map((d) => {
          const share = d.total ? d.completed / d.total : 0;
          return h('tr', { class: d.name === mine ? 'row-highlight' : null },
            h('td', { 'data-label': 'Department' }, h('strong', null, d.name), d.name === mine ? h('span', { class: 'muted' }, ' (yours)') : null),
            h('td', { 'data-label': 'Officer' }, d.officer_name || d.officer_username || '–', d.officer_name && d.officer_username ? h('span', { class: 'cell-meta' }, d.officer_username) : null),
            h('td', { class: 'num', 'data-label': 'Total' }, fmtNum(d.total)),
            h('td', { class: 'num', 'data-label': 'Open' }, fmtNum(d.open)),
            h('td', { class: 'num', 'data-label': 'Completed' }, fmtNum(d.completed)),
            h('td', { 'data-label': 'Share completed' }, h('span', { class: 'progress', role: 'img', 'aria-label': `${fmtPct(share)} completed` },
              h('span', { class: 'progress-track' }, h('span', { class: 'progress-fill', style: { width: `${share * 100}%` } })),
              h('span', { class: 'progress-text' }, fmtPct(share)))),
            h('td', { class: 'num', 'data-label': 'Workers' }, fmtNum(d.workers)));
        })))))
      : emptyState('No departments found', 'The departments file is empty.'));
}

export async function workersView(ctx) {
  const isAdmin = ctx.user.role === 'ADMIN';
  const dept = isAdmin ? (ctx.query.get('department') || '') : ctx.user.department;
  const [workers, graph] = await Promise.all([api(`/workers${qs({ department: dept })}`), getGraph()]);
  if (!ctx.alive()) return;

  const filter = isAdmin
    ? h('label', { class: 'filter filter-inline' }, h('span', { class: 'filter-label' }, 'Department'),
      (() => {
        const sel = h('select', { class: 'input', onchange: () => ctx.setQuery({ department: sel.value }) },
          h('option', { value: '' }, 'All departments'),
          CATEGORIES.map((c) => h('option', { value: c, selected: c === dept }, c)));
        return sel;
      })())
    : null;

  const counts = {};
  for (const w of workers) counts[w.location_id] = (counts[w.location_id] || 0) + 1;
  const map = createCityMap(graph, { counts, countLabel: 'workers here' });

  mount(ctx.root,
    pageHeader('Field workers', isAdmin ? 'Every field worker, with where they last reported being and their current load.' : `Field workers in ${dept}.`, filter),
    workers.length
      ? h('div', { class: 'grid-2 grid-2-wide' },
        panel(null, h('div', { class: 'table-scroll table-cards' }, h('table', { class: 'table' },
          h('thead', null, h('tr', null,
            h('th', { scope: 'col' }, 'Worker'),
            isAdmin ? h('th', { scope: 'col' }, 'Department') : null,
            h('th', { scope: 'col' }, 'Location'),
            h('th', { scope: 'col' }, 'Availability'),
            h('th', { scope: 'col', class: 'num' }, 'Active'),
            h('th', { scope: 'col', class: 'num' }, 'Completed'))),
          h('tbody', null, workers.map((w) => h('tr', null,
            h('td', { 'data-label': 'Worker' }, h('strong', null, w.full_name || w.username), h('span', { class: 'cell-meta' }, w.username)),
            isAdmin ? h('td', { 'data-label': 'Department' }, w.department) : null,
            h('td', { 'data-label': 'Location' }, w.location),
            h('td', { 'data-label': 'Availability' }, h('span', { class: `avail ${w.is_available ? 'avail-yes' : 'avail-no'}` },
              h('span', { 'aria-hidden': 'true' }, w.is_available ? '●' : '○'), w.is_available ? 'Available' : 'Busy')),
            h('td', { class: 'num', 'data-label': 'Active' }, fmtNum(w.active_tasks)),
            h('td', { class: 'num', 'data-label': 'Completed' }, fmtNum(w.completed_tasks)))))))),
        panel('Where they are', map.el))
      : emptyState('No workers found', dept ? `No field workers are registered in ${dept}.` : 'No field workers are registered.'));
}

export async function activityView(ctx) {
  const limit = [50, 100, 250, 500].includes(Number(ctx.query.get('limit'))) ? Number(ctx.query.get('limit')) : 100;
  const slot = h('div', null, loading('Reading the activity log…'));
  const limitSel = h('select', { class: 'input input-sm', 'aria-label': 'How many entries', onchange: () => ctx.setQuery({ limit: limitSel.value }) },
    [50, 100, 250, 500].map((n) => h('option', { value: String(n), selected: n === limit }, `Last ${n}`)));
  mount(ctx.root, pageHeader('Activity log', 'Sign-ins and every change to a complaint, from the console app and this website. Newest first.', limitSel), slot);

  const logs = await api(`/logs${qs({ limit })}`);
  if (!ctx.alive()) return;
  if (!logs.length) { mount(slot, emptyState('No activity yet', 'Actions appear here as people use the system.')); return; }

  const search = h('input', { class: 'input', type: 'search', placeholder: 'Filter by name, action or #id', 'aria-label': 'Filter the activity log' });
  const list = h('ol', { class: 'log' });
  const count = h('p', { class: 'muted small', 'aria-live': 'polite' });
  const paint = () => {
    const term = search.value.trim().toLowerCase();
    const rows = term ? logs.filter((l) => `${l.time} ${l.message}`.toLowerCase().includes(term)) : logs;
    mount(list, rows.map((l) => h('li', { class: `log-row ${logKind(l.message)}` },
      h('time', null, fmtDate(l.time)),
      h('span', null, linkify(l.message)))));
    count.textContent = term ? `${fmtNum(rows.length)} of ${fmtNum(logs.length)} entries match.` : `${fmtNum(logs.length)} entries.`;
  };
  search.addEventListener('input', paint);
  paint();
  mount(slot, panel(null, [h('div', { class: 'log-tools' }, search, count), list]));
}

function logKind(msg) {
  if (/logged (in|out)/i.test(msg)) return 'log-session';
  if (/completed/i.test(msg)) return 'log-done';
  if (/submitted/i.test(msg)) return 'log-new';
  return 'log-change';
}

// Turn "#123" into links without using innerHTML.
function linkify(msg) {
  const parts = [];
  const re = /#(\d+)/g;
  let last = 0;
  let m;
  while ((m = re.exec(msg))) {
    if (m.index > last) parts.push(msg.slice(last, m.index));
    parts.push(h('a', { href: `#/complaints/${m[1]}` }, m[0]));
    last = m.index + m[0].length;
  }
  if (last < msg.length) parts.push(msg.slice(last));
  return parts;
}
