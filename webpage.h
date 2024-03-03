const char webpageCode[] = 
R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">
    <title>Sunrise Alarm</title>
    <!-- Add Bootstrap CSS CDN link -->
    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css">
    <style>
        .jumbotron {
            background-image: url('https://img.freepik.com/premium-photo/beautiful-sunrise-mountain-meadow-landscape-refreshment-with-sunshine-golden-grass_930198-1188.jpg'); /* Replace 'sunrise.jpg' with your image file path */
            background-size: cover;
            color: white;
        }
    </style>
</head>
<body onload = "javascript:init()">
    <div class="jumbotron text-center">
        <h1 class="display-4">Sunrise Alarm</h1>
    </div>

    <div class="container mt-3">
        <form id="alarmForm">
            <div class="form-group form-check custom-control custom-switch">
                <input type="checkbox" class="custom-control-input" id="lightToggle" oninput="sendLightStatus()">
                <label class="custom-control-label" for="lightToggle">Light On/Off</label>
            </div>
             <div class="form-group form-check custom-control custom-switch">
                <input type="checkbox" class="custom-control-input" id="alarmToggle" oninput="sendAlarmStatus()">
                <label class="custom-control-label" for="alarmToggle">Alarm On/Off</label>
            </div>
            <div class="form-group"> 
                <label for="brightnessSlider">Brightness (0-100):</label>
                <input type="range" class="form-control-range" id="brightnessSlider" min="0" max="100" value="0" oninput="sendBrightness()">
            </div>
            <div class="form-group">
                <label for="alarmTime">Set alarm time:</label>
                <input type="time" class="form-control" id="alarmTime" oninput="sendAlarmTime()">
            </div>
            <div class="form-group">
                <label for="wakeUpTime">Wake up time in minutes:</label>
                <input type="number" class="form-control" id="wakeUpTime" min="0" oninput="sendWakeTime()">
            </div>
        </form>
    </div>

    <!-- Add Bootstrap JS and Popper.js CDN links if needed -->
    <!--
    <script src="https://code.jquery.com/jquery-3.5.1.slim.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.0.0-alpha1/dist/js/bootstrap.bundle.min.js"></script>
    -->

    <script>

        var Socket;
        var lastLightLevel = 50;
        
        function init() {
          
            socket = new WebSocket('ws://' + window.location.hostname + ':81/');
            socket.onmessage = socketRecv;
            socket.onopen = onopen;
            socket.onerror = onerror;

            document.getElementById('alarmForm').addEventListener('submit', function(event) {
                event.preventDefault(); // Prevents the default form submission
                submitForm();
            });
            
        }
        
        function onopen() {
            // Now, ask the server to send the current values of things (brightness, 
            // alarm time, wake time)...
            console.log("websocket connection open");
            socket.send("#?");
        }

        function onerror(error_event) {
            alert('Websocket error: ' + event);
        }

        // keep getting (index):71 WebSocket is already in CLOSING or CLOSED state. error
        function sendBrightness() {
            var brightness = document.getElementById("brightnessSlider").value;
            if(brightness === "0") {
                document.getElementById('lightToggle').checked = false;
            }
            else {
                document.getElementById('lightToggle').checked = true;
            }
            socket.send('#s' + brightness);
        }
        

        function sendLightStatus() {
            var lightStatus = document.getElementById("lightToggle").checked
            var brightness;
            
            if (lightStatus === false) {
                lastLightLevel = document.getElementById("brightnessSlider").value;
                document.getElementById("brightnessSlider").value = 0;
                brightness = 0;
            } else {
                document.getElementById("brightnessSlider").value = lastLightLevel;
                brightness = lastLightLevel;
            }
            
            socket.send('#s' + brightness);
        }

        function sendAlarmStatus() {
            var alarmStatus = document.getElementById("alarmToggle").checked
            var newAlarmStatus;
            
            if (alarmStatus === false) {
                newAlarmStatus = 'f';                        
            } else {
                newAlarmStatus = 'o';                        
            }
             
            socket.send('#a' + newAlarmStatus);
        }

        function sendAlarmTime() {
            var alarmTimeValue = document.getElementById('alarmTime').value;
            socket.send("#a" + alarmTimeValue);
          
        }

        function sendWakeTime() {
          var wakeUpTimeValue = document.getElementById('wakeUpTime').value;
          socket.send("#w" + wakeUpTimeValue * 60);
        }
        

        // This function receives a status message from webserver to update onscreen values...
        function socketRecv(event) {
            if (event.data[0] = '#') {
                console.log(event.data[1]);
                console.log(event.data.substr(2));
                switch(event.data[1]) {
                    case 'a':
                        if (event.data[2] == 'o') {
                            document.getElementById("alarmToggle").checked = true;      
                        } else if (event.data[2] == 'f') {
                            document.getElementById("alarmToggle").checked = false;
                        } else {
                            document.getElementById('alarmTime').value = event.data.substr(2);
                        }
                        break;
                    case 's':
                        document.getElementById('brightnessSlider').value = event.data.substr(2);
                        if (event.data.substr(2) > 0) {
                            document.getElementById("lightToggle").checked = true;    
                        } else {
                            document.getElementById("lightToggle").checked = false;
                        }
                        break;
                    case 'w':
                        document.getElementById('wakeUpTime').value = (event.data.substr(2) / 60);
                        break;
                    default:
                        console.log("Received unknown message: " + event.data);
                        break;
                }
            } else {
                console.log("Received unknown message: " + event.data);
            }
        }
    </script>
</body>
</html>
)=====";
