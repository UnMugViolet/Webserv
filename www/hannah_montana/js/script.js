
function playAudio(audioFile) {
	console.log("pouet")
	var audio = new Audio(audioFile);
	audio.play().catch(function(error) {
		console.error("Error playing audio:", error);
	});
}

function sendDelete(file) {
	console.log(file);
	fetch('/delete-post?file=/posts/' + file, {
		method: 'DELETE'
	})
	.then(response => response.text())
	.then(data => console.log('Deleted:', data))
	.catch(err => console.error('Error:', err));
}
