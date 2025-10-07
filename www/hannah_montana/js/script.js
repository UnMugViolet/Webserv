
function playAudio(audioFile) {
	var audio = new Audio(audioFile);
	audio.play().catch(function(error) {
		console.error("Error playing audio:", error);
	});
}

function sendDelete(file, uploadFile) {
	if (uploadFile)
		uploadFile = '&upload=' + uploadFile
	fetch('/delete-post?file=/posts/' + file + uploadFile + '&response=display-post.php', {
		method: 'DELETE'
	})
	.then(response => response.text())
	.then(data => console.log('Deleted:', data))
	.catch(err => console.error('Error:', err));
}
