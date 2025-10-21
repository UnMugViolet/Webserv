<?php 

$undefinedClass = new ThisIsAnUndefinedClass();

?>

<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Document</title>
</head>
<body>
	<?php 
		echo "My code has syntax errors: ";
		echo $undefinedClass;
	?>
</body>
</html>
