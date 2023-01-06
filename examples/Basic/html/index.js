function startApp() {
    window.location = "/app.html";
}

function uploadFile() {
    window.location = "/upload.html";
}

function listContents() {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/listContents", true);
    xhttp.send();
}

function logHistory() {
    window.location = "/log.html";
}

function settings() {
    window.location = "/settings.html";
}

function logout() {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/logout", true);
    xhr.send();
    setTimeout(function () { window.open("/logout.html", "_self"); }, 1000);
}
var seconds = 10;
var counterId;

function restartRedirect() {
    document.getElementById("countDown").style.display = "none";
    document.getElementById("btns").style.display = "block";
    document.location.href = '/';
}

function updateSecs(reason) {
    document.getElementById("seconds").innerHTML = seconds;
    seconds--;
    if (seconds == 8) {
        var xhttp = new XMLHttpRequest();
        xhttp.open("GET", "/restart?mode=" + reason, true);
        xhttp.send();
    }
    if (seconds == -1) {
        clearInterval(counterId);
        restartRedirect();
    }
}

function restartCountDown(reason) {
    document.getElementById("seconds").innerHTML = seconds;
    seconds--;
    counterId = setInterval(function () {
        updateSecs(reason)
    }, 1000);
}

function restart(reason) {
    seconds = 10;
    document.getElementById("btns").style.display = "none";
    document.getElementById("countDown").style.display = "block";
    restartCountDown(reason);
}