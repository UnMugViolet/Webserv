<?php

// Handle theme switching
if (isset($_GET['theme'])) {
    $theme = ($_GET['theme'] === 'yellow') ? 'yellow' : 'pink';
    setcookie('theme', $theme, time() + 3600, "/");
    $_COOKIE['theme'] = $theme;
}

// Determine current theme
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
            border: 1px solid #ccc;
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
        <a href="?theme=pink" class="<?= $theme === 'pink' ? 'active' : '' ?>">Pink Theme</a>
        <a href="?theme=yellow" class="<?= $theme === 'yellow' ? 'active' : '' ?>">Yellow Theme</a>
    </div>
    <p>Current theme: <strong><?= htmlspecialchars($theme) ?></strong></p>
</body>
</html>
