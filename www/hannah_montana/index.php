<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Hannah Montana Hardcore Gang</title>
	<link rel="stylesheet" href="./css/style.css">
	<link rel="icon" href="favicon.ico">
	<script src="./js/script.js" defer></script>
	<link rel="preconnect" href="https://fonts.googleapis.com">
	<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
	<link href="https://fonts.googleapis.com/css2?family=Damion&family=Mr+Dafoe&display=swap" rel="stylesheet">

</head>

<?php
	$project_name = "Hannah Montana Hardcore Gang";
	$small_desc = "Welcome to the Hannah Montana Hardcore Gang!";
	$big_desc = "This is a fun and interactive page dedicated to all things Hannah Montana. Explore, enjoy, and don't forget to sing along!";

	$available_features = [
		"Get",
		"Post",
		"Delete",
		"Upload",
		"Download",
		"FileTransfer",
		"List directory",
		"Redirect",
		"Errors",
		"Aliases",
	]
?>

<body>
	<?php echo "<h1>$project_name</h1>"; ?>
	<?php echo "<p>$small_desc</p>"; ?>

	<h2>Join the Gang</h2>
	<div class="c">
		<img src="./assets/img/hannah_montana.jpg" alt="Hannah Montana" class="hannah-image">
		<div>
			<?php echo "<p>$big_desc</p>"; ?>
			<button class="play-button" onclick="playAudio('./assets/music/theme_song.mp3')">Play Song ►</button>
		</div>
	</div>
	<div>
		<h2 class="mt-5">Available Features</h2>
		<div class="feature-image-container">
			<?php
				foreach ($available_features as $feature) {
					$page_url = strtolower(str_replace(" ", "-", $feature));
					echo "<a href=\"pages/$page_url.php\" class=\"feature-card\">$feature</a>"; // TODO - must be reachable without the .php as well
				}
			?>
		</div>
	</div>
</body>
</html>
