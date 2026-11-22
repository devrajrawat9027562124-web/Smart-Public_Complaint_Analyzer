// Schematic Dehradun road graph (the same 18-node graph the C engine runs
// Dijkstra on). Layout x/y come from /api/graph; they are not GPS positions.

import { h, s, clear, bindTip, tipRows, fmtNum, prefersReducedMotion } from './dom.js';

const RAMP = ['var(--seq-1)', 'var(--seq-2)', 'var(--seq-3)', 'var(--seq-4)', 'var(--seq-5)'];
const RAMP_INK = ['var(--seq-ink-1)', 'var(--seq-ink-2)', 'var(--seq-ink-3)', 'var(--seq-ink-4)', 'var(--seq-ink-5)'];

export function createCityMap(graph, opts = {}) {
  const {
    counts = null,            // { [locationId]: number }
    countLabel = 'complaints',
    onNodeClick = null,
    selected = [],
    route = null,
    caption = true,
    ariaLabel = 'Schematic map of Dehradun locations and roads',
  } = opts;

  const W = graph.width || 1000;
  const H = graph.height || 640;
  const nodes = new Map(graph.nodes.map((n) => [n.id, n]));
  const neighbours = new Map(graph.nodes.map((n) => [n.id, []]));
  for (const e of graph.edges) {
    if (neighbours.has(e.a)) neighbours.get(e.a).push(e.b);
    if (neighbours.has(e.b)) neighbours.get(e.b).push(e.a);
  }

  const state = { counts, selected: new Set(selected), route };

  const svg = s('svg', { viewBox: `0 0 ${W} ${H}`, class: 'citymap-svg', role: 'group', 'aria-label': ariaLabel });
  const gEdges = s('g', { class: 'edges' });
  const gRoute = s('g', { class: 'route-layer' });
  const gNodes = s('g', { class: 'nodes' });
  svg.append(gEdges, gRoute, gNodes);

  // Edges and km labels.
  for (const e of graph.edges) {
    const a = nodes.get(e.a);
    const b = nodes.get(e.b);
    if (!a || !b) continue;
    gEdges.appendChild(s('line', { x1: a.x, y1: a.y, x2: b.x, y2: b.y, class: 'edge', 'data-a': e.a, 'data-b': e.b }));
    const mx = (a.x + b.x) / 2;
    const my = (a.y + b.y) / 2;
    gEdges.appendChild(s('text', { x: mx, y: my + 4, class: 'km-label', 'text-anchor': 'middle' }, `${e.km} km`));
  }

  function radiusFor(c, max) {
    if (!state.counts) return 9;
    if (!c) return 6;
    return 8 + 14 * Math.sqrt(c / Math.max(1, max));
  }
  function stepFor(c, max) {
    if (!c || !max) return -1;
    return Math.min(RAMP.length - 1, Math.floor((c / max) * RAMP.length - 1e-9));
  }

  // ── Label placement ──
  // Greedy placement: each location name tries 8 positions around its dot and
  // takes the one that overlaps least with names already placed, other dots,
  // roads and km labels. Busiest locations are placed first.
  let placedScale = 0;

  function currentScale() {
    return Number(svg.style.getPropertyValue('--u')) || Math.min(1.75, Math.max(1, W / 640));
  }

  function boxOverlap(a, b) {
    const w = Math.min(a.x2, b.x2) - Math.max(a.x1, b.x1);
    const hgt = Math.min(a.y2, b.y2) - Math.max(a.y1, b.y1);
    return w > 0 && hgt > 0 ? w * hgt : 0;
  }

  // Does segment p→q pass through box b? (Liang–Barsky clipping)
  function segmentHitsBox(p, q, b) {
    let t0 = 0;
    let t1 = 1;
    const dx = q.x - p.x;
    const dy = q.y - p.y;
    const tests = [[-dx, p.x - b.x1], [dx, b.x2 - p.x], [-dy, p.y - b.y1], [dy, b.y2 - p.y]];
    for (const [pp, qq] of tests) {
      if (pp === 0) { if (qq < 0) return false; continue; }
      const t = qq / pp;
      if (pp < 0) { if (t > t1) return false; if (t > t0) t0 = t; }
      else { if (t < t0) return false; if (t < t1) t1 = t; }
    }
    return true;
  }

  function placeLabels(radius) {
    const u = currentScale();
    placedScale = u;
    const fh = 12 * u;              // label font size in map units
    const cw = fh * 0.56;           // average glyph width (sign face, semibold)
    const kmH = 10.5 * u;
    const obstacles = [];
    for (const e of graph.edges) {
      const a = nodes.get(e.a);
      const b = nodes.get(e.b);
      if (!a || !b) continue;
      const mx = (a.x + b.x) / 2;
      const my = (a.y + b.y) / 2;
      const kw = `${e.km} km`.length * kmH * 0.55;
      obstacles.push({ x1: mx - kw / 2, x2: mx + kw / 2, y1: my - kmH * 0.5, y2: my + kmH * 0.5, w: 1.5 });
    }
    for (const n of graph.nodes) {
      // Keep clear of every dot, with a margin so a name never reads as
      // belonging to a neighbouring location.
      const r = radius.get(n.id) + 3;
      const m = r + 14 * u;
      obstacles.push({ id: n.id, x1: n.x - r, x2: n.x + r, y1: n.y - r, y2: n.y + r, w: 6 });
      obstacles.push({ id: n.id, x1: n.x - m, x2: n.x + m, y1: n.y - m, y2: n.y + m, w: 2 });
    }

    const order = [...graph.nodes].sort((a, b) =>
      (neighbours.get(b.id) || []).length - (neighbours.get(a.id) || []).length || a.id - b.id);
    const placed = [];
    const out = new Map();
    for (const n of order) {
      const r = radius.get(n.id);
      const tw = n.name.length * cw;
      const g = 6;
      const d = r * 0.72 + g;
      const cands = [
        { x: n.x + r + g, y: n.y + fh * 0.35, a: 'start' },          // E
        { x: n.x - r - g, y: n.y + fh * 0.35, a: 'end' },            // W
        { x: n.x, y: n.y - r - g, a: 'middle' },                     // N
        { x: n.x, y: n.y + r + g + fh * 0.8, a: 'middle' },          // S
        { x: n.x + d, y: n.y - d, a: 'start' },                      // NE
        { x: n.x - d, y: n.y - d, a: 'end' },                        // NW
        { x: n.x + d, y: n.y + d + fh * 0.6, a: 'start' },           // SE
        { x: n.x - d, y: n.y + d + fh * 0.6, a: 'end' },             // SW
      ];
      let best = null;
      cands.forEach((c, i) => {
        const x1 = c.a === 'start' ? c.x : c.a === 'end' ? c.x - tw : c.x - tw / 2;
        const box = { x1, x2: x1 + tw, y1: c.y - fh * 0.8, y2: c.y + fh * 0.22 };
        let cost = i * 15;
        const over = Math.max(0, -box.x1 + 2) + Math.max(0, box.x2 - W + 2) +
                     Math.max(0, -box.y1 + 2) + Math.max(0, box.y2 - H + 2);
        cost += over * 400;
        for (const p of placed) cost += boxOverlap(box, p) * 40;
        for (const o of obstacles) if (o.id !== n.id) cost += boxOverlap(box, o) * o.w;
        for (const e of graph.edges) {
          const a = nodes.get(e.a);
          const b = nodes.get(e.b);
          if (a && b && segmentHitsBox(a, b, box)) cost += 900;
        }
        if (!best || cost < best.cost) best = { cost, c, box };
      });
      placed.push(best.box);
      out.set(n.id, best.c);
    }
    return out;
  }

  function drawNodes() {
    clear(gNodes);
    const vals = state.counts ? Object.values(state.counts).map(Number) : [];
    const max = vals.length ? Math.max(...vals) : 0;
    const radius = new Map(graph.nodes.map((n) =>
      [n.id, radiusFor(state.counts ? Number(state.counts[n.id] || 0) : null, max)]));
    const labels = placeLabels(radius);
    for (const n of graph.nodes) {
      const c = state.counts ? Number(state.counts[n.id] || 0) : null;
      const r = radius.get(n.id);
      const step = c === null ? -1 : stepFor(c, max);
      const { x: lx, y: ly, a: anchor } = labels.get(n.id);

      const isSel = state.selected.has(n.id);
      const g = s('g', {
        class: `node${onNodeClick ? ' node-click' : ''}${isSel ? ' node-selected' : ''}`,
        transform: `translate(0,0)`,
        tabindex: '0',
        role: onNodeClick ? 'button' : 'img',
        'aria-label': `${n.name}${c !== null ? `: ${fmtNum(c)} ${countLabel}` : ''}`,
      });
      g.appendChild(s('circle', { cx: n.x, cy: n.y, r: Math.max(r + 10, 18), class: 'hit' }));
      if (isSel) g.appendChild(s('circle', { cx: n.x, cy: n.y, r: r + 6, class: 'node-ring' }));
      g.appendChild(s('circle', {
        cx: n.x, cy: n.y, r,
        class: `node-dot${step < 0 ? ' node-dot-empty' : ''}`,
        style: step >= 0 ? { fill: RAMP[step] } : null,
      }));
      if (c && r >= 15) {
        g.appendChild(s('text', { x: n.x, y: n.y + 4.5, class: 'node-count', 'text-anchor': 'middle', style: { fill: RAMP_INK[step] } }, fmtNum(c)));
      }
      g.appendChild(s('text', { x: lx, y: ly, class: 'node-label', 'text-anchor': anchor }, n.name));
      bindTip(g, () => tipRows(n.name, c !== null ? [{ value: fmtNum(c), label: countLabel }] : [{ value: `#${n.id}`, label: 'location id' }]));
      if (onNodeClick) {
        g.addEventListener('click', () => onNodeClick(n.id));
        g.addEventListener('keydown', (e) => {
          if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); onNodeClick(n.id); }
        });
      }
      gNodes.appendChild(g);
    }
  }

  function drawRoute() {
    clear(gRoute);
    for (const el of gEdges.querySelectorAll('.edge-on')) el.classList.remove('edge-on');
    const r = state.route;
    if (!r || !Array.isArray(r.path) || r.path.length === 0) return;
    const pts = r.path.map((id) => nodes.get(id)).filter(Boolean);
    if (pts.length === 1) {
      gRoute.appendChild(s('circle', { cx: pts[0].x, cy: pts[0].y, r: 20, class: 'route-here' }));
      return;
    }
    const d = pts.map((p, i) => `${i ? 'L' : 'M'}${p.x},${p.y}`).join(' ');
    const casing = s('path', { d, class: 'route-casing' });
    const line = s('path', { d, class: 'route-line' });
    gRoute.append(casing, line);
    const start = pts[0];
    const end = pts[pts.length - 1];
    gRoute.appendChild(s('circle', { cx: start.x, cy: start.y, r: 7, class: 'route-start' }));
    gRoute.appendChild(s('circle', { cx: end.x, cy: end.y, r: 9, class: 'route-end' }));
    if (!prefersReducedMotion()) {
      requestAnimationFrame(() => {
        try {
          const total = line.getTotalLength();
          line.style.strokeDasharray = `${total}`;
          line.style.strokeDashoffset = `${total}`;
          line.getBoundingClientRect();
          line.style.transition = 'stroke-dashoffset 900ms cubic-bezier(.3,.7,.2,1)';
          line.style.strokeDashoffset = '0';
        } catch { /* not rendered yet */ }
      });
    }
  }

  drawNodes();
  drawRoute();

  const legend = state.counts !== null
    ? h('div', { class: 'map-legend' },
      h('span', null, `Circle size and shade: ${countLabel}`),
      h('span', { class: 'map-ramp', 'aria-hidden': 'true' }, RAMP.map((c) => h('span', { style: { background: c } }))),
      h('span', { class: 'map-ramp-ends' }, 'fewer to more'))
    : null;

  // Keep label text near a readable on-screen size whatever the map's width.
  const frame = h('div', { class: 'citymap-frame' }, svg);
  const rescale = () => {
    const w = svg.getBoundingClientRect().width || frame.clientWidth;
    if (!w) return;
    const u = Math.min(1.75, Math.max(1, W / w));
    svg.style.setProperty('--u', String(u));
    // Label sizes changed noticeably: place the names again.
    if (Math.abs(u - placedScale) > 0.08) drawNodes();
  };
  if ('ResizeObserver' in window) new ResizeObserver(rescale).observe(frame);
  requestAnimationFrame(rescale);

  const el = h('figure', { class: 'citymap' },
    frame,
    caption ? h('figcaption', { class: 'citymap-caption' },
      'Schematic, not to scale. Labels on roads are distances in km from the engine’s road graph.') : null,
    legend);

  return {
    el,
    setRoute(route) { state.route = route; drawRoute(); },
    setCounts(c) { state.counts = c; drawNodes(); },
    setSelected(ids) { state.selected = new Set(ids); drawNodes(); },
  };
}

export function routeSteps(route, graph) {
  if (!route || !route.path || !route.path.length) return null;
  const name = (id) => (graph.nodes.find((n) => n.id === id) || {}).name || `#${id}`;
  if (route.path.length === 1) {
    return h('p', { class: 'muted' }, `Already at ${name(route.path[0])}. No travel needed.`);
  }
  return h('div', { class: 'route-steps' },
    h('p', { class: 'route-total' },
      h('strong', null, `${route.distance} km`),
      ` from ${name(route.path[0])} to ${name(route.path[route.path.length - 1])}, ${route.path.length - 1} road segment${route.path.length > 2 ? 's' : ''}`),
    h('ol', { class: 'steps' }, (route.steps || []).map((st) => h('li', null,
      h('span', null, `${name(st.from)} to ${name(st.to)}`),
      h('span', { class: 'steps-km' }, `${st.km} km`)))));
}
