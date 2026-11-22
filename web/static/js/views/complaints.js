import { h, mount, toast, confirmDialog, fmtDate, fmtNum, fmtDec, ago, CATEGORIES, STATUSES, statusLabel, isUnassigned, sevLabel } from '../dom.js';
import { api, qs, getGraph, getDepartments } from '../api.js';
import {
  pageHeader, panel, loading, complaintTable, pagination, emptyState, statusChip, sevBadge, flagChips,
  stepper, locationSelect, withBusy, kv, field,
} from '../ui.js';
import { createCityMap, routeSteps } from '../map.js';
import { scoreBar } from '../charts.js';

const SORTS = [
  ['submitted_desc', 'Newest first'],
  ['submitted_asc', 'Oldest first'],
  ['severity_desc', 'Most severe first'],
  ['severity_asc', 'Least severe first'],
  ['id_desc', 'Highest ID first'],
  ['id_asc', 'Lowest ID first'],
];

function listTitle(user) {
  switch (user.role) {
    case 'ADMIN': return ['All complaints', 'Every complaint on record. Filter, then open one to act on it.'];
    case 'DEPT_OFFICER': return [`${user.department} complaints`, 'Complaints routed to your department. Open one to assign a field worker.'];
    case 'WORKER': return ['Assigned complaints', 'Every complaint assigned to you, including finished ones.'];
    default: return ['My complaints', 'Everything you have reported, newest first.'];
  }
}

export async function complaintListView(ctx) {
  const q = ctx.query;
  const user = ctx.user;
  const staff = user.role === 'ADMIN' || user.role === 'DEPT_OFFICER';
  const params = {
    q: q.get('q') || '',
    status: q.get('status') || '',
    category: q.get('category') || '',
    location_id: q.get('location_id') || '',
    flag: q.get('flag') || '',
    min_severity: q.get('min_severity') || '',
    sort: q.get('sort') || 'submitted_desc',
    page: Math.max(1, parseInt(q.get('page') || '1', 10) || 1),
    page_size: 20,
  };

  const graph = await getGraph();
  if (!ctx.alive()) return;
  const [title, sub] = listTitle(user);

  const apply = (patch) => ctx.setQuery({ ...params, page_size: undefined, ...patch, page: patch.page || 1 });

  const search = h('input', { class: 'input', type: 'search', value: params.q, placeholder: 'Words, place, or #id', 'aria-label': 'Search complaints' });
  const selectOf = (label, key, options) => {
    const sel = h('select', { class: 'input', 'aria-label': label, onchange: () => apply({ [key]: sel.value }) },
      options.map(([v, l]) => h('option', { value: v, selected: String(params[key]) === String(v) }, l)));
    return h('label', { class: 'filter' }, h('span', { class: 'filter-label' }, label), sel);
  };
  const locSel = locationSelect(graph, params.location_id, { includeAll: true });
  locSel.addEventListener('change', () => apply({ location_id: locSel.value }));

  const filters = h('form', { class: 'filters', role: 'search', onsubmit: (e) => { e.preventDefault(); apply({ q: search.value.trim() }); } },
    h('div', { class: 'filter filter-search' }, h('span', { class: 'filter-label' }, 'Search'),
      h('div', { class: 'search-row' }, search, h('button', { class: 'btn btn-secondary', type: 'submit' }, 'Search'))),
    selectOf('Status', 'status', [['', 'Any status'], ...STATUSES.map((s) => [s, statusLabel(s)])]),
    user.role !== 'DEPT_OFFICER' ? selectOf('Category', 'category', [['', 'Any category'], ...CATEGORIES.map((c) => [c, c]), ['General', 'General']]) : null,
    h('label', { class: 'filter' }, h('span', { class: 'filter-label' }, 'Location'), locSel),
    selectOf('Minimum severity', 'min_severity', [['', 'Any severity'], ...[10, 9, 8, 7, 6, 5, 4, 3, 2].map((n) => [String(n), `${n} or higher`])]),
    staff ? selectOf('Flags', 'flag', [['', 'Any'], ['duplicate', 'Duplicates'], ['irrelevant', 'Possible spam'], ['clean', 'Not flagged']]) : null,
    selectOf('Sort', 'sort', SORTS));

  const active = ['q', 'status', 'category', 'location_id', 'flag', 'min_severity'].some((k) => params[k]);
  const results = h('div', { class: 'results' }, loading('Loading complaints…'));
  const needWorker = user.role === 'DEPT_OFFICER' ? h('div') : null;

  mount(ctx.root,
    pageHeader(title, sub, user.role === 'USER' ? h('a', { href: '#/submit', class: 'btn btn-signal' }, 'Report a problem') : null),
    needWorker,
    filters,
    active ? h('p', { class: 'filter-reset' }, h('a', { href: '#/complaints' }, 'Clear all filters')) : null,
    results);

  if (needWorker) loadNeedsWorker(ctx, needWorker);

  const data = await api(`/complaints${qs({ ...params })}`);
  if (!ctx.alive()) return;
  if (!data.items.length && data.total > 0) {
    // e.g. #/complaints?page=99: there are complaints, just not on this page
    mount(results, emptyState('This page is past the end of the list', `There are ${fmtNum(data.total)} matching complaints.`,
      h('button', { type: 'button', class: 'btn btn-secondary', onclick: () => apply({ page: 1 }) }, 'Go to the first page')));
    return;
  }
  if (!data.items.length) {
    mount(results, active
      ? emptyState('No complaints match these filters', 'Loosen a filter or clear them all.', h('a', { href: '#/complaints', class: 'btn btn-secondary' }, 'Clear filters'))
      : emptyState('No complaints yet', user.role === 'USER' ? 'When you report a problem it shows up here.' : 'Nothing has been routed here yet.',
        user.role === 'USER' ? h('a', { href: '#/submit', class: 'btn btn-primary' }, 'Report a problem') : null));
    return;
  }
  mount(results,
    complaintTable(data.items, { showSubmitter: staff, showWorker: staff }),
    pagination({ page: data.page, pageSize: data.page_size, total: data.total, onPage: (p) => apply({ page: p }) }));
}

async function loadNeedsWorker(ctx, slot) {
  try {
    const data = await api(`/complaints${qs({ page_size: 200, sort: 'severity_desc' })}`);
    if (!ctx.alive()) return;
    // Routed to the department by an admin (ASSIGNED) and no worker yet;
    // PENDING/REVIEWED ones are not the officer's to assign until then
    const waiting = data.items.filter((c) => c.status === 'ASSIGNED' && isUnassigned(c));
    if (!waiting.length) return;
    mount(slot, panel(`Waiting for a worker (${fmtNum(waiting.length)}${data.total > data.items.length ? '+' : ''})`,
      h('ul', { class: 'mini-list' }, waiting.slice(0, 6).map((c) => h('li', null,
        h('a', { href: `#/complaints/${c.id}`, class: 'mini-row' },
          sevBadge(c.severity, { long: false }),
          h('span', { class: 'mini-title' }, c.title),
          h('span', { class: 'mini-meta' }, `${c.location}, #${c.id}`),
          statusChip(c.status))))),
      { sub: 'Most severe first. Open one to pick a worker and see the route.', className: 'panel-accent' }));
  } catch { /* optional panel */ }
}

// ── Detail ─────────────────────────────────────────────────

export async function complaintDetailView(ctx) {
  const id = Number(ctx.params[0]);
  const load = async (flash) => {
    const [detail, graph] = await Promise.all([api(`/complaints/${id}`), getGraph()]);
    if (!ctx.alive()) return;
    ctx.setTitle(`#${id} ${detail.complaint.title}`);
    renderDetail(ctx, detail, graph, load, flash);
  };
  await load();
}

function renderDetail(ctx, detail, graph, reload, flash) {
  const c = detail.complaint;
  const user = ctx.user;
  const route = detail.route || null;

  const map = createCityMap(graph, { selected: [c.location_id], route, caption: true, ariaLabel: `Map showing ${c.location}` });

  const description = h('p', { class: 'description' }, c.description || '(no description)');

  const facts = kv([
    ['Location', c.location],
    ['Category', c.category],
    ['Department', c.department || '–'],
    ['Severity', h('span', { class: 'kv-inline' }, sevBadge(c.severity), c.nlp_score !== undefined && c.nlp_score !== null && Number(c.nlp_score) !== Number(c.severity)
      ? h('span', { class: 'muted' }, `NLP estimate was ${c.nlp_score}`)
      : h('span', { class: 'muted' }, 'Same as the NLP estimate'))],
    ['Submitted', `${fmtDate(c.submitted_date)} (${ago(c.submitted_date)})`],
    c.completed_date ? ['Completed', fmtDate(c.completed_date)] : null,
    c.submitter_name ? ['Reported by', c.submitter_name] : null,
    ['Field worker', detail.worker ? `${detail.worker.full_name || detail.worker.username}${detail.worker.location ? `, now at ${detail.worker.location}` : ''}` : (c.worker_name || 'Not assigned yet')],
  ]);

  const timeline = (detail.timeline || []).length
    ? h('ol', { class: 'timeline' }, detail.timeline.map((t) => h('li', null,
      h('time', null, fmtDate(t.time)), h('span', null, t.message))))
    : h('p', { class: 'muted' }, 'No logged activity for this complaint yet.');

  const flags = [];
  if (c.is_irrelevant) {
    flags.push(h('div', { class: 'notice notice-warn', role: 'note' },
      h('strong', null, 'Flagged as possible spam. '),
      'The text checks found it too short, garbled or low on meaningful words. It stays on record for an administrator to review.'));
  }
  if (c.is_duplicate) flags.push(duplicatePanel(c, detail.original, user));

  const actions = actionPanel(ctx, c, detail, graph, reload);

  mount(ctx.root,
    h('p', { class: 'back' }, h('a', { href: '#/complaints' }, 'Back to complaints')),
    h('header', { class: 'detail-head' },
      h('p', { class: 'detail-id' }, `Complaint #${c.id}`),
      h('h1', { class: 'page-title' }, c.title),
      h('div', { class: 'chips' }, statusChip(c.status), sevBadge(c.severity), flagChips(c))),
    flash || null,
    h('div', { class: 'detail-grid' },
      h('div', { class: 'detail-main' },
        panel('Progress', stepper(c.status)),
        flags,
        panel('What was reported', [description, facts]),
        panel(route ? 'Route for the field worker' : 'Location', map.el,
          { sub: route ? 'Shortest path from the worker’s current location, computed with Dijkstra in the C engine.' : `${c.location}, ringed on the map.` }),
        panel('Activity', timeline, { sub: 'From the shared activity log.' })),
      h('div', { class: 'detail-side' },
        actions,
        route ? panel('Route summary', routeSteps(route, graph)) : null)));
}

function duplicatePanel(c, original, user) {
  const canExplain = (user.role === 'ADMIN' || user.role === 'DEPT_OFFICER') && original;
  const breakdown = h('div', { class: 'breakdown-slot' });
  const btn = canExplain ? h('button', { type: 'button', class: 'btn btn-secondary btn-sm' }, 'Why was this flagged?') : null;
  if (btn) {
    btn.addEventListener('click', () => withBusy(btn, 'Comparing…', async () => {
      try {
        const cmp = await api(`/compare${qs({ a: c.id, b: original.id })}`);
        mount(breakdown, compareTable(cmp, c, original));
        btn.remove();
      } catch (err) { toast(err.message, 'error'); }
    }));
  }
  const side = (label, x) => h('div', { class: 'dup-side' },
    h('p', { class: 'dup-label' }, label),
    h('a', { href: `#/complaints/${x.id}`, class: 'dup-title' }, `#${x.id} ${x.title}`),
    h('p', { class: 'dup-meta' }, `${x.location}, ${x.category}, ${fmtDate(x.submitted_date, false)}`),
    h('p', { class: 'dup-desc' }, x.description || ''));
  return panel('Possible duplicate', [
    h('p', { class: 'panel-sub' }, original
      ? 'The duplicate detector matched this report to an earlier complaint. Both stay on record; staff decide how to handle them.'
      : `Linked to complaint #${c.duplicate_of}, which is not visible to you.`),
    original ? h('div', { class: 'dup-grid' }, side('This complaint', c), side('Earlier complaint', original)) : null,
    btn, breakdown,
  ], { className: 'panel-accent' });
}

function compareTable(cmp, a, b) {
  const sameUser = a.user_id !== undefined && b.user_id !== undefined && Number(a.user_id) === Number(b.user_id);
  const sameLoc = Number(cmp.location) === 1;
  const threshold = sameUser ? 0.75 : sameLoc ? 0.82 : 0.88;
  const rows = [
    ['Word overlap (Jaccard)', cmp.jaccard, 0.4],
    ['Edit distance similarity (Levenshtein)', cmp.levenshtein, 0.3],
    ['Same category', cmp.category, 0.2],
    ['Same location', cmp.location, 0.1],
  ];
  const penalty = Number(cmp.title_penalty);
  return h('div', { class: 'breakdown' },
    h('div', { class: 'table-scroll' }, h('table', { class: 'table table-compact' },
      h('thead', null, h('tr', null, h('th', { scope: 'col' }, 'Signal'), h('th', { scope: 'col' }, 'Score'), h('th', { scope: 'col', class: 'num' }, 'Weight'), h('th', { scope: 'col', class: 'num' }, 'Adds'))),
      h('tbody', null,
        rows.map(([label, v, w]) => h('tr', null,
          h('td', null, label),
          h('td', null, h('span', { class: 'score-row' }, scoreBar(v), h('span', { class: 'num' }, fmtDec(v, 2)))),
          h('td', { class: 'num' }, fmtDec(w, 1)),
          h('td', { class: 'num' }, fmtDec(v * w, 3)))),
        h('tr', null,
          h('td', null, 'Title subject overlap'),
          h('td', null, h('span', { class: 'score-row' }, scoreBar(cmp.title), h('span', { class: 'num' }, fmtDec(cmp.title, 2)))),
          h('td', { class: 'num', colspan: '2' }, `multiplier ×${fmtDec(penalty, 1)}`))))),
    h('div', { class: 'combined' },
      h('span', null, 'Combined score'),
      h('span', { class: 'score-row' }, scoreBar(cmp.combined, { marker: threshold }), h('strong', { class: 'num' }, fmtDec(cmp.combined, 3))),
      h('span', { class: 'muted' }, `Threshold ${threshold}: ${sameUser ? 'same person re-submitting' : sameLoc ? 'different people, same location' : 'different people and locations'}. ${Number(cmp.combined) >= threshold ? 'At or above the threshold.' : 'Below the threshold with today’s data.'}`)),
    h('p', { class: 'caption' }, 'Weights, the title multiplier (×0.5 when title subjects barely overlap, ×0.7 when they partly overlap) and the thresholds come from the C duplicate detector.'));
}

// ── Role actions ───────────────────────────────────────────

function actionPanel(ctx, c, detail, graph, reload) {
  const user = ctx.user;
  const done = c.status === 'COMPLETED';
  if (user.role === 'ADMIN') return adminActions(c, reload, done);
  if (user.role === 'DEPT_OFFICER') return officerActions(ctx, c, detail, graph, reload, done);
  if (user.role === 'WORKER') return workerActions(ctx, c, graph, reload, done);
  return panel('What happens next', h('p', { class: 'muted' }, nextStepText(c)));
}

function nextStepText(c) {
  switch (c.status) {
    case 'PENDING': return 'An administrator reviews pending complaints in order of severity. Yours is in that queue.';
    case 'REVIEWED': return 'Reviewed. It will be routed to the responsible department next.';
    case 'ASSIGNED': return c.worker_name ? `Assigned to ${c.worker_name}, who will start work on it.` : 'With the department, waiting for a field worker.';
    case 'IN_PROGRESS': return 'A field worker is on it.';
    case 'COMPLETED': return 'Marked as fixed. If the problem is back, report it again.';
    default: return '';
  }
}

function adminActions(c, reload, done) {
  const blocks = [];
  if (c.status === 'PENDING') {
    const btn = h('button', { type: 'button', class: 'btn btn-primary btn-block' }, 'Mark as reviewed');
    btn.addEventListener('click', () => withBusy(btn, 'Saving…', async () => {
      try {
        await api(`/complaints/${c.id}/review`, { method: 'POST' });
        toast(`Complaint #${c.id} marked as reviewed.`, 'success');
        await reload();
      } catch (err) { toast(err.message, 'error'); }
    }));
    blocks.push(h('div', { class: 'action' }, h('p', { class: 'action-text' }, 'Confirms the complaint is genuine and ready for a department.'), btn));
  }
  if (!done) {
    const sev = h('select', { class: 'input' }, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10].map((n) => h('option', { value: String(n), selected: n === Number(c.severity) }, `${n} (${sevLabel(n)})`)));
    const sevBtn = h('button', { type: 'submit', class: 'btn btn-secondary' }, 'Change severity');
    const sevForm = h('form', { class: 'action', onsubmit: async (e) => {
      e.preventDefault();
      const val = Number(sev.value);
      if (val === Number(c.severity)) { toast('That is already the severity.', 'info'); return; }
      await withBusy(sevBtn, 'Saving…', async () => {
        try {
          await api(`/complaints/${c.id}/escalate`, { method: 'POST', body: { severity: val } });
          toast(`Severity of #${c.id} changed to ${val}.`, 'success');
          await reload();
        } catch (err) { toast(err.message, 'error'); }
      });
    } }, field('Severity', sev, { hint: 'Moves the complaint in the review queue. The original NLP estimate is kept.' }), sevBtn);
    blocks.push(sevForm);

    const deptSel = h('select', { class: 'input', disabled: true }, h('option', null, 'Loading departments…'));
    const deptBtn = h('button', { type: 'submit', class: 'btn btn-secondary', disabled: true }, 'Assign to department');
    getDepartments().then((depts) => {
      deptSel.replaceChildren(...depts.map((d) => h('option', { value: String(d.id), selected: d.name === c.department }, d.name)));
      deptSel.disabled = false;
      deptBtn.disabled = false;
    }).catch((err) => toast(err.message, 'error'));
    const deptForm = h('form', { class: 'action', onsubmit: async (e) => {
      e.preventDefault();
      const depts = await getDepartments();
      const target = depts.find((d) => String(d.id) === deptSel.value);
      if (!target) return;
      if (target.name !== c.department && !isUnassigned(c)) {
        const ok = await confirmDialog({
          title: `Move #${c.id} to ${target.name}?`,
          body: `It is currently with ${c.worker_name || 'a field worker'} in ${c.department}. Moving it clears that assignment.`,
          confirmLabel: 'Move complaint',
        });
        if (!ok) return;
      }
      await withBusy(deptBtn, 'Assigning…', async () => {
        try {
          await api(`/complaints/${c.id}/assign-department`, { method: 'POST', body: { department_id: target.id } });
          toast(`Complaint #${c.id} assigned to ${target.name}.`, 'success');
          await reload();
        } catch (err) { toast(err.message, 'error'); }
      });
    } }, field('Department', deptSel, { hint: 'Sets the department and category. Pending or reviewed complaints become Assigned.' }), deptBtn);
    blocks.push(deptForm);
  } else {
    blocks.push(h('p', { class: 'muted' }, 'This complaint is completed. No further actions.'));
  }
  return panel('Administrator actions', blocks, { className: 'panel-actions-box' });
}

function officerActions(ctx, c, detail, graph, reload, done) {
  const user = ctx.user;
  if (c.department !== user.department) {
    return panel('Department officer', h('p', { class: 'muted' }, `This complaint belongs to ${c.department}.`));
  }
  if (done) return panel('Department officer', h('p', { class: 'muted' }, 'Completed. No further actions.'));
  if (!isUnassigned(c)) {
    return panel('Field worker', h('p', null, `${c.worker_name || `Worker #${c.worker_id}`} is assigned. Status: ${statusLabel(c.status)}.`));
  }
  const list = h('div', { class: 'worker-pick' }, loading('Finding workers…'));
  const btn = h('button', { type: 'submit', class: 'btn btn-primary btn-block', disabled: true }, 'Assign worker');
  const form = h('form', { class: 'action', onsubmit: async (e) => {
    e.preventDefault();
    const picked = form.querySelector('input[name="worker"]:checked');
    if (!picked) { toast('Choose a worker first.', 'error'); return; }
    await withBusy(btn, 'Assigning…', async () => {
      try {
        const res = await api(`/complaints/${c.id}/assign-worker`, { method: 'POST', body: { worker_id: Number(picked.value) } });
        toast(`Worker assigned to #${c.id}.`, 'success');
        const flash = res.route ? h('div', { class: 'notice notice-ok', role: 'status' },
          h('strong', null, 'Assigned. '), `Shortest route for the worker: ${res.route.distance} km, shown on the map.`) : null;
        await reload(flash);
      } catch (err) { toast(err.message, 'error'); }
    });
  } }, h('fieldset', { class: 'fieldset' }, h('legend', null, 'Choose a field worker'), list), btn);

  (async () => {
    try {
      const workers = await api(`/workers${qs({ department: user.department })}`);
      if (!ctx.alive()) return;
      if (!workers.length) { mount(list, h('p', { class: 'muted' }, 'No workers in this department.')); return; }
      const routes = await Promise.all(workers.map((w) => api(`/route${qs({ from: w.location_id, to: c.location_id })}`).catch(() => null)));
      if (!ctx.alive()) return;
      const order = workers.map((w, i) => ({ w, r: routes[i] })).sort((x, y) => (Number(y.w.is_available) - Number(x.w.is_available)) || ((x.r ? x.r.distance : 1e9) - (y.r ? y.r.distance : 1e9)));
      mount(list, order.map(({ w, r }, i) => h('label', { class: `worker-opt${w.is_available ? '' : ' worker-busy'}` },
        h('input', { type: 'radio', name: 'worker', value: String(w.id), disabled: !w.is_available, checked: i === 0 && !!w.is_available }),
        h('span', { class: 'worker-opt-text' },
          h('strong', null, w.full_name || w.username),
          h('span', null, `At ${w.location}${r ? `, ${r.distance} km away` : ''}`),
          h('span', null, `${fmtNum(w.active_tasks)} active task${Number(w.active_tasks) === 1 ? '' : 's'}, ${w.is_available ? 'available' : 'marked busy'}`)))));
      btn.disabled = !workers.some((w) => w.is_available);
    } catch (err) {
      mount(list, h('p', { class: 'form-error' }, err.message));
    }
  })();

  return panel('Assign a field worker', form, { sub: 'Closest available workers first. Distances are shortest road paths.', className: 'panel-actions-box' });
}

function workerActions(ctx, c, graph, reload, done) {
  const mine = Number(c.worker_id) === Number(ctx.user.id);
  if (!mine) return panel('Field worker', h('p', { class: 'muted' }, 'This task is not assigned to you.'));
  if (done) return panel('Task finished', h('p', null, `You completed this on ${fmtDate(c.completed_date)}.`));
  const blocks = [];
  if (c.status !== 'IN_PROGRESS') {
    const start = h('button', { type: 'button', class: 'btn btn-primary btn-block' }, 'Start task');
    start.addEventListener('click', () => withBusy(start, 'Starting…', async () => {
      try {
        const res = await api(`/complaints/${c.id}/start`, { method: 'POST' });
        toast(`Task #${c.id} started.`, 'success');
        await reload(res.route ? h('div', { class: 'notice notice-ok', role: 'status' }, h('strong', null, 'On your way. '), `${res.route.distance} km by the shortest route, shown on the map.`) : null);
      } catch (err) { toast(err.message, 'error'); }
    }));
    blocks.push(h('div', { class: 'action' }, h('p', { class: 'action-text' }, 'Marks the task as in progress and plots your route.'), start));
  }
  const complete = h('button', { type: 'button', class: `btn ${c.status === 'IN_PROGRESS' ? 'btn-primary' : 'btn-secondary'} btn-block` }, 'Mark as completed');
  complete.addEventListener('click', async () => {
    const ok = await confirmDialog({ title: `Mark #${c.id} as completed?`, body: 'The citizen will see it as fixed, with today’s date.', confirmLabel: 'Mark as completed' });
    if (!ok) return;
    await withBusy(complete, 'Saving…', async () => {
      try {
        await api(`/complaints/${c.id}/complete`, { method: 'POST' });
        toast(`Task #${c.id} completed.`, 'success');
        await reload();
      } catch (err) { toast(err.message, 'error'); }
    });
  });
  blocks.push(h('div', { class: 'action' }, complete));
  return panel('Your task', blocks, { className: 'panel-actions-box' });
}
