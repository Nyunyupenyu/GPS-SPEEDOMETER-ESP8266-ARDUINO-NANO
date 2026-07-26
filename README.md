# GPS-SPEEDOMETER-ESP8266-ARDUINO-NANO
SPEEDOMETER BASED ON GPS 

BOARD ESP8266 
GPS 6M 
OLED SSD1306
CABLE JUMPER 8X
BREADBOARD 1X

FEATURES:
  - SPEEDOMETER (KM/H)
  - ALTITUDE (MDPL)
  - TOP SPEED LOGGER
  - GPS SIGNAL / DATE / TIME
    
SEE THE DIAGRAM OR SCHEMATIC IN PHOTOS 
CODING AVAILABLE 

1. Rangkaian Kabel (Wiring)
Siapkan kabel jumper secukupnya. Pastikan ESP8266 dalam keadaan tidak tersambung ke listrik/komputer saat merangkai.

Modul OLED SSD1306 (Layar) ke ESP8266:

VCC (OLED) -> 3V3 (ESP8266)

GND (OLED) -> GND (ESP8266)

SCL (OLED) -> D1 (ESP8266)

SDA (OLED) -> D2 (ESP8266)

Modul GPS NEO-6M ke ESP8266:

VCC (GPS) -> 3V3 (ESP8266) (Catatan: Jika GPS tidak menyala atau tidak mendapat sinyal, coba pindahkan ke pin VIN atau 5V karena beberapa modul butuh daya lebih besar).

GND (GPS) -> GND (ESP8266)

TX (GPS) -> D6 (ESP8266)

RX (GPS) -> D7 (ESP8266)

2. Persiapan Software (Arduino IDE)
Sebelum menempelkan kode, Arduino IDE butuh "buku panduan" (Library) agar tahu cara membaca GPS dan menggambar di layar OLED.

Langkah Instalasi Library:

Buka Arduino IDE.

Pergi ke menu Sketch > Include Library > Manage Libraries...

Di kolom pencarian (Search), ketik nama-nama di bawah ini dan klik Install pada hasil yang sesuai:

TinyGPSPlus (oleh Mikal Hart) - Untuk membaca data satelit.

Adafruit GFX Library (oleh Adafruit) - Untuk menggambar teks/grafis.

Adafruit SSD1306 (oleh Adafruit) - Untuk mengontrol layar OLED.

(Opsional) Jika saat menginstal Adafruit SSD1306 muncul kotak peringatan untuk menginstal dependensi tambahan (seperti Adafruit BusIO), klik Install All.
