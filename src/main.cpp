#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <WiFi.h>
#include <WebSocketsServer.h>

const char *ssid = <ssid>;
const char *password = <password>;

WebSocketsServer webSocket = WebSocketsServer(81);
bool clientConnected = false;

const int buttonPin = 12;
const int led = 13;

// Push button related variables
bool buttonState = HIGH;
bool lastButtonReading = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 50;

bool readState = LOW;

void handleButton()
{
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonReading)
  {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading != buttonState)
    {
      buttonState = reading;
      if (buttonState == LOW)
      {
        Serial.println("BTN");
        readState = !readState;
        digitalWrite(led, readState);
      }
    }
  }
  lastButtonReading = reading;
}

// OLED setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Analog pin
int sensorPin = 34;
int sensorValue = 0;

// Graph
const int graphHeight = 40;
const int graphYStart = 20;
int graphX = 0;
int lastY = graphYStart + graphHeight / 2;

void updateGraph()
{

  // Text Section
  display.fillRect(0, 8, SCREEN_WIDTH, 8, SSD1306_BLACK); // Clear only text area
  display.setCursor(0, 8);
  display.printf("Value: %d", sensorValue);

  // Show WebSocket status icon
  if (clientConnected)
  {
    display.fillCircle(SCREEN_WIDTH - 6, 11, 3, SSD1306_WHITE); // filled circle for connected
  }
  else
  {
    display.drawCircle(SCREEN_WIDTH - 6, 11, 3, SSD1306_WHITE); // empty circle for disconnected
  }

  // Graph Section
  if (graphX >= SCREEN_WIDTH)
  {
    display.fillRect(0, graphYStart, SCREEN_WIDTH, graphHeight + 1, SSD1306_BLACK);
    graphX = 0;
  }

  int y = map(sensorValue, 0, 4096, graphYStart + graphHeight, graphYStart);
  display.drawLine(graphX, lastY, graphX + 1, y, SSD1306_WHITE);
  lastY = y;
  graphX++;

  display.display(); // Only call once per update cycle
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_CONNECTED:
    Serial.printf("Client [%u] connected\n", num);
    clientConnected = true;
    break;

  case WStype_DISCONNECTED:
    Serial.printf("Client [%u] disconnected\n", num);
    clientConnected = false;
    break;

  case WStype_TEXT:
    Serial.printf("Client [%u] sent text: %s\n", num, payload);
    break;

  default:
    break;
  }
}

void setup()
{
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(led, OUTPUT);

  Serial.begin(115200);

  // Initialize display first
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("SSD1306 allocation failed");
    for (;;)
      ;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting to WiFi...");
  display.display();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  // WiFi.begin(ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi not connected");
    display.display();
  }

  // Once connected
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  handleButton();

  webSocket.loop();

  sensorValue = analogRead(sensorPin);
  // Serial.println(sensorValue);

  // Send WebSocket JSON data
  if (readState == HIGH)
  {
    updateGraph(); // Handles both text and graph
    String data = "{\"value\":\"" + String(sensorValue) + "\"}";
    webSocket.broadcastTXT(data);
  }
  else
  {
    String data = "{\"value\":\"" + String("-") + "\"}";
    webSocket.broadcastTXT(data);
  }

  delay(50); // Small delay to prevent I2C overload
}
