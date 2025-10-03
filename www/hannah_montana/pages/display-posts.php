<?php
	$postsDir = __DIR__ ."/../posts/";
	$posts = glob($postsDir . '*.txt');
	usort($posts, function($a, $b) {
		return filemtime($b) - filemtime($a); // newest first
	});
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
<body>

	<h2>Posts</h2>


	<?php
	$inMessage = false;

	foreach($posts as $postFile){
		$lines = file($postFile, FILE_IGNORE_NEW_LINES);
		$data = [];

		foreach ($lines as $line){
			if ((strpos($line, "name=") !== false) || (strpos($line, "file=") !== false)){
				list($key, $value) = explode('=', $line, 2);
				$data[trim($key)] = trim($value);
				$inMessage = false;
			}
			if ((strpos($line, "message=") === 0)){
				list($key, $value) = explode('=', $line, 2);
				$data[trim($key)] = trim($value);
				$inMessage = true;
			}
			else if ($inMessage){
				$data['message'] .= "\n" . $line;
			}
		}

		
		$name = htmlspecialchars($data['name']);
		$message = nl2br(htmlspecialchars($data['message']));
		$file = $data['file'] ?? null;

		echo "<div class='post'>";

		echo "<h3> $name </h3>";
		echo "<h4> $message </h4>";

		if ($file){ //TODO - make it work
			$filename = basename($file);

			$filepath = '../uploads/' . $filename;
			if (file_exists($filepath)){
				if (strpos(mime_content_type($filepath), 'image/') !== false){
					echo "<img src='$filepath' alt='image' class='image-file'>";
				}
				else{
					echo "<a href='$filepath' class='download-file'>Download : $filename</a>";
				}
			}
		}
		echo "</div>";
	}
	?>

	
</body>
</html>