// Runs before first paint (classic script, not a module) so a saved theme
// choice applies without a flash. Kept out of index.html for the server's CSP.
try {
  var savedTheme = localStorage.getItem('spca-theme');
  if (savedTheme === 'light' || savedTheme === 'dark') {
    document.documentElement.setAttribute('data-theme', savedTheme);
  }
} catch (e) { /* storage unavailable: follow the system theme */ }
