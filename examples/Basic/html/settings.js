function updateTestParam(state) {
    console.log("updateTestParam: " + state);
    document.getElementById('testParam').value = state;
    document.getElementById("storedTestParam").innerHTML = state;
}

function handleSubmit() {
    state = document.getElementById('testParam').value
    console.log("handleSubmit: " + state);
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            console.log("handleSubmit received: " + this.responseText);
            updateTestParam(this.responseText);
        }
    };
    xhttp.open("PUT", "params", true);
    xhttp.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
    xhttp.send("testParam=" + state);
}

function onLoad() {
    document.getElementById('testParam').value = 'new value here';
    console.log("onLoad");
    var testParamRequest = new XMLHttpRequest();
    testParamRequest.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            console.log("onLoad received: " + this.responseText);
            updateTestParam(this.responseText);
        }
    };
    testParamRequest.open("GET", "params?key=testParam", true);
    testParamRequest.send();
}