function updateDeviceName(state) {
    console.log("updateDeviceName: " + state);
    document.getElementById('deviceName').value = state;
    document.getElementById("storedDeviceName").innerHTML = state;
}

function handleSubmit() {
    state = document.getElementById('deviceName').value
    console.log("handleSubmit: " + state);
    var deviceNameRequest = new XMLHttpRequest();
    deviceNameRequest.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            console.log("handleSubmit received: " + this.responseText);
            updateDeviceName(this.responseText);
        }
    };
    deviceNameRequest.open("PUT", "params", true);
    deviceNameRequest.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
    deviceNameRequest.send("deviceName=" + state);
}

function onLoad() {
    document.getElementById('deviceName').value = 'new value here';
    console.log("onLoad");
    var deviceNameRequest = new XMLHttpRequest();
    deviceNameRequest.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            console.log("onLoad received: " + this.responseText);
            updateDeviceName(this.responseText);
        }
    };
    deviceNameRequest.open("GET", "params?key=deviceName", true);
    deviceNameRequest.send();
}