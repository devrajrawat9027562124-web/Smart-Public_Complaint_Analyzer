import { h, mount, toast, debounce, fmtDec, CATEGORIES, sevLabel, sevKey } from '../dom.js';
import { api, getGraph } from '../api.js';
import { pageHeader, panel, field, locationSelect, withBusy } from '../ui.js';
import { createCityMap } from '../map.js';
import { severityMeter, rankedBars } from '../charts.js';

const TITLE_MAX = 99;    // bytes, as stored by the C engine
const DESC_MAX = 499;
const bytes = (s) => new TextEncoder().encode(s).length;

export async function submitView(ctx) {
  const graph = await getGraph();
  if (!ctx.alive()) return;

  const title = h('input', { class: 'input', maxlength: String(TITLE_MAX), required: true, autocomplete: 'off', placeholder: 'For example: Water pipe burst near the school' });
  const desc = h('textarea', { class: 'input textarea', rows: '6', maxlength: String(DESC_MAX), required: true, placeholder: 'What is wrong, since when, and who is affected? Mention anything dangerous.' });
  const titleCount = h('span', { class: 'counter' }, `0 / ${TITLE_MAX} bytes`);
  const descCount = h('span', { class: 'counter' }, `0 / ${DESC_MAX} bytes`);
  const loc = locationSelect(graph, '', { required: true, placeholder: 'Choose a location' });
  const cat = h('select', { class: 'input' },
    h('option', { value: '' }, 'Use the suggested category'),
    CATEGORIES.map((c) => h('option', { value: c }, c)));
  const err = h('p', { class: 'form-error', role: 'alert', hidden: true });
  const submit = h('button', { type: 'submit', class: 'btn btn-signal btn-lg' }, 'Submit complaint');

  const map = createCityMap(graph, {
    caption: false,
    ariaLabel: 'Choose the location on the map',
    onNodeClick: (id) => {
      loc.value = String(id);
      map.setSelected([id]);
      schedule();
    },
  });

  const analysis = h('div', { class: 'analysis', 'aria-live': 'polite' }, idleAnalysis());

  const form = h('form', { class: 'form submit-form', novalidate: true },
    h('div', { class: 'field' },
      h('div', { class: 'field-label-row' }, h('label', { for: 'c-title', class: 'field-label' }, 'Title'), titleCount),
      Object.assign(title, { id: 'c-title' })),
    h('div', { class: 'field' },
      h('div', { class: 'field-label-row' }, h('label', { for: 'c-desc', class: 'field-label' }, 'Description'), descCount),
      Object.assign(desc, { id: 'c-desc' })),
    field('Location', loc, { hint: 'Pick from the list, or select a point on the map below.' }),
    h('div', { class: 'picker-map' }, map.el),
    field('Category', cat, { hint: 'Leave it on the suggestion unless it is clearly wrong.' }),
    err,
    h('div', { class: 'form-actions' }, submit,
      h('p', { class: 'muted small' }, 'Your name stays visible only to staff. The text is stored as written; avoid personal details about other people.')));

  mount(ctx.root,
    pageHeader('Report a problem', 'Describe it in your own words. The analysis on the right updates as you type, so you can see how the system reads your report before you send it.'),
    h('div', { class: 'submit-grid' },
      panel(null, form, { className: 'submit-form-panel' }),
      h('aside', { class: 'submit-aside' }, panel('Live analysis', analysis, { sub: 'Computed by the same C engine the staff console uses.', className: 'analysis-panel' }))));

  let seq = 0;
  let last = null;
  const run = async () => {
    const t = title.value.trim();
    const d = desc.value.trim();
    if ((t + d).length < 3) { mount(analysis, idleAnalysis()); last = null; return; }
    const mySeq = ++seq;
    analysis.classList.add('is-refreshing');
    try {
      const body = { title: t, description: d };
      if (loc.value !== '') body.location_id = Number(loc.value);
      const res = await api('/analyze', { method: 'POST', body });
      if (mySeq !== seq || !ctx.alive()) return;
      last = res;
      mount(analysis, renderAnalysis(res, cat.value, body.location_id !== undefined));
      paintCategoryOption(cat, res.category);
    } catch (e) {
      if (mySeq !== seq) return;
      mount(analysis, h('p', { class: 'form-error' }, e.message));
    } finally {
      if (mySeq === seq) analysis.classList.remove('is-refreshing');
    }
  };
  const schedule = debounce(run, 450);

  // The limits are in bytes (the C engine's buffers): a Hindi letter takes 3
  // bytes, so counting characters let long Hindi text be cut on the server
  const countBytes = (el, counter, max) => {
    const n = bytes(el.value);
    counter.textContent = `${n} / ${max} bytes`;
    counter.classList.toggle('counter-over', n > max);
  };
  title.addEventListener('input', () => { countBytes(title, titleCount, TITLE_MAX); schedule(); });
  desc.addEventListener('input', () => { countBytes(desc, descCount, DESC_MAX); schedule(); });
  loc.addEventListener('change', () => { map.setSelected(loc.value === '' ? [] : [Number(loc.value)]); schedule(); });
  cat.addEventListener('change', () => { if (last) mount(analysis, renderAnalysis(last, cat.value, loc.value !== '')); });

  form.addEventListener('submit', async (e) => {
    e.preventDefault();
    err.hidden = true;
    const problems = [];
    if (!title.value.trim()) problems.push('Add a short title.');
    if (!desc.value.trim()) problems.push('Describe the problem.');
    if (loc.value === '') problems.push('Choose a location.');
    if (/\|/.test(title.value + desc.value)) problems.push('Remove the “|” character; the data files use it as a separator.');
    if (bytes(title.value.trim()) > TITLE_MAX || bytes(desc.value.trim()) > DESC_MAX)
      problems.push('Shorten the text: it is over the limit shown next to the field (a Hindi letter counts as 3 bytes).');
    // No department handles "General": with no category found in the text, one must be chosen
    if (cat.value === '' && last && (!last.category || last.category === 'General'))
      problems.push('Choose a category: none could be worked out from the text.');
    if (problems.length) {
      err.textContent = problems.join(' ');
      err.hidden = false;
      (problems[0].startsWith('Add') ? title : problems[0].startsWith('Describe') ? desc : loc).focus();
      return;
    }
    await withBusy(submit, 'Submitting…', async () => {
      try {
        const res = await api('/complaints', {
          method: 'POST',
          body: { title: title.value.trim(), description: desc.value.trim(), category: cat.value, location_id: Number(loc.value) },
        });
        const c = res.complaint;
        toast(`Complaint #${c.id} submitted.`, 'success');
        const notes = [];
        if (c.is_duplicate) notes.push(`It looks like complaint #${c.duplicate_of}, so it is flagged as a possible duplicate.`);
        if (c.is_irrelevant) notes.push('It was flagged as possible spam and will be checked by an administrator.');
        if (notes.length) toast(notes.join(' '), 'info', 8000);
        ctx.navigate(`#/complaints/${c.id}`);
      } catch (ex) {
        err.textContent = ex.message;
        err.hidden = false;
      }
    });
  });

  title.focus();
}

function paintCategoryOption(select, suggested) {
  const opt = select.querySelector('option[value=""]');
  if (opt) opt.textContent = suggested ? `Use the suggestion: ${suggested}` : 'Use the suggested category';
}

function idleAnalysis() {
  return h('div', { class: 'analysis-idle' },
    severityMeter(0),
    h('p', null, 'Start typing a title or description. The severity estimate, suggested category and duplicate check appear here.'));
}

function renderAnalysis(a, chosenCategory, hasLocation) {
  const sev = Number(a.severity) || 1;
  const kws = Array.isArray(a.keywords) ? a.keywords : [];
  const scores = a.category_scores || {};
  const scoreRows = Object.entries(scores).map(([label, value]) => ({ label, value: Number(value) || 0 })).sort((x, y) => y.value - x.value);
  const finalCat = chosenCategory || a.category;

  return h('div', { class: 'analysis-body' },
    h('div', { class: `sev-hero sev-${sevKey(sev)}` },
      h('div', { class: 'sev-hero-num' }, String(sev), h('span', null, '/10')),
      h('div', { class: 'sev-hero-text' },
        h('strong', null, `${sevLabel(sev)} severity`),
        h('span', null, kws.length ? `${kws.length} keyword${kws.length > 1 ? 's' : ''} matched` : 'No severity keywords found'))),
    severityMeter(sev),

    a.irrelevant ? h('div', { class: 'notice notice-warn', role: 'note' },
      h('strong', null, 'This may be flagged as spam. '),
      'It looks too short or garbled, or has fewer than three meaningful words. Add a clear sentence about the problem.') : null,

    a.duplicate ? h('div', { class: 'notice notice-dup', role: 'note' },
      h('strong', null, `Similar to complaint #${a.duplicate.id}. `),
      `“${a.duplicate.title}” matches with a score of ${fmtDec(a.duplicate.similarity, 2)} (threshold ${fmtDec(a.duplicate.threshold, 2)}). You can still submit; it will be linked as a possible duplicate.`)
      : hasLocation ? h('p', { class: 'muted small' }, 'No earlier complaint is close enough to count as a duplicate.')
        : h('p', { class: 'muted small' }, 'Choose a location to run the duplicate check.'),

    h('h3', { class: 'subhead' }, 'Why this score'),
    kws.length
      ? h('div', { class: 'table-scroll' }, h('table', { class: 'table table-compact kw-table' },
        h('thead', null, h('tr', null, h('th', { scope: 'col' }, 'Word'), h('th', { scope: 'col', class: 'num' }, 'Weight'), h('th', { scope: 'col' }, 'Modifier'), h('th', { scope: 'col', class: 'num' }, 'Counts as'))),
        h('tbody', null, kws.map((k) => h('tr', null,
          h('td', null, h('span', { class: 'kw' }, k.word)),
          h('td', { class: 'num' }, String(k.weight)),
          h('td', null, k.modifier === 'amplified' ? h('span', { class: 'mod mod-amp' }, 'Amplified ×1.5')
            : k.modifier === 'negated' ? h('span', { class: 'mod mod-neg' }, 'Negated ×0.3')
            : k.modifier === 'amplified+negated' ? h('span', { class: 'mod mod-neg' }, 'Amplified ×1.5, negated ×0.3')
            : h('span', { class: 'muted' }, 'None')),
          h('td', { class: 'num' }, fmtDec(k.applied, 1)))))))
      : h('p', { class: 'muted small' }, 'Without any severity keywords the score stays at the minimum of 1. Words like “burst”, “sparking”, “sewage” or “accident” carry weight.'),
    kws.length ? h('p', { class: 'muted small' },
      `Score = average of the weights${a.hit_bonus ? ` + ${fmtDec(a.hit_bonus, 1)} bonus for ${kws.length} matches` : ''}, rounded and kept between 1 and 10.`) : null,

    h('h3', { class: 'subhead' }, 'Category'),
    h('p', { class: 'cat-line' },
      a.category && a.category !== 'General'
        ? ['Suggested: ', h('strong', null, a.category)]
        : 'No category keywords yet: choose a category from the list.',
      chosenCategory && chosenCategory !== a.category ? h('span', { class: 'muted' }, ` You chose ${finalCat}.`) : null),
    scoreRows.some((r) => r.value)
      ? rankedBars(scoreRows, { unit: 'keyword matches', color: 'var(--series-1)' })
      : null);
}
