function playAudio(audioFile) {
	var audio = new Audio(audioFile);
	audio.play().catch(function(error) {
		console.error("Error playing audio:", error);
	});
}

function sendDelete(file, uploadFile) {
	const div = document.getElementById(file);
	const mess = document.getElementById("messageBox_" + file);

	console.log(uploadFile);
	mess.style.display = "none";
	if (uploadFile)
		uploadFile = '&upload=/var/uploads/' + uploadFile
	fetch('/delete-post?file=/var/posts/' + file + uploadFile, {
		method: 'DELETE'
	})
	.then(async response => {
	if (!response.ok)
	{
		const errorHtml = await response.text();

		document.open();
		document.write(errorHtml);
		document.close();
	}
	})
	.then(data => console.log('Deleted:', data))
	.catch(err => console.error('Error:', err));
	div.remove();
}



const button = document.getElementById('stop-button');

if (button) {
	button.addEventListener('mouseover', function () {
		button.style.left = `${Math.ceil(Math.random() * 90)}%`;
		button.style.top = `${Math.ceil(Math.random() * 90)}%`;
		button.style.p
	});
	button.addEventListener('click', function () {
		playAudio('./assets/music/theme_song.mp3');
	})
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


function downloadFile(filepath, filename) {
	fetch(filepath, {
		method: 'GET',
		headers: {
			'Accept': 'application/octet-stream'
		}
	})
	.then(response => {
		if (!response.ok) {
			throw new Error('server response not ok');
		}
		return response.blob();
	})
	.then(blob => {
		const url = window.URL.createObjectURL(blob);

		const a = document.createElement('a');
		a.href = url;
		a.download = filename;
		document.body.appendChild(a);
		a.click();
		document.body.removeChild(a);

		window.URL.revokeObjectURL(url);
	})
	.catch(error => {
		console.error('Download failed:', error);
	});
}

document.querySelectorAll('.theme-link').forEach(a => {
  a.addEventListener('click', function(e){
    e.preventDefault();
    const theme = this.dataset.theme;
    // set cookie (max-age en secondes)
    document.cookie = "Theme=" + theme + "; path=/; max-age=3600; SameSite=Lax";
    // update page styles immediately
    document.body.style.background = (theme === 'yellow') ? '#ffe066' : '#c71585';
    document.body.style.color = (theme === 'yellow') ? '#b8860b' : '#c71585';
    // update active class
    document.querySelectorAll('.theme-link').forEach(x => x.classList.remove('active'));
    this.classList.add('active');
  });
});

