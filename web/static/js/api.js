// Thin JSON client for the /api endpoints described in web/API.md.

export class ApiError extends Error {
  constructor(status, message) {
    super(message);
    this.status = status;
  }
}

let unauthorizedHandler = null;
export function onUnauthorized(fn) { unauthorizedHandler = fn; }

export async function api(path, { method = 'GET', body, signal, quiet401 = false } = {}) {
  let res;
  try {
    res = await fetch(`/api${path}`, {
      method,
      headers: body !== undefined ? { 'Content-Type': 'application/json' } : {},
      body: body !== undefined ? JSON.stringify(body) : undefined,
      credentials: 'same-origin',
      signal,
    });
  } catch (err) {
    if (err && err.name === 'AbortError') throw err;
    throw new ApiError(0, 'Cannot reach the server. Check that web/server.py is running.');
  }
  let data = null;
  const text = await res.text();
  if (text) {
    try { data = JSON.parse(text); } catch { data = null; }
  }
  if (!res.ok) {
    const message = (data && data.error) || `Request failed (${res.status})`;
    if (res.status === 401 && !quiet401 && unauthorizedHandler) unauthorizedHandler();
    throw new ApiError(res.status, message);
  }
  return data;
}

export function qs(params) {
  const parts = [];
  for (const [k, v] of Object.entries(params || {})) {
    if (v === undefined || v === null || v === '') continue;
    parts.push(`${encodeURIComponent(k)}=${encodeURIComponent(v)}`);
  }
  return parts.length ? `?${parts.join('&')}` : '';
}

// Small caches for data that rarely changes during a session.
let graphPromise = null;
export function getGraph() {
  if (!graphPromise) {
    graphPromise = api('/graph').catch((err) => { graphPromise = null; throw err; });
  }
  return graphPromise;
}

let deptPromise = null;
export function getDepartments(force = false) {
  if (force || !deptPromise) {
    deptPromise = api('/departments').catch((err) => { deptPromise = null; throw err; });
  }
  return deptPromise;
}

export function resetCaches() {
  deptPromise = null;
}
