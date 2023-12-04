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
            background-image: url('sunrise.jpg'); /* Replace 'sunrise.jpg' with your image file path */
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
            <div class="form-group"> 
                <label for="brightnessSlider">Brightness (0-100):</label>
                <input type="range" class="form-control-range" id="brightnessSlider" min="0" max="100" value="0" oninput="sendBrightness()">
            </div>
            <div class="form-group">
                <label for="alarmTime">Set alarm time:</label>
                <input type="time" class="form-control" id="alarmTime">
            </div>
            <div class="form-group">
                <label for="wakeUpTime">Wake up time in minutes:</label>
                <input type="number" class="form-control" id="wakeUpTime" min="0">
            </div>
            <button type="submit" class="btn btn-primary">Save</button>
        </form>
    </div>

    <!-- Add Bootstrap JS and Popper.js CDN links if needed -->
    <!--
    <script src="https://code.jquery.com/jquery-3.5.1.slim.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.0.0-alpha1/dist/js/bootstrap.bundle.min.js"></script>
    -->

    <script>

        var Socket;
        function init() {
          
          Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
          document.getElementById('alarmForm').addEventListener('submit', function(event) {
            event.preventDefault(); // Prevents the default form submission
            submitForm();
          });
        }

        // keep getting (index):71 WebSocket is already in CLOSING or CLOSED state. error
        function sendBrightness() {
            var brightness = document.getElementById("brightnessSlider").value
            if(brightness === "0") {
              document.getElementById('lightToggle').checked = false;
            }
            else {
              document.getElementById('lightToggle').checked = true;
            }
            Socket.send('#' + brightness);
        }
        

        function sendLightStatus() {
          var lightStatus = document.getElementById("lightToggle").checked
          if (lightStatus === false) {
            document.getElementById("brightnessSlider").value = 0;
          } else {
            document.getElementById("brightnessSlider").value = 50;
          }
          Socket.send('L' + lightStatus);
        }
        
        function submitForm() {
          
            alert("Alarm saved");
            var brightnessValue = document.getElementById('brightnessSlider').value;
            var alarmTimeValue = document.getElementById('alarmTime').value;
            var wakeUpTimeValue = document.getElementById('wakeUpTime').value;

            
     
            console.log('Brightness:', brightnessValue);
            console.log('Set alarm time:', alarmTimeValue);
            console.log('Wake up time in minutes:', wakeUpTimeValue);

            Socket.send("Alarm time: " + alarmTimeValue + " | WakeUpIn: " + wakeUpTimeValue);
        }
    </script>
</body>
</html>
)=====";
