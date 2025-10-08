
function playAudio(audioFile) {
	var audio = new Audio(audioFile);
	audio.play().catch(function(error) {
		console.error("Error playing audio:", error);
	});
}

function sendDelete(file, uploadFile) {
	const div = document.getElementById(file);
	const mess = document.getElementById("messageBox_" + file);

	mess.style.display = "none";
	if (uploadFile)
		uploadFile = '&upload=' + uploadFile
	fetch('/delete-post?file=/posts/' + file + uploadFile, {
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

function showSubmitMessage() {
	const message = document.getElementById("submitMessage");
	message.style.display = "flex";
	
	setTimeout(() => {message.style.display = "none" ; 
	}, 3000);

	setTimeout(() => {document.getElementById("form").reset()}, 10);
}
