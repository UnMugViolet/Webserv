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
	<script src="./../js/script.js" defer></script>
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
		$safePostFile = htmlspecialchars(basename($postFile), ENT_QUOTES, 'UTF-8');

		echo	"<div id=\"messageBox_$safePostFile\" class=\"deleteBox\">";
		echo	"	<h3> Delete post? </h3>";
		echo	"	<button onclick='sendDelete(\"$safePostFile\", \"$file\")' class=\"yesBox\">YES</button>";
		echo	"	<button onclick='hideBox(\"messageBox_$safePostFile\")' class=\"noBox\">NO</button>";
		echo	"</div>";
		echo "<div class='post' id=\"$safePostFile\">";

		echo "	<button onclick='showBox(\"messageBox_$safePostFile\")' class='cross'>X</button>"; //TODO - delete post + file by pressing the X

		echo "<h3> $name </h3>";
		echo "<h4> $message </h4>";

		if ($file){
			$filename = basename($file);

			$filepath = '../uploads/' . $filename;
			if (file_exists(__DIR__ . '/' . $filepath)){
				if (strpos(mime_content_type(__DIR__ . '/' . $filepath), 'image/') !== false){
					echo "<img src='$filepath' class='image-file'>";
				}
				else{
					echo "<a href='$filepath' class='download-file'>Download : $filename</a>";
				}
			}
			else
				echo "broken file: $filepath";
		}
		echo "</div>";
	}
	?>

	
</body>
</html>