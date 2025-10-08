<?php

$form = [];

?>

<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Hannah Montana Hardcore Gang</title>
	<link rel="stylesheet" href="../css/style.css">
	<link rel="icon" href="favicon.ico">
	<script src="./js/script.js" defer></script>
	<link rel="preconnect" href="https://fonts.googleapis.com">
	<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
	<link href="https://fonts.googleapis.com/css2?family=Damion&family=Mr+Dafoe&display=swap" rel="stylesheet">

</head>
<html>
<body>

	<h2>Post</h2>
	<div class="form-container">
		<form action="/pages/post.php" method="POST" enctype="multipart/form-data">
			<label for="name">Name:</label>
			<input type="text" id="name" name="name" required><br><br>

			<label for="message">Message:</label><br>
			<textarea id="message" name="message" rows="4" cols="50" required></textarea><br><br>

			<label for="file">File upload:</label>
			<input type="file" name="file"><br><br>

			<input class="" type="submit" value="Envoyer">
		</form>
	</div>

	<div class="form-response">
		<?php
			if(!isset($_SERVER["REQUEST_METHOD"]))
			{
				echo "<h1 class=\"text-5xl\"> Antonin needs to do that part !</h1>"; // TODO - must be done by Antonin
				return ;
			}
			if ($_SERVER["REQUEST_METHOD"] == "POST") {
				// $name = htmlspecialchars($_POST['name']);
				// $message = htmlspecialchars($_POST['message']);

				if (!empty($name) && !empty($message)) {
					echo "<h3>Thank you, $name!</h3>";
					echo "<p>Your message has been received:</p>";
					echo "<blockquote>$message</blockquote>";
				} else {
					echo "<p class=\"error\">Please fill in all fields.</p>";
				}
			}
		?>
	</div>

</body>
</html>
