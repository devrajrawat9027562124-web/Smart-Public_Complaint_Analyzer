import { h, mount, toast, fmtDate, ROLE_LABEL } from '../dom.js';
import { api, getGraph } from '../api.js';
import { pageHeader, panel, kv, locationSelect, withBusy, field } from '../ui.js';
import { createCityMap } from '../map.js';

export async function profileView(ctx) {
  const user = await ctx.refreshUser().catch(() => ctx.user);
  const graph = await getGraph();
  if (!ctx.alive()) return;

  const details = kv([
    ['Name', user.full_name || '–'],
    ['Username', user.username],
    ['Role', ROLE_LABEL[user.role] || user.role],
    user.role !== 'USER' ? ['Department', user.department || '–'] : null,
    ['Phone', user.phone || '–'],
    ['Email', user.email || '–'],
    ['Registered', fmtDate(user.registered_date)],
    user.role === 'WORKER' ? ['Current location', user.location] : null,
    user.role === 'WORKER' ? ['Availability', user.is_available ? 'Available for new tasks' : 'Busy (no new tasks); change it on My tasks'] : null,
  ]);

  let locationPanel = null;
  if (user.role === 'WORKER') {
    const sel = locationSelect(graph, user.location_id, { required: true });
    const save = h('button', { type: 'submit', class: 'btn btn-primary' }, 'Update my location');
    const map = createCityMap(graph, {
      selected: [user.location_id],
      caption: false,
      ariaLabel: 'Select your current location on the map',
      onNodeClick: (id) => { sel.value = String(id); map.setSelected([id]); },
    });
    sel.addEventListener('change', () => map.setSelected([Number(sel.value)]));
    const form = h('form', { class: 'form', onsubmit: async (e) => {
      e.preventDefault();
      if (Number(sel.value) === Number(user.location_id)) { toast('You are already at that location.', 'info'); return; }
      await withBusy(save, 'Saving…', async () => {
        try {
          const res = await api('/me/location', { method: 'POST', body: { location_id: Number(sel.value) } });
          toast(`Location updated to ${res.user.location}.`, 'success');
          await profileView(ctx);
        } catch (err) { toast(err.message, 'error'); }
      });
    } }, field('Where are you now?', sel, { hint: 'Routes to your tasks start from here.' }), save);
    locationPanel = panel('Update my location', [form, map.el]);
  }

  mount(ctx.root,
    pageHeader('Profile', 'Your account as stored in the data files.'),
    h('div', { class: user.role === 'WORKER' ? 'grid-2' : 'narrow' },
      panel('Account', details),
      locationPanel));
}
