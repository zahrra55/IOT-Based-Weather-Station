// ===============================================
// IOT WEATHER STATION SKETCH FOR ARDUINO UNO R4 WIFI
// Reads DHT, BMP180/085 sensors, displays status on the LED Matrix,
// and publishes data to a local Web Dashboard.
// ===============================================

// --- 1. INCLUDE LIBRARIES ---
#include <Adafruit_BMP085.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "Arduino_LED_Matrix.h"
#include "WiFiS3.h"

// ----------------------------------------------------
// Pin Definitions
// ----------------------------------------------------
#define Rain_SensorPin A1 
#define Air_SensorPin A0
#define Temp_Hum_SensorPin 2

// ----------------------------------------------------
// Global Objects and Variables
// ----------------------------------------------------

// Update intervals
unsigned long lastSensorUpdate = 0;
const unsigned long sensorInterval = 5000; // Update sensors every 5 seconds

// Variables for Wi-Fi check and IP printing
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 30000; // Recheck Wi-Fi every 30 seconds

// NEW: IP print variables
unsigned long lastIPPrint = 0;
const unsigned long ipPrintInterval = 60000; // Print IP every 60 seconds

// Sensor and Network Objects
// DHT11 is less accurate and slower than DHT22, but works for this
DHT_Unified dht(Temp_Hum_SensorPin, DHT11); 
Adafruit_BMP085 bmp;
WiFiServer server(80);
ArduinoLEDMatrix matrix;

// LED Matrix frames (optional, used for visual feedback)
const uint32_t wifi_connected[] = {
  0x3f840,
  0x49f22084,
  0xe4110040
};
const uint32_t no_wifi[] = {
  0x403f844,
  0x49f22484,
  0xe4110040
};

// Wi-Fi Credentials
char ssid[] = "186F"; // Ensure this is your correct Wi-Fi Name
char pass[] = "30004000"; // Ensure this is your correct Wi-Fi Password

// Global data variables
float temperature = 0.0, humidity = 0.0, pressure = 0.0;
int AQI = 0, rainfall = 0;

// Define the threshold for analog rain reading
// Analog value is 0 (wet) to 1023 (dry). 
const int RAIN_ANALOG_THRESHOLD = 270;

// ----------------------------------------------------
// FUNCTION DEFINITIONS
// ----------------------------------------------------

// Prints the IP address in a clear box
void print_ip_box() {
  delay(100); 
  Serial.println("\n=============================================");
  Serial.println("  Connected to WiFi Successfully!");
  Serial.print("  Dashboard IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("=============================================");
}

// Connects to the defined Wi-Fi network
void wifi_connect() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    matrix.loadFrame(no_wifi);
    while (true)
      ;
  }
  
  Serial.print("Attempting to connect to WiFi network: ");
  Serial.println(ssid);
  matrix.loadSequence(LEDMATRIX_ANIMATION_WIFI_SEARCH);
  matrix.play(true);
  delay(1000); // Give the matrix time to start animation

  // Attempt connection using DHCP (dynamic IP assignment)
  WiFi.begin(ssid, pass); 

  // --- ADDED TIMEOUT LOGIC (30 seconds) ---
  unsigned long connectionStart = millis();
  const unsigned long connectionTimeout = 30000; // 30 seconds

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - connectionStart > connectionTimeout) {
      Serial.println("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
      Serial.println("!!! FAILED TO CONNECT AFTER 30 SECONDS    !!!");
      Serial.println("!!! Please check Wi-Fi name (ssid) and    !!!");
      Serial.println("!!! password in the code.               !!!");
      Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
      matrix.loadFrame(no_wifi);
      matrix.play(false);
      
      // Stop execution until the user power cycles or resets the board
      while(true); 
    }
    Serial.print(".");
    delay(1000);
  }
  // --- END TIMEOUT LOGIC ---

  matrix.loadFrame(wifi_connected);
  matrix.play(false); // Stop animation, show static frame
  
  // Print IP address once immediately after connection
  print_ip_box();
}

// Attempts to reconnect if connection is lost
void wifi_reconnect() {
  Serial.println("Wifi Reconnecting........");
  matrix.loadFrame(no_wifi);
  wifi_connect();
}

/**
 * @brief Reads data from all connected sensors and updates global variables.
 * * Includes NaN check for DHT sensor to prevent corrupted data from being sent 
 * to the web dashboard, which would otherwise create gaps in the charts.
 */
void read_sensor_data() {
  sensors_event_t event;
  float new_temperature, new_humidity;

  // --- DHT (Temperature & Humidity) ---
  dht.temperature().getEvent(&event);
  new_temperature = event.temperature;
  dht.humidity().getEvent(&event);
  new_humidity = event.relative_humidity;

  // IMPORTANT: Check for NaN (sensor failure). If invalid, KEEP the old value.
  if (!isnan(new_temperature) && new_temperature >= -50.0 && new_temperature <= 100.0) {
    temperature = new_temperature;
  } else {
    Serial.println("Warning: Invalid temperature reading (NaN/Extreme). Keeping previous value.");
  }
  
  if (!isnan(new_humidity) && new_humidity >= 0.0 && new_humidity <= 100.0) {
    humidity = new_humidity;
  } else {
    Serial.println("Warning: Invalid humidity reading (NaN/Extreme). Keeping previous value.");
  }

  // --- BMP (Pressure) ---
  pressure = bmp.readPressure() / 100.0; // Convert Pa to hPa (mbar)

  // --- MQ-135 (Air Quality) ---
  int mq135Raw = analogRead(Air_SensorPin);
  // This is a rough estimation; highly depends on calibration!
  float mq135PPM = mq135Raw * (5.0 / 1023.0) * 200.0;
  AQI = map(mq135PPM, 0, 500, 0, 300);
  // Ensure AQI is within a reasonable range for display
  if (AQI > 500) AQI = 500;
  if (AQI < 0) AQI = 0;

  // --- Rain Sensor (Analog: 0=Wet, 1023=Dry) ---
  int rainAnalogRead = analogRead(Rain_SensorPin);
  rainfall = rainAnalogRead < RAIN_ANALOG_THRESHOLD ? 1 : 0; 
  
  Serial.println("--- Sensor Reading ---");
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.println(" C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.println(" mbar");
  Serial.print("AQI: ");
  Serial.println(AQI);
  // Show the raw analog value for debugging
  Serial.print("Rain (Analog A1): "); 
  Serial.print(rainAnalogRead);
  Serial.print(" -> ");
  Serial.println(rainfall ? "Yes" : "No");
  Serial.println("-------------------------");
}

// Sends the sensor data as a JSON object to the client
void send_json_data(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();

  // Note: Since we filter NaN in read_sensor_data, the String() conversion is safe.
  String json = "{\"temperature\":" + String(temperature) + 
                ",\"humidity\":" + String(humidity) + 
                ",\"pressure\":" + String(pressure) + 
                ",\"aqi\":" + String(AQI) + 
                ",\"rainfall\":" + String(rainfall) + "}";

  client.println(json);
}

// Sends the main HTML dashboard page to the client
void send_web_page(WiFiClient &client) {
  // Send HTTP headers
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  // Use raw string literal for HTML content
  // UPDATED HTML AND CSS FOR BETTER AESTHETICS AND COMPREHENSIVE CHART
  const char *html = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>IoT Weather Station Dashboard</title>
<style>
/* Modern Styling */
body { 
    font-family: 'Inter', sans-serif; 
    background: #eef2f5; 
    color: #2c3e50; 
    text-align: center; 
    padding: 20px; 
}
h1 { 
    font-size: 2.2rem; 
    color: #34495e; 
    margin-bottom: 30px; 
    font-weight: 700;
}
.container { 
    max-width: 1000px; 
    margin: auto; 
}
/* Grid for Cards */
.data-grid { 
    display: grid; 
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); 
    gap: 15px; 
    margin-bottom: 25px; 
}
.data-card { 
    background: #ffffff; 
    padding: 20px; 
    border-radius: 12px; 
    box-shadow: 0 6px 15px rgba(0, 0, 0, 0.08); 
    display: flex;
    flex-direction: column;
    align-items: center;
}
.card-label {
    font-size: 0.9rem;
    color: #7f8c8d;
    margin-bottom: 5px;
    font-weight: 500;
}
.card-value {
    font-size: 1.6rem;
    font-weight: 600;
    color: #2980b9;
}
.rain-yes { color: #e74c3c; } /* Red for Rain */
.rain-no { color: #27ae60; }  /* Green for No Rain */

/* Graph Container */
.graph { 
    background: #ffffff; 
    padding: 20px; 
    border-radius: 12px; 
    box-shadow: 0 6px 15px rgba(0, 0, 0, 0.08); 
}
canvas { 
    width: 100% !important; 
    height: 450px !important; 
}

/* Responsive adjustments */
@media (max-width: 600px) {
    .data-grid {
        grid-template-columns: 1fr;
    }
}
</style>
</head>
<body>
<h1>IoT Weather Station Dashboard</h1>
<div class='container'>
    <div id='weather' class='data-grid'>
        <!-- Sensor Cards will be injected here -->
    </div>
    <div class='graph'>
        <h2>Sensor Readings Trend</h2>
        <canvas id='combinedGraph'></canvas>
    </div>
</div>
<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>
<!-- Required for multiple axes in Chart.js -->
<script src='https://cdn.jsdelivr.net/npm/chartjs-plugin-datalabels@2.0.0'></script>
<script>
const ctxCombined = document.getElementById('combinedGraph').getContext('2d');

// Enhanced Chart Configuration to plot all 4 variables
const combinedChart = new Chart(ctxCombined, {
    type: 'line',
    data: {
        labels: [],
        datasets: [
        {
            label: 'Temperature (°C)',
            data: [],
            borderColor: '#ff5733',
            backgroundColor: 'rgba(255, 87, 51, 0.1)',
            yAxisID: 'yTemp',
            tension: 0.3,
            fill: true
        },
        {
            label: 'Humidity (%)',
            data: [],
            borderColor: '#2196f3',
            backgroundColor: 'rgba(33, 150, 243, 0.1)',
            yAxisID: 'yHumid',
            tension: 0.3,
            fill: true
        },
        {
            label: 'Pressure (mbar)',
            data: [],
            borderColor: '#34495e',
            backgroundColor: 'rgba(52, 73, 94, 0.1)',
            yAxisID: 'yPressure',
            tension: 0.3,
            fill: false 
        },
        {
            label: 'AQI (Index)',
            data: [],
            borderColor: '#f1c40f',
            backgroundColor: 'rgba(241, 196, 15, 0.1)',
            yAxisID: 'yAQI',
            tension: 0.3,
            fill: false
        }
        ]
    },
    options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: false,
        interaction: {
            mode: 'index',
            intersect: false,
        },
        stacked: false,
        scales: {
            x: { 
                title: { display: true, text: 'Time' }, 
                grid: { display: false }
            },
            yTemp: {
                type: 'linear',
                display: true,
                position: 'left',
                title: { display: true, text: 'Temp (°C) / Humidity (%)' },
                grid: { color: 'rgba(0, 0, 0, 0.05)'},
                min: 0,
                max: 100, // Shared for Temp/Humid
                ticks: { color: '#ff5733' }
            },
            yHumid: { // Shares the same range as yTemp
                type: 'linear',
                display: false, 
                position: 'left',
            },
            yPressure: {
                type: 'linear',
                display: true,
                position: 'right',
                title: { display: true, text: 'Pressure (mbar)' },
                grid: { drawOnChartArea: false },
                min: 950, // Typical low range
                max: 1050, // Typical high range
                ticks: { color: '#34495e' }
            },
            yAQI: {
                type: 'linear',
                display: true,
                position: 'right',
                title: { display: true, text: 'AQI' },
                grid: { drawOnChartArea: false },
                min: 0,
                max: 300, // Standard max for unhealthy AQI range
                ticks: { color: '#f1c40f' },
                offset: true // Offset from yPressure
            }
        }
    }
});

// Fetch and update data
function fetchWeatherData() {
    fetch('/data') 
        .then(response => {
            if (!response.ok || response.headers.get("content-type").indexOf("application/json") === -1) {
                console.error("Not a valid JSON response:", response.status);
                return;
            }
            return response.json();
        })
        .then(data => {
            if (!data) return;

            // Update Cards
            const rainfallClass = data.rainfall ? 'rain-yes' : 'rain-no';
            const rainfallText = data.rainfall ? 'YES' : 'NO';

            document.getElementById('weather').innerHTML = `
                <div class='data-card'>
                    <div class='card-label'>Temperature</div>
                    <div class='card-value'>${data.temperature}°C</div>
                </div>
                <div class='data-card'>
                    <div class='card-label'>Humidity</div>
                    <div class='card-value'>${data.humidity}%</div>
                </div>
                <div class='data-card'>
                    <div class='card-label'>Pressure</div>
                    <div class='card-value'>${data.pressure} mbar</div>
                </div>
                <div class='data-card'>
                    <div class='card-label'>AQI</div>
                    <div class='card-value'>${data.aqi}</div>
                </div>
                <div class='data-card'>
                    <div class='card-label'>Rain Detected</div>
                    <div class='card-value ${rainfallClass}'>${rainfallText}</div>
                </div>
            `;

            // Update Chart
            let time = new Date().toLocaleTimeString();

            // Add new data points
            combinedChart.data.labels.push(time);
            combinedChart.data.datasets[0].data.push(data.temperature); // Temperature
            combinedChart.data.datasets[1].data.push(data.humidity);  // Humidity
            combinedChart.data.datasets[2].data.push(data.pressure);  // Pressure
            combinedChart.data.datasets[3].data.push(data.aqi);       // AQI

            // Keep only the last 15 points for better trend visualization
            const maxPoints = 15;
            if (combinedChart.data.labels.length > maxPoints) {
                combinedChart.data.labels.shift();
                combinedChart.data.datasets[0].data.shift();
                combinedChart.data.datasets[1].data.shift();
                combinedChart.data.datasets[2].data.shift();
                combinedChart.data.datasets[3].data.shift();
            }
            combinedChart.update();
        })
        .catch(error => console.error('Error fetching data:', error));
}
// Fetch data every 1 second
setInterval(fetchWeatherData, 1000); 
</script>
</body>
</html>
)rawliteral";
  // Send the entire HTML content in one go
  client.print(html);
}


// Handles incoming client requests
void run_local_webserver() {
  WiFiClient client = server.available();
  if (client) {
    // Read the first line of the request (e.g., "GET / HTTP/1.1")
    String request = client.readStringUntil('\n');
    client.flush(); // Clear remaining bytes

    // Check for the requested path
    if (request.indexOf("GET /data") != -1) {
      // If the request is for JSON data for the AJAX update
      send_json_data(client);
    } else if (request.indexOf("GET / ") != -1) { 
      // If the request is for the root path (the main dashboard page)
      send_web_page(client);
    }

    // Give the client time to send the data before closing the connection
    delay(1); 
    client.stop();
  }
}


// ----------------------------------------------------
// REQUIRED ARDUINO SETUP AND LOOP (MODIFIED FOR RELIABILITY)
// ----------------------------------------------------

void setup() {
  // Start Serial for debugging
  Serial.begin(115200);
  matrix.begin();
  
  // 1. Initialize Sensors
  dht.begin();
  // pinMode is not strictly required for analogRead but kept for completeness
  pinMode(Air_SensorPin, INPUT); 
  pinMode(Rain_SensorPin, INPUT); 

  // Check for BMP sensor connection (crucial for BMP085 library error check)
  if (!bmp.begin()) {
    Serial.println("FATAL ERROR: Could not find BMP085/BMP180 sensor or library is missing.");
    Serial.println("Please ensure the Adafruit BMP085 Library is installed and the sensor is wired correctly (SDA/SCL).");
    matrix.loadFrame(no_wifi);  // Visual cue for a sensor error
    while (1)
      ; // Stop execution on fatal sensor error
  }

  // 2. Connect to Wi-Fi and start server
  wifi_connect();
  server.begin();

  // 3. Perform initial sensor read
  read_sensor_data();
  lastSensorUpdate = millis();
  
  // Initialize the IP printing timer
  lastIPPrint = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. Handle Wi-Fi Reconnection
  if (WiFi.status() != WL_CONNECTED && currentMillis - lastWiFiCheck >= wifiCheckInterval) {
    wifi_reconnect();
    lastWiFiCheck = currentMillis; 
  }
  
  // 2. Update Sensor Readings Periodically
  if (currentMillis - lastSensorUpdate >= sensorInterval) {
    read_sensor_data();
    lastSensorUpdate = currentMillis;
  }

  // 3. Run the Web Server to Serve Data
  run_local_webserver();

  // 4. Periodically print the IP address
  if (WiFi.status() == WL_CONNECTED && currentMillis - lastIPPrint >= ipPrintInterval) {
    print_ip_box();
    lastIPPrint = currentMillis;
  }
}
