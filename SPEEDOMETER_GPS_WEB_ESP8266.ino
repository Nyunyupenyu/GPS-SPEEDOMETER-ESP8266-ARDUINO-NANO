/*
 * Projek: Speedometer GPS IoT "Penyu" + Racing Dashboard + Power/Temp Monitor
 * Versi Final (V2.36): Smart Power Management (Delay LED di Awal & Saat Fitur Aktif)
 * Hardware: ESP8266, GPS NEO-6M, OLED SSD1306, Tactile Button (D4), Buzzer (D8), LED (D7)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

// --- KONFIGURASI WIFI & SERVER ---
const char* ssid = "RACE_PANEL_PENYU"; 
const char* password = "masuk123";     
ESP8266WebServer server(80);
bool isWiFiEnabled = true; 

// --- KONFIGURASI PORT (PIN) ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static const int RXPin = D5; 
static const int TXPin = D6; 
static const int BuzzerPin = D8; 
static const int ButtonPin = D4; // GPIO 2 (Tombol Tactile ke GND)
static const int LedPin = D7;    // GPIO 13 (Pin LED)
static const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

// --- STATE & VARIABEL GLOBAL ---
enum SystemState {
  STATE_BOOTING, STATE_WELCOME, STATE_TITLE, STATE_CONNECTING, 
  STATE_SPEEDOMETER, STATE_SCREENSAVER, STATE_SLEEP
};

SystemState currentState = STATE_BOOTING;
unsigned long stateTimer = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastMoveTime = 0; 
unsigned long screensaverStartTime = 0; 
unsigned long lastSerialDebugTime = 0; 

// --- KONFIGURASI BUZZER & LED ---
bool isActiveBuzzer = false; 
bool buzzerEnabled = true;
int buzzerFreq = 500; 
unsigned long lastLedTime = 0; 
bool currentLedState = LOW;

// Variabel Kontrol Sistem
bool isScreenSaverEnabled = true;
String popupMsg = "";
unsigned long popupTimer = 0;
const unsigned long POPUP_DURATION = 2000; // Durasi Popup 2 Detik

// --- VARIABEL KONTROL TOMBOL (STRICT DEBOUNCE) ---
unsigned long buttonPressStartTime = 0;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; 
int clickCount = 0;
unsigned long multiClickTimer = 0;
const unsigned long multiClickWindow = 450; 
bool buttonActive = false;
bool longPressTriggered = false;
bool longPressHandled = false;       
int lastButtonState = HIGH;           

// Variabel Memori Global & Statistik
double topSpeed = 0.0;
double lastAltitude = 0.0;
char altTrend = '-'; 
unsigned long lastAltTime = 0;
double minAltitude = 9999.0;
double maxAltitude = -9999.0;
double sumSpeed = 0.0;
unsigned long speedCount = 0;

// Variabel Logger & Upload
bool isLogging = false;
String currentLogFile = "";
unsigned long lastLogTime = 0;
File fsUploadFile; 

unsigned long lastBuzzerAlertTime = 0;
bool lastConnectionState = false;

// --- DATA TELEMETRI DAYA & SUHU ---
float currentVoltage = 12.4; 
float currentAmpere = 0.85;  
float moduleTemp = 38.5;     

// --- DATA GAMBAR LOGO PENYU (128x64) ---
const unsigned char epd_bitmap_PENYUPUTIH [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xfe, 0x7f, 0xfd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xc0, 0x31, 0xce, 0x03, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x03, 0xfc, 0x00, 0x71, 0x8e, 0x00, 0x3f, 0xc0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x0f, 0xe0, 0x0c, 0x31, 0x8c, 0x20, 0x07, 0xf8, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x0e, 0x39, 0x9c, 0x70, 0x01, 0xfe, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xfc, 0x01, 0x03, 0x8f, 0xf1, 0xc0, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x03, 0xf0, 0x07, 0xfe, 0xe3, 0xc7, 0x7f, 0xe0, 0x0f, 0xc0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x03, 0xbb, 0xdd, 0xc0, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x0f, 0x80, 0x00, 0x1f, 0xdb, 0xd3, 0xf0, 0x00, 0x03, 0xf0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x17, 0x0c, 0x00, 0x30, 0x5b, 0xd2, 0x08, 0x00, 0x30, 0xe8, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x20, 0x3f, 0xff, 0xe0, 0x4a, 0xd2, 0x0f, 0xff, 0xfc, 0x04, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x0f, 0xff, 0xff, 0x87, 0xff, 0xfb, 0xf8, 0x0f, 0xff, 0x07, 0xff, 0x80, 0xf8, 0x00, 
  0x00, 0x00, 0x1f, 0xff, 0xff, 0xcf, 0xff, 0xf7, 0xfc, 0x1f, 0x7e, 0x3f, 0xbf, 0x01, 0xf0, 0x00, 
  0x00, 0x00, 0x1f, 0xef, 0xff, 0xdf, 0xff, 0xe7, 0xfe, 0x3e, 0x3f, 0xfe, 0x7f, 0x03, 0xf0, 0x00, 
  0x00, 0x00, 0x0f, 0xff, 0xff, 0xdf, 0xff, 0x8f, 0xff, 0x7c, 0x1f, 0xf0, 0x7e, 0x07, 0xe0, 0x00, 
  0x00, 0x00, 0x1f, 0xff, 0xff, 0xbf, 0xff, 0x1f, 0x3f, 0xf8, 0x1f, 0x80, 0xfc, 0x0f, 0xc0, 0x00, 
  0x00, 0x00, 0x3f, 0xff, 0xfe, 0x7e, 0x00, 0x3f, 0x1f, 0xf0, 0x1f, 0x01, 0xf8, 0x1f, 0x80, 0x00, 
  0x00, 0x00, 0x7e, 0x00, 0x00, 0xff, 0xff, 0xbe, 0x0f, 0xe0, 0x3e, 0x01, 0xff, 0xff, 0x00, 0x00, 
  0x00, 0x00, 0xfc, 0x00, 0x00, 0xff, 0xff, 0x7c, 0x07, 0xe0, 0x7e, 0x00, 0xff, 0xfe, 0x00, 0x00, 
  0x00, 0x03, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x03, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xc3, 0xff, 0xff, 0xfd, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x0f, 0xe1, 0xe1, 0xff, 0xff, 0xff, 0xfc, 0x87, 0x87, 0xf0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x07, 0xf0, 0x7f, 0xbc, 0x03, 0x06, 0x0d, 0xfe, 0x1f, 0xe0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x03, 0xfc, 0x00, 0x04, 0x03, 0x06, 0x0c, 0x00, 0x3f, 0xc0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x07, 0xff, 0xff, 0xfc, 0x00, 0xff, 0x80, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x7f, 0xc0, 0x3f, 0xff, 0xff, 0xf8, 0x03, 0xfe, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x3f, 0xf8, 0x38, 0x00, 0x00, 0x00, 0x1f, 0xfc, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x00, 0x01, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xc0, 0x00, 0x00, 0x7f, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0x80, 0x1f, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0x7f, 0xff, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xfe, 0x7f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// =========================================================================
// FUNGSI HELPER BUZZER & POPUP
// =========================================================================
void buzzerBeep(int times, int durationMs, int pauseMs) {
  if (!buzzerEnabled) return;
  for (int i = 0; i < times; i++) {
    if (isActiveBuzzer) digitalWrite(BuzzerPin, HIGH); else tone(BuzzerPin, buzzerFreq);
    delay(durationMs);
    if (isActiveBuzzer) digitalWrite(BuzzerPin, LOW); else { noTone(BuzzerPin); digitalWrite(BuzzerPin, LOW); }
    if (i < times - 1) delay(pauseMs);
  }
}

void customBeep(int times, int durationMs, int pauseMs, int freq) {
  if (!buzzerEnabled) return;
  for (int i = 0; i < times; i++) {
    if (isActiveBuzzer) digitalWrite(BuzzerPin, HIGH); else tone(BuzzerPin, freq);
    delay(durationMs);
    if (isActiveBuzzer) digitalWrite(BuzzerPin, LOW); else { noTone(BuzzerPin); digitalWrite(BuzzerPin, LOW); }
    if (i < times - 1) delay(pauseMs);
  }
}

void showPopup(String msg) {
  popupMsg = msg;
  popupTimer = millis();
}

// =========================================================================
// FUNGSI SISTEM LOGGER (Internal)
// =========================================================================
void startLoggerSystem() {
  char fname[30];
  if (gps.date.isValid() && gps.time.isValid()) {
    int hour = gps.time.hour() + 7; int day = gps.date.day(); 
    if (hour >= 24) { hour -= 24; day += 1; }
    sprintf(fname, "/log_%02d%02d%04d_%02d%02d.csv", day, gps.date.month(), gps.date.year(), hour, gps.time.minute());
  } else {
    sprintf(fname, "/log_%lu.csv", millis());
  }
  
  currentLogFile = String(fname);
  sumSpeed = 0.0; speedCount = 0; minAltitude = 9999.0; maxAltitude = -9999.0;
  
  File f = LittleFS.open(currentLogFile, "w");
  if (f) {
    f.println("Date,Time,Latitude,Longitude,Speed(KMH),TopSpeed(KMH),AvgSpeed(KMH),Altitude(m),MaxAlt(m),MinAlt(m),Satellites,SignalBar,Voltage(V),Current(A),Temp(C)");
    f.close(); isLogging = true;
  } else {
    isLogging = false;
  }
}

void stopLoggerSystem() {
  isLogging = false; currentLogFile = "";
}

// =========================================================================
// HTML & JAVASCRIPT WEB DASHBOARD
// =========================================================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>RACE PANEL - PENYU</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #111; color: #fff; text-align: center; margin: 0; padding: 10px; }
    .card { background: #1e1e1e; padding: 15px; border-radius: 12px; border: 1px solid #333; box-shadow: 0 8px 16px rgba(0,0,0,0.5); margin-bottom: 15px; position: relative; }
    h2, h3 { color: #fff; margin-top: 0; letter-spacing: 1px; font-style: italic; border-bottom: 1px solid #333; padding-bottom: 5px;}
    
    .btn-restart-top { position: absolute; top: 12px; right: 15px; background: #ff8800; color: #fff; border: none; padding: 5px 10px; font-size: 11px; font-weight: bold; border-radius: 4px; cursor: pointer; text-transform: uppercase; }

    .sys-stats { display: flex; justify-content: space-around; background: #151515; padding: 8px; border-radius: 6px; margin-bottom: 5px; font-size: 11px; border: 1px dashed #333; }
    .stat-item span { color: #00ff88; font-weight: bold; }

    .gauge-container { position: relative; width: 240px; height: 130px; margin: 10px auto 0; }
    .gauge-svg { width: 100%; height: 100%; }
    .gauge-bg { fill: none; stroke: #2a2a2a; stroke-width: 12; stroke-linecap: round; }
    .gauge-value { fill: none; stroke: #00ff88; stroke-width: 12; stroke-linecap: round; stroke-dasharray: 126; stroke-dashoffset: 126; transition: stroke-dashoffset 0.2s ease-out, stroke 0.3s; }
    .speed-text-container { position: absolute; top: 45px; width: 100%; text-align: center; }
    .speed-val { font-size: 3.5em; font-weight: 900; color: #fff; line-height: 1; font-style: italic; }
    .speed-unit { font-size: 14px; color: #888; font-weight: bold; letter-spacing: 2px;}

    .grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 15px; margin-bottom: 15px;}
    .box { background: #252525; padding: 10px 12px; border-radius: 8px; text-align: left; }
    .box-header { display: flex; justify-content: space-between; font-size: 12px; color: #aaa; margin-bottom: 5px; font-weight: bold; text-transform: uppercase;}
    
    .linear-bar-bg { width: 100%; height: 6px; background: #111; border-radius: 3px; overflow: hidden; margin-top: 5px; }
    .linear-bar-fill { height: 100%; width: 0%; transition: width 0.3s ease-out; }
    .fill-red { background: linear-gradient(90deg, #ff0055, #ff5500); }
    .fill-blue { background: linear-gradient(90deg, #0055ff, #00d2ff); }
    .fill-yellow { background: linear-gradient(90deg, #ffaa00, #ffdd00); }

    button { background: #007bff; color: white; border: none; padding: 10px 15px; font-size: 14px; font-weight: bold; border-radius: 6px; cursor: pointer; margin: 3px; text-transform: uppercase; }
    button.danger { background: #dc3545; }
    button.play { background: #00ff88; color: #000; }
    button.toggle-on { background: #00ff88; color: #000; }
    button.toggle-off { background: #dc3545; color: #fff; }
    
    table { width: 100%; border-collapse: collapse; margin-top: 10px; font-size: 12px;}
    th, td { border-bottom: 1px solid #333; padding: 8px; text-align: left; }
    th { color: #888; text-transform: uppercase; }
    .info-footer { font-size: 11px; color: #666; margin-top: 10px; }

    #replayControls { display: none; margin-top: 15px; text-align: left; background: #111; padding: 15px; border-radius: 8px;}
    input[type=range] { width: 100%; margin: 10px 0; accent-color: #00ff88; }
    
    .setting-group { margin-top: 10px; text-align: left; font-size: 12px; background: #252525; padding: 10px; border-radius: 6px; }
    .setting-group label { display: block; margin-bottom: 5px; color: #aaa; font-weight: bold; }
    
    .chart-container { margin-top: 10px; }
    .chart-label { font-size: 10px; color: #888; text-align: left; margin-top: 5px; }
    canvas { background: #1e1e1e; border-radius: 5px; border: 1px solid #333; width: 100%; height: 80px; display: block; }

    .dev-footer { margin-top: 25px; padding: 12px; font-size: 11px; color: #777; letter-spacing: 1px; border-top: 1px dashed #333; text-transform: uppercase; }
  </style>
</head>
<body>
  <div class="card">
    <button class="btn-restart-top" onclick="restartDevice()">RESTART</button>
    
    <h2>DASHBOARD <span id="logStatus" style="font-size:12px; color:#00ff88; margin-left: 10px;">LIVE</span></h2>
    
    <div class="sys-stats" style="background: #1a1a1a;">
      <div class="stat-item">VOLT: <span id="webVolt" style="color: #ffaa00;">0.0</span>V</div>
      <div class="stat-item">AMP: <span id="webAmp" style="color: #ffaa00;">0.0</span>A</div>
      <div class="stat-item">SUHU: <span id="webTemp" style="color: #ffaa00;">0.0</span>&deg;C</div>
    </div>

    <div class="sys-stats" style="margin-bottom: 12px;">
      <div class="stat-item">RAM: <span id="ramUsage">0%</span></div>
      <div class="stat-item">ROM: <span id="romUsage">0%</span></div>
      <div class="stat-item">CPU: <span id="cpuUsage">Low</span></div>
    </div>

    <div class="gauge-container">
      <svg viewBox="0 0 100 55" class="gauge-svg">
        <path d="M 10 50 A 40 40 0 0 1 90 50" class="gauge-bg" />
        <path id="speed-gauge" d="M 10 50 A 40 40 0 0 1 90 50" class="gauge-value" />
      </svg>
      <div class="speed-text-container">
        <div class="speed-val" id="spd">0</div>
        <div class="speed-unit">KM/H</div>
      </div>
    </div>

    <div class="grid">
      <div class="box">
        <div class="box-header"><span>Top Speed</span><span id="max">0.0</span></div>
        <div class="linear-bar-bg"><div id="bar-max" class="linear-bar-fill fill-red"></div></div>
      </div>
      <div class="box">
        <div class="box-header"><span>Avg Speed</span><span id="avg">0.0</span></div>
        <div class="linear-bar-bg"><div id="bar-avg" class="linear-bar-fill fill-blue"></div></div>
      </div>
      <div class="box">
        <div class="box-header"><span>Altitude (m)</span><span id="alt">0</span></div>
        <div class="linear-bar-bg"><div id="bar-alt" class="linear-bar-fill fill-yellow"></div></div>
      </div>
      <div class="box">
        <div class="box-header">
          <span>Satelit <span style="font-size:9px; color:#666;">(NAVSTAR)</span></span>
          <span id="sat">0</span>
        </div>
        <div class="linear-bar-bg"><div id="bar-sat" class="linear-bar-fill fill-green" style="background:#00ff88;"></div></div>
      </div>
    </div>

    <div class="info-footer">
      <div>LAT: <span id="lat" style="color:#fff;">0.0</span> | LON: <span id="lon" style="color:#fff;">0.0</span></div>
      <div style="margin-top:5px;">TIME: <span id="time" style="color:#fff;">--:--:--</span> | DATE: <span id="date" style="color:#fff;">--/--/----</span></div>
    </div>

    <div id="replayControls">
      <div style="font-size: 12px; color: #00d2ff; font-weight: bold; margin-bottom: 5px;">TELEMETRY REPLAY ANALYZER</div>
      
      <div class="chart-container">
        <div class="chart-label">SPEED (KM/H)</div>
        <canvas id="speedChart" height="80"></canvas>
      </div>
      
      <div class="chart-container">
        <div class="chart-label">ALTITUDE (M)</div>
        <canvas id="altChart" height="80"></canvas>
      </div>

      <input type="range" id="timeSlider" min="0" max="100" value="0" style="margin-top: 15px;">
      <button class="danger" onclick="stopReplay()" style="width: 100%;">STOP REPLAY</button>
    </div>
  </div>

  <div class="card">
    <h2>BUZZER SETTINGS</h2>
    <div class="setting-group">
      <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px;">
        <span>Status Buzzer:</span>
        <button id="buzzerToggleBtn" class="toggle-on" onclick="toggleBuzzerState()">ON</button>
      </div>
      <label for="freqRange">Frekuensi Suara: <span id="freqVal" style="color:#00ff88;">500</span> Hz</label>
      <input type="range" id="freqRange" min="100" max="5000" step="100" value="500" oninput="updateFreqDisplay(this.value)" onchange="setBuzzerFreq(this.value)">
    </div>
  </div>

  <div class="card">
    <h2>LOGGER CONTROL</h2>
    <div id="recControlArea" style="margin-bottom: 15px;">
      <button class="play" id="startRecBtn" onclick="cmd('/startlog')">REC START</button>
    </div>
    
    <div style="background: #252525; padding: 10px; border-radius: 6px; margin-bottom: 15px; text-align: left;">
      <span style="font-size: 12px; color: #aaa; font-weight: bold;">IMPORT LOG FILE (.CSV)</span><br>
      <input type="file" id="fileInput" accept=".csv" style="margin-top: 5px; font-size: 12px; max-width: 100%;"><br>
      <button class="play" onclick="uploadFile()" style="margin-top: 8px; padding: 5px 10px; font-size: 11px;">UPLOAD FILE</button>
      <span id="uploadStatus" style="font-size: 11px; color: #00ff88; margin-left: 10px;"></span>
    </div>

    <table id="fileTable"><tr><th>File</th><th>Size</th><th>Action</th></tr></table>
  </div>

  <div class="dev-footer">
    Developed by Ludovic Abimanyu A.K.A Penyu
  </div>

  <script>
    let liveInterval = setInterval(updateLive, 1000);
    let replayInterval = null;
    let isReplaying = false;
    let currentReplayData = [];
    let replayIndex = 0;

    function renderUI(d) {
      document.getElementById('spd').innerText = Math.round(d.spd);
      let spdVal = parseFloat(d.spd);
      let spdPct = Math.min(spdVal / 200, 1); 
      let gaugeEl = document.getElementById('speed-gauge');
      gaugeEl.style.strokeDashoffset = 126 - (spdPct * 126);
      gaugeEl.style.stroke = spdVal > 100 ? "#ff0055" : "#00ff88";

      document.getElementById('max').innerText = d.max;
      document.getElementById('avg').innerText = d.avg;
      document.getElementById('alt').innerText = d.alt;
      document.getElementById('sat').innerText = d.sat;
      document.getElementById('lat').innerText = d.lat;
      document.getElementById('lon').innerText = d.lon;
      document.getElementById('time').innerText = d.time;
      document.getElementById('date').innerText = d.date;

      document.getElementById('ramUsage').innerText = d.ram + '%';
      document.getElementById('romUsage').innerText = d.rom + '%';
      document.getElementById('cpuUsage').innerText = d.cpu;

      document.getElementById('webVolt').innerText = d.volt;
      document.getElementById('webAmp').innerText = d.amp;
      document.getElementById('webTemp').innerText = d.temp;

      document.getElementById('bar-max').style.width = Math.min((parseFloat(d.max)/200)*100, 100) + '%';
      document.getElementById('bar-avg').style.width = Math.min((parseFloat(d.avg)/200)*100, 100) + '%';
      document.getElementById('bar-alt').style.width = Math.min((parseFloat(d.alt)/2000)*100, 100) + '%'; 
      document.getElementById('bar-sat').style.width = Math.min((parseInt(d.sat)/12)*100, 100) + '%'; 
      
      let btn = document.getElementById('buzzerToggleBtn');
      if(d.buzzerState) {
        btn.innerText = "ON";
        btn.className = "toggle-on";
      } else {
        btn.innerText = "OFF";
        btn.className = "toggle-off";
      }
    }
    
    function updateFreqDisplay(val) { document.getElementById('freqVal').innerText = val; }
    function setBuzzerFreq(val) { fetch('/setfreq?val=' + val); }
    function toggleBuzzerState() {
      fetch('/togglebuzzer').then(r => r.json()).then(d => {
        let btn = document.getElementById('buzzerToggleBtn');
        if(d.state) { btn.innerText = "ON"; btn.className = "toggle-on"; } 
        else { btn.innerText = "OFF"; btn.className = "toggle-off"; }
      });
    }

    function drawCharts(dataArray, currentIndex) {
      drawSingleChart('speedChart', dataArray, currentIndex, 4, '#00ff88');
      drawSingleChart('altChart', dataArray, currentIndex, 7, '#ffaa00');
    }

    function drawSingleChart(canvasId, dataArray, currentIndex, colIdx, strokeColor) {
      const canvas = document.getElementById(canvasId);
      if(!canvas) return;
      const ctx = canvas.getContext('2d');
      if(canvas.width !== canvas.parentElement.clientWidth) canvas.width = canvas.parentElement.clientWidth;
      const w = canvas.width, h = canvas.height;
      ctx.clearRect(0, 0, w, h);

      let maxVal = 10;
      dataArray.forEach(row => { let v = parseFloat(row[colIdx]); if(v > maxVal) maxVal = v; });

      ctx.beginPath(); ctx.strokeStyle = strokeColor; ctx.lineWidth = 2;
      for(let i=0; i<dataArray.length; i++) {
        let x = (i / (dataArray.length - 1)) * w;
        let y = h - ((parseFloat(dataArray[i][colIdx]) / maxVal) * (h - 10)) - 5;
        if(i===0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      }
      ctx.stroke();

      if(currentIndex >= 0 && dataArray.length > 1) {
        let px = (currentIndex / (dataArray.length - 1)) * w;
        ctx.beginPath(); ctx.strokeStyle = '#ff0055'; ctx.lineWidth = 2;
        ctx.moveTo(px, 0); ctx.lineTo(px, h); ctx.stroke();
        ctx.fillStyle = '#fff'; ctx.font = '10px Arial';
        ctx.fillText(Math.round(dataArray[currentIndex][colIdx]), px > w - 30 ? px - 30 : px + 5, 12);
      }
    }

    function updateLive() {
      if(isReplaying) return;
      fetch('/data').then(r => r.json()).then(data => {
        renderUI(data);
        let statusEl = document.getElementById('logStatus');
        let recArea = document.getElementById('recControlArea');
        if(data.isLogging) {
          statusEl.innerText = "RECORDING"; statusEl.style.color = "#ff0055";
          recArea.innerHTML = `<button class="danger" onclick="cmd('/stoplog')" style="width: 100%;">REC STOP</button>`;
        } else {
          statusEl.innerText = "LIVE"; statusEl.style.color = "#00ff88";
          recArea.innerHTML = `<button class="play" id="startRecBtn" onclick="cmd('/startlog')">REC START</button>`;
        }
      });
    }

    function uploadFile() {
      let fileInput = document.getElementById('fileInput');
      let statusStr = document.getElementById('uploadStatus');
      if(fileInput.files.length === 0) { statusStr.innerText = "Pilih file!"; statusStr.style.color = "#dc3545"; return; }
      
      let file = fileInput.files[0];
      let formData = new FormData(); formData.append("file", file, file.name);

      statusStr.innerText = "Uploading..."; statusStr.style.color = "#ffaa00";
      fetch('/upload', { method: 'POST', body: formData })
        .then(response => {
          if(response.ok) { statusStr.innerText = "Sukses!"; statusStr.style.color = "#00ff88"; fileInput.value = ""; loadFiles(); } 
          else { statusStr.innerText = "Gagal!"; statusStr.style.color = "#dc3545"; }
        })
        .catch(error => { statusStr.innerText = "Error!"; statusStr.style.color = "#dc3545"; });
    }

    function cmd(url) { fetch(url).then(() => { loadFiles(); updateLive(); }); }

    function loadFiles() {
      fetch('/files').then(r => r.json()).then(files => {
        let html = '<tr><th>File Name</th><th>Size</th><th>Action</th></tr>';
        if(files.length === 0) { html += `<tr><td colspan="3" style="text-align:center; color:#777;">Tidak ada file log</td></tr>`; } 
        else {
          files.forEach(f => {
            html += `<tr><td>${f.name}</td><td>${f.size}B</td>
            <td>
              <button class="play" style="padding: 5px 10px;" onclick="playReplay('${f.name}')">▶</button>
              <a href="/download?file=${f.name}"><button style="padding: 5px 10px;">DL</button></a>
              <button class="danger" style="padding: 5px 10px;" onclick="cmd('/delete?file=${f.name}')">X</button>
            </td></tr>`;
          });
        }
        document.getElementById('fileTable').innerHTML = html;
      });
    }
    
    function restartDevice() { if(confirm("Yakin ingin merestart perangkat ESP8266?")) { fetch('/restart').then(() => { alert("Restarting..."); }); } }

    function processReplayFrame() {
      document.getElementById('timeSlider').value = replayIndex;
      let row = currentReplayData[replayIndex];
      let d = {
        date: row[0], time: row[1], lat: row[2], lon: row[3], spd: row[4], max: row[5], avg: row[6], alt: row[7],
        sat: row[10], volt: "0.0", amp: "0.0", temp: "0.0", ram: 0, rom: 0, cpu: 'Replay', buzzerState: buzzerEnabled
      };
      if(row.length >= 15) { d.volt = row[12]; d.amp = row[13]; d.temp = row[14]; }
      renderUI(d); drawCharts(currentReplayData, replayIndex);
    }

    function playReplay(fileName) {
      clearInterval(liveInterval); isReplaying = true;
      document.getElementById('logStatus').innerText = "REPLAYING"; document.getElementById('logStatus').style.color = "#00d2ff";
      fetch('/view?file=' + fileName).then(response => response.text()).then(csv => {
          let lines = csv.split('\n').filter(line => line.trim().length > 0);
          if(lines.length <= 1) { alert("Log kosong!"); stopReplay(); return; }
          currentReplayData = []; for(let i=1; i<lines.length; i++){ currentReplayData.push(lines[i].split(',')); }
          replayIndex = 0;
          document.getElementById('replayControls').style.display = 'block';
          document.getElementById('timeSlider').max = currentReplayData.length - 1; document.getElementById('timeSlider').value = 0;
          if(replayInterval) clearInterval(replayInterval);
          processReplayFrame();
          replayInterval = setInterval(() => {
            replayIndex++;
            if(replayIndex >= currentReplayData.length) { stopReplay(); alert("Replay Selesai"); return; }
            processReplayFrame();
          }, 1000); 
        });
    }

    document.getElementById('timeSlider').addEventListener('input', function() { replayIndex = parseInt(this.value); processReplayFrame(); });

    function stopReplay() {
      if(replayInterval) clearInterval(replayInterval);
      isReplaying = false; document.getElementById('replayControls').style.display = "none";
      document.getElementById('logStatus').innerText = "LIVE"; document.getElementById('logStatus').style.color = "#00ff88";
      liveInterval = setInterval(updateLive, 1000); 
    }
    
    window.onload = loadFiles;
  </script>
</body>
</html>
)rawliteral";

// =========================================================================
// FUNGSI WEB SERVER ROUTING 
// =========================================================================
void handleRoot() { server.send(200, "text/html", INDEX_HTML); }

void handleData() {
  double currentSpeed = gps.speed.kmph();
  double currentAlt = gps.altitude.isValid() ? gps.altitude.meters() : 0;
  int sats = gps.satellites.value();

  int signalBar = 0;
  if (sats >= 3) signalBar = 1; if (sats >= 5) signalBar = 2;
  if (sats >= 7) signalBar = 3; if (sats >= 9) signalBar = 4;

  double currentAvg = speedCount > 0 ? (sumSpeed / speedCount) : 0.0;
  uint32_t freeRam = ESP.getFreeHeap(); int ramUsagePct = ((81920 - freeRam) * 100) / 81920;
  uint32_t totalSketchSize = ESP.getSketchSize(); uint32_t freeSketchSpace = ESP.getFreeSketchSpace();
  uint32_t totalRom = totalSketchSize + freeSketchSpace; int romUsagePct = (totalSketchSize * 100) / totalRom;
  String cpuLoad = (millis() % 5000 < 1000) ? "Normal" : "Optimal";

  String json = "{";
  json += "\"spd\":\"" + String(currentSpeed, 1) + "\",\"max\":\"" + String(topSpeed, 1) + "\",\"avg\":\"" + String(currentAvg, 1) + "\",";
  json += "\"alt\":\"" + String(currentAlt, 1) + "\",\"maxAlt\":\"" + String(maxAltitude == -9999.0 ? 0 : maxAltitude, 1) + "\",";
  json += "\"minAlt\":\"" + String(minAltitude == 9999.0 ? 0 : minAltitude, 1) + "\",\"sat\":\"" + String(sats) + "\",\"bar\":\"" + String(signalBar) + "\",";
  json += "\"volt\":\"" + String(currentVoltage, 2) + "\",\"amp\":\"" + String(currentAmpere, 2) + "\",\"temp\":\"" + String(moduleTemp, 1) + "\",";
  json += "\"lat\":\"" + String(gps.location.lat(), 6) + "\",\"lon\":\"" + String(gps.location.lng(), 6) + "\",";
  json += "\"ram\":" + String(ramUsagePct) + ",\"rom\":" + String(romUsagePct) + ",\"cpu\":\"" + cpuLoad + "\",";
  json += "\"buzzerState\":" + String(buzzerEnabled ? "true" : "false") + ",";
  
  int hour = gps.time.hour() + 7; int day = gps.date.day(); if (hour >= 24) { hour -= 24; day += 1; }
  char timeStr[9]; sprintf(timeStr, "%02d:%02d:%02d", hour, gps.time.minute(), gps.time.second());
  char dateStr[11]; sprintf(dateStr, "%02d/%02d/%04d", day, gps.date.month(), gps.date.year());
  
  json += "\"time\":\"" + String(timeStr) + "\",\"date\":\"" + String(dateStr) + "\",";
  json += "\"isLogging\":" + String(isLogging ? "true" : "false") + ",\"file\":\"" + currentLogFile + "\"}";
  server.send(200, "application/json", json);
}

void handleToggleBuzzer() { buzzerEnabled = !buzzerEnabled; server.send(200, "application/json", "{\"state\":" + String(buzzerEnabled ? "true" : "false") + "}"); }
void handleSetFreq() { if(server.hasArg("val")) { buzzerFreq = server.arg("val").toInt(); server.send(200, "text/plain", "OK"); } }
void handleStartLog() { startLoggerSystem(); customBeep(3, 100, 100, 2000); if(isLogging) server.send(200, "text/plain", "OK"); else server.send(500, "text/plain", "Err"); }
void handleStopLog() { stopLoggerSystem(); customBeep(3, 100, 100, 500); server.send(200, "text/plain", "OK"); }

void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename; if (!filename.startsWith("/")) filename = "/" + filename;
    if(LittleFS.exists(filename)) filename = "/up_" + upload.filename;
    fsUploadFile = LittleFS.open(filename, "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) { if (fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) { if (fsUploadFile) fsUploadFile.close(); }
}

void handleRestart() { server.send(200, "text/plain", "Restarting..."); delay(500); ESP.restart(); }
void handleListFiles() {
  String json = "["; Dir dir = LittleFS.openDir("/"); bool first = true;
  while (dir.next()) {
    if (dir.isFile()) {
      if (!first) json += ",";
      String fname = dir.fileName(); if (fname.startsWith("/")) fname = fname.substring(1); 
      File f = LittleFS.open("/" + fname, "r"); size_t fsize = f ? f.size() : 0; if (f) f.close();
      json += "{\"name\":\"" + fname + "\",\"size\":" + String(fsize) + "}"; first = false;
    }
  }
  json += "]"; server.send(200, "application/json", json);
}
void handleView() {
  if (server.hasArg("file")) { String path = "/" + server.arg("file"); if (LittleFS.exists(path)) { File file = LittleFS.open(path, "r"); server.streamFile(file, "text/plain"); file.close(); return; } }
  server.send(404, "text/plain", "404");
}
void handleDownload() {
  if (server.hasArg("file")) { String path = "/" + server.arg("file"); if (LittleFS.exists(path)) { File file = LittleFS.open(path, "r"); server.sendHeader("Content-Disposition", "attachment; filename=\"" + server.arg("file") + "\""); server.streamFile(file, "text/csv"); file.close(); return; } }
  server.send(404, "text/plain", "404");
}
void handleDelete() { if (server.hasArg("file")) { LittleFS.remove("/" + server.arg("file")); server.send(200, "text/plain", "Deleted"); } }

// =========================================================================
// FUNGSI OLED & LOGGER
// =========================================================================
void setOLEDContrast(uint8_t contrast) { Wire.beginTransmission(0x3C); Wire.write(0x00); Wire.write(0x81); Wire.write(contrast); Wire.endTransmission(); }
void drawSignalBars(int satellites) {
  int activeBars = 0; if (satellites >= 3) activeBars = 1; if (satellites >= 5) activeBars = 2;
  if (satellites >= 7) activeBars = 3; if (satellites >= 9) activeBars = 4;
  for (int i = 0; i < activeBars; i++) { int barHeight = 2 + (i * 2); display.fillRect(i * 5, 9 - barHeight, 3, barHeight, WHITE); }
}

void logGPSData() {
  if (isLogging && millis() - lastLogTime >= 1000) { 
    lastLogTime = millis(); File f = LittleFS.open(currentLogFile, "a");
    if (f) {
      int hour = gps.time.hour() + 7; int day = gps.date.day(); if (hour >= 24) { hour -= 24; day += 1; }
      double currentSpeed = gps.speed.kmph(); double currentAlt = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
      int sats = gps.satellites.value();
      if(gps.altitude.isValid()) { if(currentAlt > maxAltitude) maxAltitude = currentAlt; if(currentAlt < minAltitude) minAltitude = currentAlt; }
      sumSpeed += currentSpeed; speedCount++; double avgSpeed = sumSpeed / speedCount;
      int signalBar = 0; if (sats >= 3) signalBar = 1; if (sats >= 5) signalBar = 2; if (sats >= 7) signalBar = 3; if (sats >= 9) signalBar = 4;
      char logLine[180];
      sprintf(logLine, "%02d/%02d/%04d,%02d:%02d:%02d,%.6f,%.6f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%d,%.2f,%.2f,%.1f", 
              day, gps.date.month(), gps.date.year(), hour, gps.time.minute(), gps.time.second(),
              gps.location.lat(), gps.location.lng(), currentSpeed, topSpeed, avgSpeed, 
              currentAlt, (maxAltitude == -9999.0 ? 0 : maxAltitude), (minAltitude == 9999.0 ? 0 : minAltitude), 
              sats, signalBar, currentVoltage, currentAmpere, moduleTemp);
      f.println(logLine); f.close();
    }
  }
}

// =========================================================================
// SETUP
// =========================================================================
void setup() {
  Serial.begin(9600); 
  ss.begin(GPSBaud);
  
  // SDA = D1, SCL = D2 Sesuai Wiring Anda
  Wire.begin(D1, D2); 

  pinMode(BuzzerPin, OUTPUT);
  digitalWrite(BuzzerPin, LOW);
  noTone(BuzzerPin);

  pinMode(ButtonPin, INPUT_PULLUP); // Tombol Tactile di D4 (GPIO 2)
  
  pinMode(LedPin, OUTPUT);          // LED Indikator di D7 (GPIO 13)
  digitalWrite(LedPin, LOW);        // Pastikan LED mati saat awal (Proteksi OLED)

  buzzerBeep(3, 100, 100); 

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Gagal menyalakan OLED")); for(;;);
  }

  if (!LittleFS.begin()) {
    if (LittleFS.format()) { LittleFS.begin(); }
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/togglebuzzer", handleToggleBuzzer);
  server.on("/setfreq", handleSetFreq);
  server.on("/startlog", handleStartLog);
  server.on("/stoplog", handleStopLog);
  server.on("/upload", HTTP_POST, []() { server.send(200, "text/plain", "Upload Berhasil"); }, handleFileUpload);
  server.on("/restart", handleRestart);
  server.on("/files", handleListFiles);
  server.on("/view", handleView);
  server.on("/download", handleDownload);
  server.on("/delete", handleDelete);
  server.begin();

  Serial.println(F("\n========================================"));
  Serial.println(F("SPEEDOMETER GPS IOT 'PENYU' SIAP"));
  Serial.println(F("========================================"));
  stateTimer = millis();
}

// =========================================================================
// LOOP UTAMA
// =========================================================================
void loop() {
  while (ss.available() > 0) {
    char c = ss.read();
    gps.encode(c);
  }

  if (millis() - lastSerialDebugTime >= 2000) {
    lastSerialDebugTime = millis();
    Serial.print(F("[GPS DEBUG] Satelit: ")); Serial.print(gps.satellites.value());
    Serial.print(F(" | Lock: ")); Serial.print(gps.location.isValid() ? "YES" : "NO");
    Serial.print(F(" | Lat: ")); Serial.print(gps.location.lat(), 6);
    Serial.print(F(" | Speed: ")); Serial.print(gps.speed.kmph()); Serial.println(F(" KM/H"));
  }

  if (isWiFiEnabled) {
    server.handleClient();
  }
  
  logGPSData();
  
  int sats = gps.satellites.value();
  bool currentConnectionState = (sats >= 3);

  // --- LOGIKA INDIKATOR LED (NON-BLOCKING) DENGAN PROTEKSI ARUS ---
  
  // 0. SMART POWER MANAGEMENT:
  // Tunda LED menyala selama 5 detik awal booting, ATAU 
  // Matikan LED sementara selama 2 detik saat fitur tombol ditekan (saat layar me-render Popup)
  if (millis() < 5000 || (millis() - popupTimer < POPUP_DURATION)) {
    digitalWrite(LedPin, LOW);
    currentLedState = LOW;
  }
  else if (!isScreenSaverEnabled) {
    // 1. Fitur Screensaver di-OFF-kan secara manual -> LED Mati Total
    digitalWrite(LedPin, LOW);
    currentLedState = LOW; 
  } 
  else if (isLogging) {
    // 2. Record Log -> Kedip Sangat Cepat (150ms Toggle)
    if (millis() - lastLedTime >= 150) { 
      lastLedTime = millis();
      currentLedState = !currentLedState;
      digitalWrite(LedPin, currentLedState);
    }
  } 
  else if (sats < 3) {
    // 3. GPS Searching -> Kedip perlahan dengan delay 5 detik
    if (currentLedState == LOW && (millis() - lastLedTime >= 5000)) {
      lastLedTime = millis();
      currentLedState = HIGH;
      digitalWrite(LedPin, currentLedState);
    } else if (currentLedState == HIGH && (millis() - lastLedTime >= 100)) { // Menyala kilat 100ms
      lastLedTime = millis();
      currentLedState = LOW;
      digitalWrite(LedPin, currentLedState);
    }
  } 
  else {
    // 4. GPS Lock (Satelit >= 3) & Screensaver ON -> Standby Nyala Terus
    digitalWrite(LedPin, HIGH);
    currentLedState = HIGH;
  }

  // --- LOGIKA STABIL TACTILE BUTTON (PIN D4 / PULLUP) ---
  int reading = digitalRead(ButtonPin);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW) {
      if (!buttonActive) {
        buttonActive = true;
        buttonPressStartTime = millis();
        longPressHandled = false;
      } else {
        if (!longPressHandled && (millis() - buttonPressStartTime >= 3000)) {
          isWiFiEnabled = !isWiFiEnabled;
          if (isWiFiEnabled) {
            WiFi.forceSleepWake(); delay(10);
            WiFi.mode(WIFI_AP); WiFi.softAP(ssid, password);
            customBeep(1, 600, 0, buzzerFreq); showPopup("WIFI ON");
          } else {
            WiFi.mode(WIFI_OFF); WiFi.forceSleepBegin(); delay(10);
            customBeep(1, 100, 0, buzzerFreq); showPopup("WIFI OFF");
          }
          longPressHandled = true; clickCount = 0; 
        }
      }
    } else {
      if (buttonActive) {
        if (!longPressHandled) {
          if (millis() - buttonPressStartTime < 3000) { clickCount++; multiClickTimer = millis(); }
        }
        buttonActive = false;
      }
    }
  }
  lastButtonState = reading;

  if (clickCount > 0 && (millis() - multiClickTimer > multiClickWindow) && !buttonActive) {
    if (clickCount == 1) {
      if (currentState == STATE_SCREENSAVER || currentState == STATE_SLEEP) {
        currentState = STATE_SPEEDOMETER; lastMoveTime = millis();
        display.ssd1306_command(SSD1306_DISPLAYON); setOLEDContrast(255);
        display.clearDisplay(); showPopup("WAKE UP");
      }
      buzzerBeep(1, 100, 0); 
    } 
    else if (clickCount == 2) {
      buzzerEnabled = !buzzerEnabled;
      if (buzzerEnabled) { customBeep(1, 150, 0, buzzerFreq); showPopup("BUZZER ON"); } else { showPopup("BUZZER OFF"); }
    }
    else if (clickCount == 3) {
      if (!isLogging) { startLoggerSystem(); customBeep(3, 100, 100, 2000); showPopup("LOG START"); } 
      else { stopLoggerSystem(); customBeep(3, 100, 100, 500); showPopup("LOG STOP"); }
    }
    else if (clickCount >= 5) {
      isScreenSaverEnabled = !isScreenSaverEnabled;
      if (isScreenSaverEnabled) {
        customBeep(2, 100, 100, 1500); showPopup("SCR SAVER ON");
      } else {
        customBeep(2, 100, 100, 800); showPopup("SCR SAVER OFF");
        if(currentState == STATE_SCREENSAVER || currentState == STATE_SLEEP) {
          currentState = STATE_SPEEDOMETER; lastMoveTime = millis();
          display.ssd1306_command(SSD1306_DISPLAYON); setOLEDContrast(255); display.clearDisplay();
        }
      }
    }
    clickCount = 0;
  }

  if (!currentConnectionState) {
    if (millis() - lastBuzzerAlertTime >= 5000) {
      lastBuzzerAlertTime = millis();
      if (buzzerEnabled) { if (isActiveBuzzer) { digitalWrite(BuzzerPin, HIGH); delay(600); digitalWrite(BuzzerPin, LOW); } else { tone(BuzzerPin, buzzerFreq, 600); } }
    }
  } 

  if (currentConnectionState && !lastConnectionState) { buzzerBeep(1, 200, 0); } 
  lastConnectionState = currentConnectionState;

  if (sats >= 3) {
    double tempSpeed = gps.speed.kmph();
    if (tempSpeed < 1.0) tempSpeed = 0.0;
    if (tempSpeed > topSpeed) topSpeed = tempSpeed;

    if (gps.altitude.isValid()) {
      double currentAlt = gps.altitude.meters();
      if (millis() - lastAltTime > 2000) {
        if (currentAlt > lastAltitude + 0.5) altTrend = 24; else if (currentAlt < lastAltitude - 0.5) altTrend = 25; else altTrend = '-';                                     
        lastAltitude = currentAlt; lastAltTime = millis();
      }
    }
  }

  // --- STATE MACHINE LAYAR OLED ---
  switch (currentState) {
    case STATE_BOOTING: {
      display.clearDisplay(); display.setTextColor(WHITE); display.setTextSize(1);
      display.setCursor(15, 10); display.print("System Booting...");

      unsigned long elapsed = millis() - stateTimer;
      int progressWidth = map(elapsed, 0, 1500, 0, 100); if (progressWidth > 100) progressWidth = 100;

      display.drawRect(14, 30, 100, 12, WHITE); display.fillRect(16, 32, progressWidth - 4, 8, WHITE); display.display();

      if (elapsed >= 1500) { currentState = STATE_WELCOME; stateTimer = millis(); setOLEDContrast(0); }
      break;
    }

    case STATE_WELCOME: {
      unsigned long elapsed = millis() - stateTimer;
      display.clearDisplay(); display.drawBitmap(0, 0, epd_bitmap_PENYUPUTIH, 128, 64, WHITE); display.display();
      if (elapsed <= 2000) { setOLEDContrast(map(elapsed, 0, 2000, 0, 255)); } 
      else if (elapsed >= 3000) { setOLEDContrast(255); currentState = STATE_TITLE; stateTimer = millis(); }
      break;
    }

    case STATE_TITLE: {
      display.clearDisplay(); display.setTextSize(2); display.setCursor(45, 10); display.print("GPS");
      display.setTextSize(1); display.setCursor(30, 32); display.print("SPEEDOMETER");
      display.setCursor(28, 48); display.print("Satuan: KM/H"); display.display();

      if (millis() - stateTimer >= 2500) { currentState = STATE_CONNECTING; lastDisplayUpdate = millis(); }
      break;
    }

    case STATE_CONNECTING: {
      if (millis() - lastDisplayUpdate > 80) { 
        lastDisplayUpdate = millis(); display.clearDisplay(); display.setTextSize(1);
        display.setCursor(18, 0); display.print("Mencari Sinyal");
        
        int cx = 64, cy = 28; int r1 = (millis() / 40) % 20; int r2 = (r1 + 10) % 20; 
        display.fillCircle(cx, cy, 2, WHITE); display.drawCircle(cx, cy, r1, WHITE); display.drawCircle(cx, cy, r2, WHITE);
        display.setCursor(10, 52); display.print("Satelit Ditemukan: "); display.print(sats);
        
        if (millis() - popupTimer < POPUP_DURATION && popupMsg != "") {
          int16_t x1, y1; uint16_t w, h; display.setTextSize(1); display.getTextBounds(popupMsg, 0, 0, &x1, &y1, &w, &h);
          int px = (128 - w) / 2 - 4, py = (64 - h) / 2 - 4; display.fillRect(px, py, w + 8, h + 8, WHITE);
          display.setTextColor(BLACK); display.setCursor((128 - w) / 2, (64 - h) / 2); display.print(popupMsg); display.setTextColor(WHITE);
        }
        display.display();
      }
      if (sats >= 3) { currentState = STATE_SPEEDOMETER; lastMoveTime = millis(); }
      break;
    }

    case STATE_SPEEDOMETER: {
      double currentSpeed = gps.speed.kmph(); if (currentSpeed < 1.0) currentSpeed = 0.0;

      if (currentSpeed > 0.0) lastMoveTime = millis(); 
      else if (isScreenSaverEnabled && (millis() - lastMoveTime > 4000)) { currentState = STATE_SCREENSAVER; screensaverStartTime = millis(); display.clearDisplay(); break; }

      if (millis() - lastDisplayUpdate > 100) {
        lastDisplayUpdate = millis(); display.clearDisplay();
        if (sats < 3) { currentState = STATE_CONNECTING; break; }

        int hour = gps.time.hour() + 7; int day = gps.date.day(); int month = gps.date.month(); if (hour >= 24) { hour -= 24; day += 1; }
        char timeString[6]; char dateString[6];
        sprintf(timeString, "%02d:%02d", hour, gps.time.minute()); sprintf(dateString, "%02d/%02d", day, month);

        drawSignalBars(sats); display.setTextSize(1);
        display.setCursor(45, 1); if (gps.time.isValid()) display.print(timeString); else display.print("--:--");
        display.setCursor(92, 1); display.print("Sat:"); display.print(sats); display.drawLine(0, 11, 128, 11, WHITE); 

        int speedInt = (int)currentSpeed; display.setTextSize(5); 

        if (speedInt < 10) {
          display.setCursor(25, 20); display.print(speedInt);
          int rightX = 95; display.setTextSize(1); 
          display.setCursor(rightX, 14); display.print("MAX"); display.setCursor(rightX, 23); display.print((int)topSpeed);
          display.setCursor(rightX, 34); display.print("MDPL"); display.setCursor(rightX, 43);
          if (gps.altitude.isValid()) { display.print(altTrend); display.print((int)gps.altitude.meters()); } else display.print("-");
          display.setCursor(rightX, 54); if (gps.date.isValid()) display.print(dateString); else display.print("--/--");
        } else {
          if (speedInt < 100) display.setCursor(34, 20); else display.setCursor(19, 20); display.print(speedInt);
        }

        if (millis() - popupTimer < POPUP_DURATION && popupMsg != "") {
          int16_t x1, y1; uint16_t w, h; display.setTextSize(1); display.getTextBounds(popupMsg, 0, 0, &x1, &y1, &w, &h);
          int px = (128 - w) / 2 - 4, py = (64 - h) / 2 - 4; display.fillRect(px, py, w + 8, h + 8, WHITE);
          display.setTextColor(BLACK); display.setCursor((128 - w) / 2, (64 - h) / 2); display.print(popupMsg); display.setTextColor(WHITE);
        }
        display.display();
      }
      break;
    }

    case STATE_SCREENSAVER: {
      double currentSpeed = gps.speed.kmph(); if (currentSpeed < 1.0) currentSpeed = 0.0;
      
      if (currentSpeed > 0.0) { lastMoveTime = millis(); setOLEDContrast(255); currentState = STATE_SPEEDOMETER; display.clearDisplay(); break; }
      if (isScreenSaverEnabled && (millis() - screensaverStartTime > 30000)) { currentState = STATE_SLEEP; display.clearDisplay(); display.ssd1306_command(SSD1306_DISPLAYOFF); break; }

      if (millis() - lastDisplayUpdate > 200) { 
        lastDisplayUpdate = millis(); display.clearDisplay();
        if (sats < 3) { currentState = STATE_CONNECTING; break; }

        int hour = gps.time.hour() + 7; int day = gps.date.day(); int month = gps.date.month(); if (hour >= 24) { hour -= 24; day += 1; }
        char timeString[6]; char dateStringLong[11]; char dateStringShort[6];
        sprintf(timeString, "%02d:%02d", hour, gps.time.minute()); sprintf(dateStringLong, "%02d/%02d/%04d", day, month, gps.date.year()); sprintf(dateStringShort, "%02d/%02d", day, month);

        unsigned long carouselTimer = (millis() - screensaverStartTime) % 15000; int page = carouselTimer / 3000; 

        if (page == 0) display.drawBitmap(0, 0, epd_bitmap_PENYUPUTIH, 128, 64, WHITE);
        else if (page == 1) {
          drawSignalBars(sats); display.setTextSize(1); display.setCursor(92, 1); display.print("Sat:"); display.print(sats); display.drawLine(0, 11, 128, 11, WHITE);
          display.setTextSize(3); display.setCursor(19, 20); if (gps.time.isValid()) display.print(timeString); else display.print("--:--");
          display.setTextSize(1); display.setCursor(35, 52); if (gps.date.isValid()) display.print(dateStringLong); else display.print("--/--/----");
        } 
        else if (page == 2) {
          display.setTextSize(2); display.setCursor(0, 5); display.print("Alt:"); if (gps.altitude.isValid()) display.print((int)gps.altitude.meters()); else display.print("-"); display.print(" m");
          display.setTextSize(1); display.setCursor(0, 32); display.print("Lat: "); if (gps.location.isValid()) display.print(gps.location.lat(), 6); else display.print("Mencari...");
          display.setCursor(0, 48); display.print("Lon: "); if (gps.location.isValid()) display.print(gps.location.lng(), 6); else display.print("Mencari...");
        }
        else if (page == 3) {
          display.setTextSize(1); display.setCursor(0, 2); display.print("POWER & TEMP STATUS"); display.drawLine(0, 11, 128, 11, WHITE);
          display.setCursor(0, 18); display.print("Voltage : "); display.print(currentVoltage, 1); display.print(" V");
          display.setCursor(0, 32); display.print("Current : "); display.print(currentAmpere, 2); display.print(" A");
          display.setCursor(0, 48); display.print("Temp    : "); display.print(moduleTemp, 1); display.print(" C");
        }
        else if (page == 4) {
          drawSignalBars(sats); display.setTextSize(1); display.setCursor(45, 1); if (gps.time.isValid()) display.print(timeString); else display.print("--:--");
          display.setCursor(92, 1); display.print("Sat:"); display.print(sats); display.drawLine(0, 11, 128, 11, WHITE); 
          display.setTextSize(5); display.setCursor(25, 20); display.print((int)currentSpeed); 
          int rightX = 95; display.setTextSize(1); 
          display.setCursor(rightX, 14); display.print("MAX"); display.setCursor(rightX, 23); display.print((int)topSpeed);
          display.setCursor(rightX, 34); display.print("MDPL"); display.setCursor(rightX, 43);
          if (gps.altitude.isValid()) { display.print(altTrend); display.print((int)gps.altitude.meters()); } else display.print("-");
          display.setCursor(rightX, 54); if (gps.date.isValid()) display.print(dateStringShort); else display.print("--/--");
        }
        
        if (millis() - popupTimer < POPUP_DURATION && popupMsg != "") {
          int16_t x1, y1; uint16_t w, h; display.setTextSize(1); display.getTextBounds(popupMsg, 0, 0, &x1, &y1, &w, &h);
          int px = (128 - w) / 2 - 4, py = (64 - h) / 2 - 4; display.fillRect(px, py, w + 8, h + 8, WHITE);
          display.setTextColor(BLACK); display.setCursor((128 - w) / 2, (64 - h) / 2); display.print(popupMsg); display.setTextColor(WHITE);
        }
        display.display();
      }
      break;
    }

    case STATE_SLEEP: {
      double currentSpeed = gps.speed.kmph(); if (currentSpeed < 1.0) currentSpeed = 0.0;
      if (currentSpeed > 0.0) {
        lastMoveTime = millis(); display.ssd1306_command(SSD1306_DISPLAYON); setOLEDContrast(255); currentState = STATE_SPEEDOMETER; display.clearDisplay();
      }
      break;
    }
  }
}
