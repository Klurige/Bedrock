function updateLed(state) {
    console.log("updateLed: " + state);
    document.getElementById("ledStatus").innerHTML = state;
    var ledSwitch = document.getElementById("ledSwitch");
    if (state == "on") {
        ledSwitch.checked = true;
    } else {
        ledSwitch.checked = false;
    }
}

function updateCountDir(state) {
    console.log("updateCountDir: " + state);
    document.getElementById("countDir").innerHTML = state;
}


function toggleLed() {
    var ledSwitch = document.getElementById("ledSwitch");
    var state;
    if (ledSwitch.checked == true) {
        state = "on";
    } else {
        state = "off";
    }

    // Make it grey.

    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            console.log("toggleLed received: " + this.responseText);
            updateLed(this.responseText);
        }
    };
    xhttp.open("GET", "led_state?value=" + state, true);
    xhttp.send();
}

function onLoad() {
    var ledRequest = new XMLHttpRequest();
    ledRequest.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            console.log("onLoad received: " + this.responseText);
            updateLed(this.responseText);
        }
    };
    ledRequest.open("GET", "led_state", true);
    ledRequest.send();

    var countRequest = new XMLHttpRequest();
    countRequest.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            console.log("onLoad received: " + this.responseText);
            updateCountDir(this.responseText);
        }
    };
    countRequest.open("GET", "count_dir_set", true);
    countRequest.send();
}

if (!!window.EventSource) {
    var source = new EventSource('/events');
    source.addEventListener('measurement', function (e) {
        console.log("Received measurement");
        document.getElementById("measurement").innerHTML = e.data;
    }, false);
    source.addEventListener('led_state', function (e) {
        updateLed(e.data);
    }, false);
    source.addEventListener('count_direction', function (e) {
        console.log("Received count_direction");
        updateCountDir(e.data);
    }, false);
    source.addEventListener('open', function (e) {
        console.log("Events Connected");
        onLoad();
    }, false);
    source.addEventListener('error', function (e) {
        if (e.target.readyState != EventSource.OPEN) {
            console.log("Events Disconnected");
            numLost++;
            document.getElementById("numLost").innerHTML = numLost;
        }
    }, false);
}
