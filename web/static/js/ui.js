// Shared UI building blocks.

import { h, fmtDate, ago, statusLabel, sevLabel, sevKey, STATUSES, fmtNum } from './dom.js';

export function pageHeader(title, sub, actions) {
  return h('header', { class: 'page-head' },
    h('div', { class: 'page-head-text' },
      h('h1', { class: 'page-title' }, title),
      sub ? h('p', { class: 'page-sub' }, sub) : null),
    actions ? h('div', { class: 'page-actions' }, actions) : null);
}

export function panel(title, children, { actions, className = '', sub } = {}) {
  return h('section', { class: `panel ${className}` },
    title ? h('div', { class: 'panel-head' },
      h('div', null,
        h('h2', { class: 'panel-title' }, title),
        sub ? h('p', { class: 'panel-sub' }, sub) : null),
      actions ? h('div', { class: 'panel-actions' }, actions) : null) : null,
    children);
}

export function loading(label = 'Loading…') {
  return h('div', { class: 'loading', role: 'status' }, h('span', { class: 'spinner', 'aria-hidden': 'true' }), label);
}

export function errorState(err, retry) {
  return h('div', { class: 'state state-error', role: 'alert' },
    h('h2', null, 'This view could not load'),
    h('p', null, err && err.message ? err.message : 'Something went wrong.'),
    retry ? h('button', { class: 'btn btn-secondary', type: 'button', onclick: retry }, 'Try again') : null);
}

export function emptyState(title, body, action) {
  return h('div', { class: 'state' },
    h('h2', null, title),
    body ? h('p', null, body) : null,
    action || null);
}

export function statusChip(status) {
  return h('span', { class: `chip chip-status st-${(status || '').toLowerCase()}` },
    h('span', { class: 'chip-dot', 'aria-hidden': 'true' }), statusLabel(status));
}

export function sevBadge(sev, { long = true } = {}) {
  const n = Number(sev) || 0;
  return h('span', { class: `sev sev-${sevKey(n)}`, title: `Severity ${n} of 10 (${sevLabel(n)})` },
    h('span', { class: 'sev-num' }, String(n)),
    long ? h('span', { class: 'sev-text' }, sevLabel(n)) : null);
}

export function flagChips(c) {
  const out = [];
  if (c.is_duplicate) out.push(h('span', { class: 'chip chip-flag chip-dup' }, c.duplicate_of ? `Duplicate of #${c.duplicate_of}` : 'Duplicate'));
  if (c.is_irrelevant) out.push(h('span', { class: 'chip chip-flag chip-spam' }, 'Possible spam'));
  return out;
}

export function stepper(status) {
  const idx = STATUSES.indexOf(status);
  return h('ol', { class: 'stepper', 'aria-label': `Progress: ${statusLabel(status)}` },
    STATUSES.map((st, i) => h('li', {
      class: `step${i < idx ? ' step-done' : ''}${i === idx ? ' step-current' : ''}`,
      'aria-current': i === idx ? 'step' : null,
    },
    h('span', { class: 'step-mark', 'aria-hidden': 'true' }, i < idx || (i === idx && status === 'COMPLETED') ? '✓' : String(i + 1)),
    h('span', { class: 'step-label' }, statusLabel(st)))));
}

export function complaintTable(items, { showSubmitter = false, showWorker = false } = {}) {
  return h('div', { class: 'table-scroll table-cards' },
    h('table', { class: 'table complaints-table' },
      h('thead', null, h('tr', null,
        h('th', { scope: 'col', class: 'num' }, 'ID'),
        h('th', { scope: 'col' }, 'Complaint'),
        h('th', { scope: 'col' }, 'Category'),
        h('th', { scope: 'col' }, 'Severity'),
        h('th', { scope: 'col' }, 'Status'),
        showWorker ? h('th', { scope: 'col' }, 'Worker') : null,
        h('th', { scope: 'col' }, 'Submitted'))),
      h('tbody', null, items.map((c) => h('tr', null,
        h('td', { class: 'num', 'data-label': 'ID' }, h('a', { href: `#/complaints/${c.id}`, class: 'id-link' }, `#${c.id}`)),
        h('td', { class: 'cell-main', 'data-label': 'Complaint' },
          h('a', { href: `#/complaints/${c.id}`, class: 'title-link' }, c.title || '(untitled)'),
          h('span', { class: 'cell-meta' },
            c.location || '',
            showSubmitter && c.submitter_name ? `, reported by ${c.submitter_name}` : ''),
          flagChips(c).length ? h('span', { class: 'cell-flags' }, flagChips(c)) : null),
        h('td', { 'data-label': 'Category' }, c.category || '–'),
        h('td', { 'data-label': 'Severity' }, sevBadge(c.severity)),
        h('td', { 'data-label': 'Status' }, statusChip(c.status)),
        showWorker ? h('td', { 'data-label': 'Worker' }, c.worker_name || h('span', { class: 'muted' }, 'Unassigned')) : null,
        h('td', { 'data-label': 'Submitted' }, h('span', { title: fmtDate(c.submitted_date), class: 'cell-date' }, fmtDate(c.submitted_date, false)), h('span', { class: 'cell-meta' }, ago(c.submitted_date))))))));
}

export function pagination({ page, pageSize, total, onPage }) {
  const pages = Math.max(1, Math.ceil(total / pageSize));
  if (pages <= 1) return h('p', { class: 'pager-info' }, `${fmtNum(total)} result${total === 1 ? '' : 's'}`);
  const from = (page - 1) * pageSize + 1;
  const to = Math.min(total, page * pageSize);
  return h('nav', { class: 'pager', 'aria-label': 'Pagination' },
    h('p', { class: 'pager-info' }, `${fmtNum(from)}–${fmtNum(to)} of ${fmtNum(total)}`),
    h('div', { class: 'pager-buttons' },
      h('button', { class: 'btn btn-ghost btn-sm', type: 'button', disabled: page <= 1, onclick: () => onPage(page - 1) }, 'Previous'),
      h('span', { class: 'pager-page' }, `Page ${page} of ${pages}`),
      h('button', { class: 'btn btn-ghost btn-sm', type: 'button', disabled: page >= pages, onclick: () => onPage(page + 1) }, 'Next')));
}

let fieldSeq = 0;
export function field(label, control, { hint, id } = {}) {
  const fid = id || control.id || `f${++fieldSeq}`;
  control.id = fid;
  const hintEl = hint ? h('p', { class: 'field-hint', id: `${fid}-hint` }, hint) : null;
  if (hintEl) control.setAttribute('aria-describedby', hintEl.id);
  return h('div', { class: 'field' }, h('label', { for: fid, class: 'field-label' }, label), control, hintEl);
}

export function locationSelect(graph, value, { includeAll = false, allLabel = 'All locations', required = false, placeholder } = {}) {
  const sel = h('select', { class: 'input', required: required || null },
    includeAll ? h('option', { value: '' }, allLabel) : null,
    placeholder ? h('option', { value: '', disabled: true, selected: value === '' || value === null || value === undefined }, placeholder) : null,
    [...graph.nodes].sort((a, b) => a.name.localeCompare(b.name)).map((n) => h('option', { value: String(n.id), selected: String(value) === String(n.id) }, n.name)));
  return sel;
}

// Runs an async action with the button disabled and a busy label.
export async function withBusy(btn, label, fn) {
  const prev = btn.textContent;
  btn.disabled = true;
  btn.setAttribute('aria-busy', 'true');
  if (label) btn.textContent = label;
  try {
    return await fn();
  } finally {
    btn.disabled = false;
    btn.removeAttribute('aria-busy');
    btn.textContent = prev;
  }
}

export function kv(rows) {
  return h('dl', { class: 'kv' }, rows.filter(Boolean).map(([k, v]) => [h('dt', null, k), h('dd', null, v)]));
}
