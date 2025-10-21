<?php

// Prefer server-provided THEME env (set by RequestHandler) if available
$env_theme = getenv('THEME');
if ($env_theme !== false && $env_theme !== '') {
    $theme = ($env_theme === 'yellow') ? 'yellow' : 'pink';
    // Keep cookie in sync for subsequent non-CGI requests
    setcookie('theme', $theme, time() + 3600, "/");
    $_COOKIE['theme'] = $theme;
}
// Otherwise handle explicit ?theme=... from the client
else if (isset($_GET['theme'])) {
    $theme = ($_GET['theme'] === 'yellow') ? 'yellow' : 'pink';
    setcookie('theme', $theme, time() + 3600, "/");
    $_COOKIE['theme'] = $theme;
}

// Determine current theme (cookie first)
$theme = isset($_COOKIE['theme']) ? $_COOKIE['theme'] : 'pink';
$bg = $theme === 'yellow' ? '#ffe066' : '#c71585';
$color = $theme === 'yellow' ? '#b8860b' : '#c71585';
?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Theme Switcher</title>
	<link rel="stylesheet" href="../css/style.css">

    <style>
        body {
            background: <?= $bg ?>;
            color: <?= $color ?>;
            transition: background 0.3s, color 0.3s;
        }
        .switcher {
			width: 100%;
			height: 75vh;
			display: flex;
			justify-content: center;
			align-items: center;
            margin: 2em;
        }
        .switcher a {
            padding: 0.5em 1em;
            margin: 0 0.5em;
            border-radius: 5px;
            text-decoration: none;
            color: #222;
            background: #fffbe7;
            border: 3px solid #ccc;
        }
        .switcher a.active {
            font-weight: bold;
            border: 2px solid #222;
        }
    </style>
</head>
<body>
    <a href="/" class="go-back-button">Go Back</a>
    <h1>Theme Switcher</h1>
    <div class="switcher">
        <a href="?theme=pink" class="theme-link" data-theme="pink">Pink Theme</a>
        <a href="?theme=yellow" class="theme-link" data-theme="yellow">Yellow Theme</a>
    </div>
    <p>Current theme: <strong id="current-theme"><?= htmlspecialchars($theme) ?></strong></p>
	<script>
		document.addEventListener('DOMContentLoaded', function() {
  var currentEl = document.getElementById('current-theme');
  var links = document.querySelectorAll('.theme-link');

  // apply active class on load according to server/cookie-rendered theme
  var pageTheme = currentEl ? currentEl.textContent.trim() : '';
  links.forEach(function(a) {
    if (a.dataset.theme === pageTheme) a.classList.add('active');
  });

  links.forEach(function(a) {
    a.addEventListener('click', function(e) {
      e.preventDefault();
      var theme = this.dataset.theme;
      // set cookie
      document.cookie = "theme=" + theme + "; path=/; max-age=3600; SameSite=Lax";
      // update styles
      document.body.style.background = (theme === 'yellow') ? '#ffe066' : '#c71585';
      document.body.style.color = (theme === 'yellow') ? '#b8860b' : '#c71585';
      // update active link
      links.forEach(function(x){ x.classList.remove('active'); });
      this.classList.add('active');
      // update displayed current theme
      if (currentEl) currentEl.textContent = theme;
    });
  });
});
	</script>
</body>
</html>

