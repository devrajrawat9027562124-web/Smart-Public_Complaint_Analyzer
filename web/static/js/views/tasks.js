import { h, mount, toast, confirmDialog, fmtNum, fmtDate, ago } from '../dom.js';
import { api, qs, getGraph } from '../api.js';
import { pageHeader, panel, emptyState, sevBadge, statusChip, withBusy } from '../ui.js';
import { createCityMap, routeSteps } from '../map.js';

export async function tasksView(ctx) {
  const graph = await getGraph();
  if (!ctx.alive()) return;

  const load = async () => {
    const user = await ctx.refreshUser().catch(() => ctx.user);
    const data = await api(`/complaints${qs({ page_size: 200, sort: 'severity_desc' })}`);
    if (!ctx.alive()) return;
    const active = data.items.filter((c) => c.status !== 'COMPLETED')
      .sort((a, b) => (b.status === 'IN_PROGRESS') - (a.status === 'IN_PROGRESS') || b.severity - a.severity);
    const done = data.items.filter((c) => c.status === 'COMPLETED')
      .sort((a, b) => String(b.completed_date || '').localeCompare(String(a.completed_date || '')));

    const counts = {};
    for (const c of active) counts[c.location_id] = (counts[c.location_id] || 0) + 1;
    const map = createCityMap(graph, { counts, countLabel: 'active tasks', selected: [user.location_id] });
    const routeSlot = h('div', { class: 'route-slot' },
      h('p', { class: 'muted small' }, `The ringed point is your current location, ${user.location}. Choose “Show route” on a task to plot the shortest path.`));

    const showRoute = async (c, btn) => withBusy(btn, 'Routing…', async () => {
      try {
        const r = await api(`/route${qs({ from: user.location_id, to: c.location_id })}`);
        map.setRoute(r);
        mount(routeSlot, h('h3', { class: 'subhead' }, `To #${c.id} at ${c.location}`), routeSteps(r, graph));
        map.el.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
      } catch (err) { toast(err.message, 'error'); }
    });

    const card = (c) => {
      const routeBtn = h('button', { type: 'button', class: 'btn btn-ghost btn-sm' }, 'Show route');
      routeBtn.addEventListener('click', () => showRoute(c, routeBtn));
      const buttons = [routeBtn];
      if (c.status !== 'IN_PROGRESS') {
        const start = h('button', { type: 'button', class: 'btn btn-secondary btn-sm' }, 'Start task');
        start.addEventListener('click', () => withBusy(start, 'Starting…', async () => {
          try {
            const res = await api(`/complaints/${c.id}/start`, { method: 'POST' });
            toast(`Task #${c.id} started.`, 'success');
            await load();
            if (res.route) toast(`Route: ${res.route.distance} km. Open the task to see it on the map.`, 'info');
          } catch (err) { toast(err.message, 'error'); }
        }));
        buttons.push(start);
      }
      // Only a started task can be completed (ASSIGNED -> IN_PROGRESS -> COMPLETED)
      const complete = c.status !== 'IN_PROGRESS' ? null : h('button', { type: 'button', class: 'btn btn-primary btn-sm' }, 'Mark as completed');
      if (complete) complete.addEventListener('click', async () => {
        const ok = await confirmDialog({ title: `Mark #${c.id} as completed?`, body: `${c.title}, at ${c.location}. The citizen will see it as fixed today.`, confirmLabel: 'Mark as completed' });
        if (!ok) return;
        await withBusy(complete, 'Saving…', async () => {
          try {
            await api(`/complaints/${c.id}/complete`, { method: 'POST' });
            toast(`Task #${c.id} completed.`, 'success');
            await load();
          } catch (err) { toast(err.message, 'error'); }
        });
      });
      if (complete) buttons.push(complete);
      return h('li', { class: `task${c.status === 'IN_PROGRESS' ? ' task-live' : ''}` },
        h('div', { class: 'task-top' }, sevBadge(c.severity), statusChip(c.status)),
        h('a', { href: `#/complaints/${c.id}`, class: 'task-title' }, c.title),
        h('p', { class: 'task-meta' }, `#${c.id} at ${c.location}, reported ${ago(c.submitted_date)}`),
        h('div', { class: 'task-actions' }, buttons));
    };

    // Available / busy is the worker's own switch: officers can only assign
    // new tasks to available workers
    const availBtn = h('button', { type: 'button', class: 'btn btn-secondary' },
      user.is_available ? 'Set me as busy' : 'Set me as available');
    availBtn.addEventListener('click', () => withBusy(availBtn, 'Saving…', async () => {
      try {
        const res = await api('/me/availability', { method: 'POST', body: { available: !user.is_available } });
        toast(res.message, 'success');
        await load();
      } catch (err) { toast(err.message, 'error'); }
    }));

    mount(ctx.root,
      pageHeader('My tasks', `In progress first, then the most severe. You are at ${user.location}, and ${user.is_available ? 'available for new tasks' : 'marked busy (no new tasks)'}.`,
        [availBtn, h('a', { href: '#/profile', class: 'btn btn-ghost' }, 'Change my location')]),
      h('div', { class: 'tasks-grid' },
        h('div', null,
          active.length
            ? h('ul', { class: 'tasks' }, active.map(card))
            : emptyState('No active tasks', 'Your department officer assigns tasks. They show up here.'),
          done.length ? h('details', { class: 'done-list' },
            h('summary', null, `Completed tasks (${fmtNum(done.length)})`),
            h('ul', null, done.map((c) => h('li', null,
              h('a', { href: `#/complaints/${c.id}` }, `#${c.id} ${c.title}`),
              h('span', { class: 'muted' }, ` completed ${fmtDate(c.completed_date, false)}`))))) : null),
        h('div', { class: 'tasks-side' }, panel('Task map', [map.el, routeSlot]))));
  };
  await load();
}
