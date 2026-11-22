import { h, mount, toast, fmtNum, ago, sevLabel } from '../dom.js';
import { api, qs, getDepartments } from '../api.js';
import { pageHeader, loading, emptyState, sevBadge, flagChips, withBusy } from '../ui.js';

export async function queueView(ctx) {
  const limit = [25, 50, 100, 200].includes(Number(ctx.query.get('limit'))) ? Number(ctx.query.get('limit')) : 50;
  const listSlot = h('div', null, loading('Building the priority queue…'));
  const limitSel = h('select', { class: 'input input-sm', 'aria-label': 'How many to show', onchange: () => ctx.setQuery({ limit: limitSel.value }) },
    [25, 50, 100, 200].map((n) => h('option', { value: String(n), selected: n === limit }, `Show ${n}`)));

  mount(ctx.root,
    pageHeader('Review queue',
      'Pending complaints in the order the C engine’s max-heap returns them: highest severity first, and the oldest first when severities tie.',
      limitSel),
    listSlot);

  let depts = [];
  try { depts = await getDepartments(); } catch (err) { toast(err.message, 'error'); }

  const load = async () => {
    const data = await api(`/queue${qs({ limit })}`);
    if (!ctx.alive()) return;
    if (!data.items.length) {
      mount(listSlot, emptyState('The queue is empty', 'Every complaint has been reviewed. New submissions land here automatically.'));
      return;
    }
    mount(listSlot,
      h('p', { class: 'queue-count' }, data.total > data.items.length
        ? `Showing the top ${fmtNum(data.items.length)} of ${fmtNum(data.total)} pending complaints.`
        : `${fmtNum(data.total)} pending complaint${data.total === 1 ? '' : 's'}.`),
      h('ol', { class: 'queue' }, data.items.map((c, i) => queueItem(c, i + 1, depts, load))));
  };
  await load();
}

function queueItem(c, rank, depts, reload) {
  const review = h('button', { type: 'button', class: 'btn btn-primary btn-sm' }, 'Mark as reviewed');
  review.addEventListener('click', () => withBusy(review, 'Saving…', async () => {
    try {
      await api(`/complaints/${c.id}/review`, { method: 'POST' });
      toast(`Complaint #${c.id} marked as reviewed.`, 'success');
      await reload();
    } catch (err) { toast(err.message, 'error'); }
  }));

  const sev = h('select', { class: 'input input-sm', 'aria-label': `Severity for complaint ${c.id}` },
    [1, 2, 3, 4, 5, 6, 7, 8, 9, 10].map((n) => h('option', { value: String(n), selected: n === Number(c.severity) }, `${n} ${sevLabel(n)}`)));
  const sevBtn = h('button', { type: 'submit', class: 'btn btn-ghost btn-sm' }, 'Set');
  const sevForm = h('form', { class: 'inline-form', onsubmit: async (e) => {
    e.preventDefault();
    if (Number(sev.value) === Number(c.severity)) { toast('That is already the severity.', 'info'); return; }
    await withBusy(sevBtn, '…', async () => {
      try {
        await api(`/complaints/${c.id}/escalate`, { method: 'POST', body: { severity: Number(sev.value) } });
        toast(`Severity of #${c.id} set to ${sev.value}. The queue has been re-ordered.`, 'success');
        await reload();
      } catch (err) { toast(err.message, 'error'); }
    });
  } }, h('label', { class: 'inline-label' }, 'Severity', sev), sevBtn);

  const dept = h('select', { class: 'input input-sm', 'aria-label': `Department for complaint ${c.id}` },
    depts.map((d) => h('option', { value: String(d.id), selected: d.name === c.department }, d.name)));
  const deptBtn = h('button', { type: 'submit', class: 'btn btn-ghost btn-sm', disabled: !depts.length }, 'Assign');
  const deptForm = h('form', { class: 'inline-form', onsubmit: async (e) => {
    e.preventDefault();
    const d = depts.find((x) => String(x.id) === dept.value);
    if (!d) return;
    await withBusy(deptBtn, '…', async () => {
      try {
        await api(`/complaints/${c.id}/assign-department`, { method: 'POST', body: { department_id: d.id } });
        toast(`Complaint #${c.id} assigned to ${d.name}.`, 'success');
        await reload();
      } catch (err) { toast(err.message, 'error'); }
    });
  } }, h('label', { class: 'inline-label' }, 'Department', dept), deptBtn);

  const snippet = (c.description || '').length > 180 ? `${c.description.slice(0, 177).trimEnd()}…` : (c.description || '');

  return h('li', { class: 'queue-item' },
    h('span', { class: 'queue-rank', 'aria-label': `Priority ${rank}` }, String(rank)),
    h('div', { class: 'queue-body' },
      h('div', { class: 'queue-top' },
        sevBadge(c.severity),
        h('a', { href: `#/complaints/${c.id}`, class: 'queue-title' }, c.title),
        flagChips(c)),
      h('p', { class: 'queue-meta' },
        `#${c.id} at ${c.location}, ${c.category}, submitted ${ago(c.submitted_date)}${c.submitter_name ? ` by ${c.submitter_name}` : ''}`),
      snippet ? h('p', { class: 'queue-desc' }, snippet) : null,
      h('div', { class: 'queue-actions' }, review, sevForm, deptForm)));
}
