// Hand-built SVG/HTML charts. Marks follow the dataviz spec: thin marks, 4px
// rounded data-ends square at the baseline, 2px lines, hairline grid, legends for
// two or more series, tooltips that never gate (every chart has a table view).

import { h, s, clear, bindTip, tipRows, showTip, hideTip, fmtNum } from './dom.js';

// ── Shared pieces ──────────────────────────────────────────

export function dataTable(columns, rows, summary = 'Show data as a table') {
  return h('details', { class: 'data-table' },
    h('summary', null, summary),
    h('div', { class: 'table-scroll' },
      h('table', { class: 'table table-compact' },
        h('thead', null, h('tr', null, columns.map((c) => h('th', { scope: 'col', class: c.num ? 'num' : null }, c.label)))),
        h('tbody', null, rows.map((r) => h('tr', null, columns.map((c) => h('td', { class: c.num ? 'num' : null }, c.get(r)))))))));
}

export function emptyChart(message = 'No data yet.') {
  return h('div', { class: 'chart-empty' }, message);
}

function niceMax(max, ticks = 4) {
  if (!max || max <= 0) return { max: 1, step: 1 };
  const raw = max / ticks;
  const pow = Math.pow(10, Math.floor(Math.log10(raw)));
  const candidates = [1, 2, 2.5, 5, 10].map((m) => m * pow);
  let step = candidates.find((c) => c >= raw) || raw;
  if (step < 1) step = 1;
  return { max: Math.ceil(max / step) * step, step };
}

// Re-render an SVG whenever the container width changes.
function responsive(draw, minHeight) {
  const host = h('div', { class: 'chart-host', style: { minHeight: `${minHeight}px` } });
  let lastW = 0;
  const render = () => {
    const w = Math.round(host.clientWidth);
    if (!w || w === lastW) return;
    lastW = w;
    clear(host);
    host.appendChild(draw(w));
  };
  if ('ResizeObserver' in window) {
    const ro = new ResizeObserver(() => render());
    ro.observe(host);
  } else {
    window.addEventListener('resize', render);
  }
  requestAnimationFrame(render);
  return host;
}

// Rounded data-end (top) with a square baseline, as an SVG path.
function columnPath(x, y, w, hgt, r = 4) {
  if (hgt <= 0) return '';
  const rr = Math.min(r, w / 2, hgt);
  return `M${x},${y + hgt} L${x},${y + rr} Q${x},${y} ${x + rr},${y} L${x + w - rr},${y} Q${x + w},${y} ${x + w},${y + rr} L${x + w},${y + hgt} Z`;
}

// ── Stat tile ──────────────────────────────────────────────

export function statTile({ label, value, sub, tone }) {
  return h('div', { class: `stat${tone ? ` stat-${tone}` : ''}` },
    h('div', { class: 'stat-label' }, label),
    h('div', { class: 'stat-value' }, value),
    sub ? h('div', { class: 'stat-sub' }, sub) : null);
}

// ── Tall segmented bar (one whole split into ≤ 6 parts) ─────

export function segmentedBar(parts, { ariaLabel = 'Distribution', format = fmtNum } = {}) {
  const total = parts.reduce((a, p) => a + (p.value || 0), 0);
  if (!total) return emptyChart();
  const bar = h('div', { class: 'segbar', role: 'group', 'aria-label': ariaLabel });
  const blocks = [];
  for (const p of parts) {
    if (!p.value) continue;
    const share = p.value / total;
    const label = h('span', { class: 'segbar-label', style: { color: p.ink || '#fff' } }, `${Math.round(share * 100)}%`);
    const block = h('div', {
      class: 'segbar-part',
      tabindex: '0',
      style: { flexGrow: String(p.value), background: p.color },
      'aria-label': `${p.name}: ${format(p.value)} (${Math.round(share * 100)}%)`,
    }, label);
    bindTip(block, () => tipRows(p.name, [{ value: format(p.value), label: 'complaints' }, { value: `${(share * 100).toFixed(1)}%`, label: 'of total' }]));
    blocks.push({ block, label });
    bar.appendChild(block);
  }
  const fit = () => {
    for (const { block, label } of blocks) {
      label.style.visibility = 'visible';
      if (label.offsetWidth > block.clientWidth - 12) label.style.visibility = 'hidden';
    }
  };
  if ('ResizeObserver' in window) new ResizeObserver(fit).observe(bar);
  requestAnimationFrame(fit);
  const legend = h('ul', { class: 'legend' }, parts.map((p) => h('li', null,
    h('span', { class: 'legend-swatch', style: { background: p.color }, 'aria-hidden': 'true' }), p.name)));
  return h('div', { class: 'segbar-wrap' }, bar, legend,
    dataTable([
      { label: 'Part', get: (p) => p.name },
      { label: 'Count', num: true, get: (p) => format(p.value) },
      { label: 'Share', num: true, get: (p) => `${((p.value / total) * 100).toFixed(1)}%` },
    ], parts));
}

// ── Ranked horizontal bars (HTML) ──────────────────────────

export function rankedBars(rows, { format = fmtNum, detail, color = 'var(--series-1)', max, onSelect, unit = 'complaints' } = {}) {
  if (!rows.length || rows.every((r) => !r.value)) return emptyChart();
  const top = max || Math.max(...rows.map((r) => r.value));
  const list = h('ol', { class: 'rbars' });
  for (const r of rows) {
    const pct = top ? (r.value / top) * 100 : 0;
    const item = h('li', { class: 'rbar', tabindex: '0' },
      h('span', { class: 'rbar-label' }, r.label),
      h('span', { class: 'rbar-track' },
        h('span', { class: 'rbar-fill', style: { '--p': String(Math.max(pct, r.value ? 1.5 : 0) / 100), background: r.color || color } }),
        h('span', { class: 'rbar-value' }, format(r.value))));
    bindTip(item, () => tipRows(r.label, [{ value: format(r.value), label: unit }, ...(detail ? detail(r) : [])]));
    if (onSelect) {
      item.classList.add('rbar-action');
      item.addEventListener('click', () => onSelect(r));
      item.addEventListener('keydown', (e) => { if (e.key === 'Enter') onSelect(r); });
    }
    list.appendChild(item);
  }
  return list;
}

// ── Column chart (single series) ───────────────────────────

export function columnChart(data, { height = 220, format = fmtNum, xTitle, unit = 'complaints', color = 'var(--series-1)' } = {}) {
  if (!data.length || data.every((d) => !d.value)) return emptyChart();
  const chart = responsive((width) => {
    const m = { t: 22, r: 8, b: xTitle ? 44 : 28, l: 40 };
    const iw = width - m.l - m.r;
    const ih = height - m.t - m.b;
    const { max, step } = niceMax(Math.max(...data.map((d) => d.value)));
    const y = (v) => m.t + ih - (v / max) * ih;
    const slot = iw / data.length;
    const bw = Math.min(24, slot * 0.6);
    const svg = s('svg', { width, height, viewBox: `0 0 ${width} ${height}`, class: 'chart-svg', role: 'img', 'aria-label': 'Column chart' });
    for (let v = 0; v <= max + 1e-9; v += step) {
      svg.appendChild(s('line', { x1: m.l, x2: width - m.r, y1: y(v), y2: y(v), class: v === 0 ? 'axis-line' : 'grid-line' }));
      svg.appendChild(s('text', { x: m.l - 8, y: y(v) + 4, class: 'axis-text', 'text-anchor': 'end' }, fmtNum(v)));
    }
    const peak = data.reduce((a, d) => (d.value > a.value ? d : a), data[0]);
    data.forEach((d, i) => {
      const cx = m.l + slot * i + slot / 2;
      const top = y(d.value);
      const g = s('g', { class: 'col-g', tabindex: '0', 'aria-label': `${d.label}: ${format(d.value)}` });
      g.appendChild(s('rect', { x: m.l + slot * i, y: m.t, width: slot, height: ih, class: 'hit' }));
      g.appendChild(s('path', { d: columnPath(cx - bw / 2, top, bw, m.t + ih - top), style: { fill: d.color || color }, class: 'col-mark' }));
      g.appendChild(s('text', { x: cx, y: height - m.b + 16, class: 'axis-text', 'text-anchor': 'middle' }, d.short || d.label));
      if (d === peak && d.value) {
        g.appendChild(s('text', { x: cx, y: top - 6, class: 'value-text', 'text-anchor': 'middle' }, format(d.value)));
      }
      bindTip(g, () => tipRows(d.title || d.label, [{ value: format(d.value), label: unit }]));
      svg.appendChild(g);
    });
    if (xTitle) svg.appendChild(s('text', { x: m.l + iw / 2, y: height - 6, class: 'axis-title', 'text-anchor': 'middle' }, xTitle));
    return svg;
  }, height);
  return h('div', { class: 'chart' }, chart,
    dataTable([{ label: 'Value', get: (d) => d.title || d.label }, { label: 'Count', num: true, get: (d) => format(d.value) }], data));
}

// ── Line chart (≥ 1 series, crosshair tooltip) ─────────────

export function lineChart(series, labels, { height = 240, format = fmtNum, labelFormat = (x) => x } = {}) {
  if (!labels.length || series.every((sr) => sr.values.every((v) => !v))) return emptyChart();
  const legend = series.length > 1
    ? h('ul', { class: 'legend' }, series.map((sr) => h('li', null,
      h('span', { class: 'legend-line', style: { background: sr.color }, 'aria-hidden': 'true' }), sr.name)))
    : null;

  const chart = responsive((width) => {
    const m = { t: 14, r: 84, b: 30, l: 40 };
    const iw = width - m.l - m.r;
    const ih = height - m.t - m.b;
    const all = series.flatMap((sr) => sr.values);
    const { max, step } = niceMax(Math.max(...all));
    const n = labels.length;
    const x = (i) => m.l + (n === 1 ? iw / 2 : (iw * i) / (n - 1));
    const y = (v) => m.t + ih - (v / max) * ih;
    const svg = s('svg', { width, height, viewBox: `0 0 ${width} ${height}`, class: 'chart-svg', role: 'img', 'aria-label': series.map((sr) => sr.name).join(' and ') });
    for (let v = 0; v <= max + 1e-9; v += step) {
      svg.appendChild(s('line', { x1: m.l, x2: m.l + iw, y1: y(v), y2: y(v), class: v === 0 ? 'axis-line' : 'grid-line' }));
      svg.appendChild(s('text', { x: m.l - 8, y: y(v) + 4, class: 'axis-text', 'text-anchor': 'end' }, fmtNum(v)));
    }
    const every = Math.max(1, Math.ceil(n / Math.max(2, Math.floor(iw / 58))));
    labels.forEach((lab, i) => {
      const isLast = i === n - 1;
      const onStep = i % every === 0;
      if (!(isLast || (onStep && n - 1 - i >= Math.ceil(every * 0.6)))) return;
      svg.appendChild(s('text', { x: x(i), y: height - 8, class: 'axis-text', 'text-anchor': 'middle' }, labelFormat(lab)));
    });
    // Areas only for a single series; lines always.
    for (const sr of series) {
      const pts = sr.values.map((v, i) => `${x(i)},${y(v)}`).join(' ');
      if (series.length === 1) {
        svg.appendChild(s('polygon', { points: `${x(0)},${y(0)} ${pts} ${x(n - 1)},${y(0)}`, style: { fill: sr.color, opacity: '0.1' } }));
      }
      svg.appendChild(s('polyline', { points: pts, class: 'line-mark', style: { stroke: sr.color } }));
    }
    // End labels when they don't collide; legend always carries identity.
    const ends = series.map((sr) => ({ sr, v: sr.values[n - 1], yy: y(sr.values[n - 1]) }));
    const collide = ends.some((a, i) => ends.some((b, j) => i < j && Math.abs(a.yy - b.yy) < 16));
    for (const e of ends) {
      svg.appendChild(s('circle', { cx: x(n - 1), cy: e.yy, r: 4, class: 'dot-mark', style: { fill: e.sr.color } }));
      if (!collide) {
        svg.appendChild(s('text', { x: x(n - 1) + 10, y: e.yy + 4, class: 'value-text' }, `${format(e.v)} ${e.sr.short || ''}`.trim()));
      }
    }
    // Crosshair layer.
    const cross = s('line', { y1: m.t, y2: m.t + ih, class: 'crosshair', visibility: 'hidden' });
    const dots = series.map((sr) => s('circle', { r: 4, class: 'dot-mark', style: { fill: sr.color }, visibility: 'hidden' }));
    svg.appendChild(cross);
    dots.forEach((d) => svg.appendChild(d));
    const overlay = s('rect', { x: m.l, y: m.t, width: iw, height: ih, class: 'hit', tabindex: '0', 'aria-label': 'Chart values. Use the left and right arrow keys to move between points.' });
    let idx = n - 1;
    const showAt = (i, cx, cy) => {
      idx = Math.max(0, Math.min(n - 1, i));
      cross.setAttribute('x1', x(idx));
      cross.setAttribute('x2', x(idx));
      cross.setAttribute('visibility', 'visible');
      dots.forEach((d, k) => {
        d.setAttribute('cx', x(idx));
        d.setAttribute('cy', y(series[k].values[idx]));
        d.setAttribute('visibility', 'visible');
      });
      showTip(tipRows(labelFormat(labels[idx]), series.map((sr) => ({ value: format(sr.values[idx]), label: sr.name, key: sr.color }))), cx, cy);
    };
    const hide = () => {
      cross.setAttribute('visibility', 'hidden');
      dots.forEach((d) => d.setAttribute('visibility', 'hidden'));
      hideTip();
    };
    overlay.addEventListener('pointermove', (e) => {
      const r = svg.getBoundingClientRect();
      const px = e.clientX - r.left;
      const i = n === 1 ? 0 : Math.round(((px - m.l) / iw) * (n - 1));
      showAt(i, e.clientX, e.clientY);
    });
    overlay.addEventListener('pointerleave', hide);
    overlay.addEventListener('blur', hide);
    const keyShow = () => {
      const r = svg.getBoundingClientRect();
      showAt(idx, r.left + x(idx), r.top + m.t + 10);
    };
    overlay.addEventListener('focus', keyShow);
    overlay.addEventListener('keydown', (e) => {
      if (e.key === 'ArrowLeft') { idx -= 1; keyShow(); e.preventDefault(); }
      if (e.key === 'ArrowRight') { idx += 1; keyShow(); e.preventDefault(); }
    });
    svg.appendChild(overlay);
    return svg;
  }, height);

  return h('div', { class: 'chart' }, legend, chart,
    dataTable([{ label: 'Period', get: (r) => labelFormat(r.label) }, ...series.map((sr, k) => ({ label: sr.name, num: true, get: (r) => format(r.values[k]) }))],
      labels.map((label, i) => ({ label, values: series.map((sr) => sr.values[i]) }))));
}

// ── Severity meter (10 cells) ──────────────────────────────

export function severityMeter(sev) {
  const cells = [];
  for (let i = 1; i <= 10; i++) {
    cells.push(h('span', { class: `meter-cell${i <= sev ? ` on sev-${keyFor(sev)}` : ''}`, 'aria-hidden': 'true' }));
  }
  return h('div', { class: 'meter', role: 'meter', 'aria-valuemin': '1', 'aria-valuemax': '10', 'aria-valuenow': String(sev), 'aria-label': `Severity ${sev} of 10` }, cells);
}
function keyFor(n) {
  if (n >= 9) return 'critical';
  if (n >= 7) return 'high';
  if (n >= 5) return 'medium';
  if (n >= 3) return 'low';
  return 'minor';
}

// ── 0–1 score bar (used by the duplicate breakdown) ────────

export function scoreBar(value, { marker } = {}) {
  const v = Math.max(0, Math.min(1, Number(value) || 0));
  return h('span', { class: 'score' },
    h('span', { class: 'score-fill', style: { width: `${v * 100}%` } }),
    marker !== undefined ? h('span', { class: 'score-marker', style: { left: `${marker * 100}%` }, title: `Threshold ${marker}` }) : null);
}
