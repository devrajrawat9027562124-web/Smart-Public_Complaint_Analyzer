import { h, mount, toast, ROLE_LABEL } from '../dom.js';
import { api } from '../api.js';
import { logo, setUser, navigate, defaultRoute, canOpen, store } from '../app.js';
import { field, withBusy } from '../ui.js';

function authFrame(title, sub, form, aside) {
  return h('div', { class: 'auth' },
    h('header', { class: 'landing-bar' }, h('a', { href: '#/', class: 'landing-brand' }, logo())),
    h('main', { id: 'main', class: 'auth-main', tabindex: '-1' },
      h('div', { class: 'auth-card' },
        h('h1', { class: 'auth-title' }, title),
        sub ? h('p', { class: 'auth-sub' }, sub) : null,
        form),
      aside || null));
}

function errorBox() {
  return h('p', { class: 'form-error', role: 'alert', hidden: true });
}
function showError(box, msg) {
  box.textContent = msg;
  box.hidden = false;
}

function safeNext(query, user) {
  const next = query.get('next');
  if (next && /^#\/[a-z]/.test(next) && !/^#\/(login|signup)/.test(next) && canOpen(next, user)) return next;
  return defaultRoute(user);
}

export async function loginView(ctx) {
  if (store.user) { navigate(defaultRoute(store.user)); return; }
  const username = h('input', { class: 'input', name: 'username', autocomplete: 'username', required: true, autocapitalize: 'none', spellcheck: 'false' });
  const password = h('input', { class: 'input', name: 'password', type: 'password', autocomplete: 'current-password', required: true });
  const err = errorBox();
  const submit = h('button', { type: 'submit', class: 'btn btn-primary btn-block btn-lg' }, 'Sign in');
  const form = h('form', { class: 'form', novalidate: true },
    field('Username', username),
    field('Password', password),
    err,
    submit,
    h('p', { class: 'auth-switch' }, 'New here? ', h('a', { href: '#/signup' }, 'Create a citizen account')));

  form.addEventListener('submit', async (e) => {
    e.preventDefault();
    err.hidden = true;
    if (!username.value.trim() || !password.value) {
      showError(err, 'Enter your username and password.');
      return;
    }
    await withBusy(submit, 'Signing in…', async () => {
      try {
        const res = await api('/login', { method: 'POST', body: { username: username.value.trim(), password: password.value }, quiet401: true });
        setUser(res.user);
        toast(`Signed in as ${res.user.full_name || res.user.username}.`, 'success');
        navigate(safeNext(ctx.query, res.user));
      } catch (ex) {
        showError(err, ex.status === 401 ? 'That username and password do not match an account.' : ex.message);
        password.select();
      }
    });
  });

  const demoSlot = h('div', { class: 'demo-slot' });
  mount(ctx.root, authFrame('Sign in', 'Citizens, staff and field workers all sign in here. Your role decides what you see next.', form, demoSlot));
  username.focus();

  try {
    const sm = await api('/public/summary');
    const demos = Array.isArray(sm && sm.demo_accounts) ? sm.demo_accounts : [];
    if (!ctx.alive() || !demos.length) return;
    mount(demoSlot, h('section', { class: 'demo', 'aria-labelledby': 'demo-title' },
      h('h2', { id: 'demo-title', class: 'demo-title' }, 'Local demo accounts'),
      h('p', { class: 'demo-sub' }, 'Sample accounts from this installation’s data files. Selecting one fills the form; it does not sign you in.'),
      h('ul', { class: 'demo-list' }, demos.map((d) => h('li', null,
        h('button', {
          type: 'button',
          class: 'demo-btn',
          onclick: () => {
            username.value = d.username || '';
            password.value = d.password || '';
            submit.focus();
          },
        },
        h('strong', null, ROLE_LABEL[d.role] || d.role || 'Account'),
        d.label || d.full_name ? h('span', null, d.label || d.full_name) : null,
        h('code', null, d.username)))))));
  } catch { /* demo list is optional */ }
}

export async function signupView(ctx) {
  if (store.user) { navigate(defaultRoute(store.user)); return; }
  const username = h('input', { class: 'input', name: 'username', autocomplete: 'username', required: true, minlength: '3', maxlength: '30', autocapitalize: 'none', spellcheck: 'false', pattern: '[A-Za-z0-9_.\\-]{3,30}' });
  const password = h('input', { class: 'input', name: 'password', type: 'password', autocomplete: 'new-password', required: true, minlength: '4', maxlength: '49' });
  const fullName = h('input', { class: 'input', name: 'full_name', autocomplete: 'name', required: true, maxlength: '99' });
  const phone = h('input', { class: 'input', name: 'phone', type: 'tel', autocomplete: 'tel', inputmode: 'numeric', maxlength: '15' });
  const email = h('input', { class: 'input', name: 'email', type: 'email', autocomplete: 'email', maxlength: '99' });
  const err = errorBox();
  const submit = h('button', { type: 'submit', class: 'btn btn-primary btn-block btn-lg' }, 'Create account');
  const form = h('form', { class: 'form', novalidate: true },
    field('Full name', fullName),
    field('Username', username, { hint: '3 to 30 characters: letters, digits, underscore, dot or hyphen. No spaces.' }),
    field('Password', password, { hint: 'At least 4 characters. Do not reuse a password from another site: this demo stores passwords as plain text.' }),
    h('div', { class: 'field-row' },
      field('Phone (optional)', phone, { hint: '10 to 15 digits' }),
      field('Email (optional)', email)),
    err,
    submit,
    h('p', { class: 'auth-switch' }, 'Already registered? ', h('a', { href: '#/login' }, 'Sign in')));

  form.addEventListener('submit', async (e) => {
    e.preventDefault();
    err.hidden = true;
    const u = username.value.trim();
    const problems = [];
    if (!fullName.value.trim()) problems.push('Enter your full name.');
    if (!/^[A-Za-z0-9_.-]{3,30}$/.test(u)) problems.push('Choose a username of 3 to 30 letters, digits, underscores, dots or hyphens.');
    if (password.value.length < 4) problems.push('Use a password of at least 4 characters.');
    if (phone.value.trim() && !/^\d{10,15}$/.test(phone.value.trim())) problems.push('Enter the phone number as 10 to 15 digits, or leave it empty.');
    if (email.value.trim() && !/^[^\s@|]+@[^\s@|]+\.[^\s@|]+$/.test(email.value.trim())) problems.push('Enter a valid email address, or leave it empty.');
    if (problems.length) { showError(err, problems.join(' ')); return; }
    await withBusy(submit, 'Creating account…', async () => {
      try {
        const res = await api('/signup', {
          method: 'POST',
          body: { username: u, password: password.value, full_name: fullName.value.trim(), phone: phone.value.trim(), email: email.value.trim() },
          quiet401: true,
        });
        setUser(res.user);
        toast('Account created. You are signed in.', 'success');
        navigate('#/submit');
      } catch (ex) {
        showError(err, ex.message);
      }
    });
  });

  mount(ctx.root, authFrame('Create a citizen account', 'You need an account to report problems and follow their progress.', form));
  fullName.focus();
}
