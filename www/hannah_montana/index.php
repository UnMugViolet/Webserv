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
		["Post a comment with an attachment", "post"],
		["Display Posts with Attachments and Delete", "display-posts"],
		["Set and View Cookies", "cookies"],
		["List Directory Contents", ""],
		["Redirect internal", "redirect-internal"],
		["Redirect external", "redirect-external"],
		"Errors",
		["Infinite Loop (Warning: May crash your browser)", "infloop"]
	]
?>

<body onclick="playAudio('./assets/music/theme_song.mp3')">
	<?php echo "<h1>$project_name</h1>"; ?>
	<?php echo "<p>$small_desc</p>"; ?>

	<h2>Join the Gang</h2>
	<section class="content">
		<div class="dual-image-container-left">
			<img  src="./assets/img/hannah_montana.jpg" alt="Hannah Montana" class="hannah-image">
		</div>
		<div class="dual-image-container-right">
			<?php echo "<p>$big_desc</p>"; ?>
			<div class="audio-container">
				<button class="play-button" onclick="playAudio('./assets/music/theme_song.mp3')">Play Song ►</button>
				<button id="stop-button" class="stop-button">Reset Song ⏹</button>
			</div>
		</div>
	</section>
	<div>
		<h2 class="mt-5">Available Features</h2>
		<div class="feature-image-container">
			<?php
				foreach ($available_features as $feature) {
					if (is_array($feature)) {
						$feature_name = $feature[0];
						$page_url = $feature[1];
						$page_ext = strlen ($page_url) == 0 ? "" : "php";
					} else {
						$feature_name = $feature;
						$page_url = strtolower(str_replace(" ", "-", $feature));
						$page_ext = strlen ($page_url) == 0 ? "" : "php";
					}
					echo "<a href=\"pages/$page_url.$page_ext\" class=\"feature-card\">$feature_name</a>";
				}
			?>
		</div>
	</div>
</body>
</html>
