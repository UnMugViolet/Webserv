
function playAudio(audioFile) {
	var audio = new Audio(audioFile);
	audio.play().catch(function(error) {
		console.error("Error playing audio:", error);
	});
}

function sendDelete(file, uploadFile) {
	const div = document.getElementById(file);
	const mess = document.getElementById("messageBox");

	mess.style.display = "none";
	if (uploadFile)
		uploadFile = '&upload=' + uploadFile
	fetch('/delete-post?file=/posts/' + file + uploadFile + '&response=display-post.php', {
		method: 'DELETE'
	})
	
	.then(response => response.text())
	.then(data => console.log('Deleted:', data))
	.catch(err => console.error('Error:', err));
	div.remove();
}

function showBox(boxId) {
	console.log("show");
	const div = document.getElementById(boxId);
	
	div.style.display = "flex";
}

function hideBox(boxId) {
	const div = document.getElementById(boxId);

	div.style.display = "none";
}