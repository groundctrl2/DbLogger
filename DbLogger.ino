#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* ssid = "Wifi-SSID"; // * Marks to store in flash memory, pointer in RAM
const char* password = "password";

#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Declaration for display

const byte sensorPin = A0; // DAOKI audio sensor pin
int graphBuffer[SCREEN_WIDTH] = {0}; // Buffer to store prev readings
int loopCounter = 0; // Counts up to 128 (screen width) before sending buffer to google sheets

int threshold = 0; // Adjust as needed

void setup() {
  Serial.begin(9600);

  // Setup display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Initialize display
  display.clearDisplay();  // Clear Adafruit logo
  display.setTextColor(WHITE);
  display.setTextSize(2);

  pinMode(sensorPin, INPUT);

  // Setup wifi
  wifi_setup();
  delay(2000); // Pause to show wifi connected
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // Calc value (to OLED) and add to buffer
    int sensorValue = analogRead(sensorPin); // Get output from sensor
    int normalizedValue = map(sensorValue, 0, 4095, 0, SCREEN_HEIGHT - 1); // Scale to OLED height based on max pot value
    updateGraph(normalizedValue); // Add normalized value to buffer
    drawGraph(); // Draw to OLED

    // TESTING
    if (sensorValue > threshold) threshold = sensorValue;
    Serial.println("Sensor Value: " + String(sensorValue) + " | Normalized Value: " + String(normalizedValue) + " | Max Sensor Value: " + String(threshold));
  }
  else wifi-setup(); // Attempt reconnecting
}

void sendBuffer() {
  Serial.println("Sending buffer");

  // Convert buffer to JSON array
  String postData = "[";
  for (int i = 0; i < SCREEN_WIDTH; i++) {
    postData += String(graphBuffer[i]);
    if (i < SCREEN_WIDTH - 1) postData += ",";
  }
  postData += "]";

  HTTPClient http;
  http.begin("https://script.google.com/macros/googleScriptLink/exec");
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(postData);

  if (httpResponseCode > 0) {
    Serial.println("Data sent successfully: " + String(httpResponseCode));
  } else {
    Serial.println("Error sending data: " + String(httpResponseCode));
  }

  http.end();
}

void wifi_setup() {
  WiFi.begin(ssid, password, 6); // Delete 6 if wifi channel unknown, it'll find it (slower).
  write_to_oled("CONNECTING");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  write_to_oled("CONNECTED");
}

void updateGraph(int newValue) {
  for (int i = 0; i < SCREEN_WIDTH - 1; i++) // Shift all values in the buffer to the left
    graphBuffer[i] = graphBuffer[i+1];
  graphBuffer[SCREEN_WIDTH - 1] = newValue; // Add the new value at the end of the buffer
}

void drawGraph() {
  display.clearDisplay();
  // Draw the waveform using the buffer
  for (int x = 0; x < SCREEN_WIDTH - 1; x++) {
    int y1 = SCREEN_HEIGHT - 1 - graphBuffer[x]; // Invert y-axis for OLED
    int y2 = SCREEN_HEIGHT - 1 - graphBuffer[x + 1];
    display.drawLine(x, y1, x + 1, y2, WHITE); // Connect points with lines
  }
  display.display();
}

void write_to_oled(String msg) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(msg);
  display.display();
}
