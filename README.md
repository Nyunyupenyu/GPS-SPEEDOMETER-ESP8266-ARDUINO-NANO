1. Navigasi & Tampilan Layar OLED
Menu Booting & Welcome: Menampilkan animasi logo penyu dan proses booting saat perangkat pertama kali dinyalakan.

Status Mencari Sinyal (STATE_CONNECTING): Otomatis aktif saat jumlah satelit kurang dari 3. Layar menampilkan animasi radar pencarian dan jumlah satelit secara real-time.

Speedometer Utama: Menampilkan kecepatan saat ini (KM/H), kecepatan maksimum (Top Speed), altitudo (MDPL) dengan tren naik/turun, jumlah satelit, tanggal, dan jam lokal secara real-time.

Auto Screen Saver & Sleep: Layar akan masuk ke mode screensaver (menampilkan carousel halaman logo, koordinat, status daya/suhu) jika kendaraan diam lebih dari 4 detik, dan masuk mode sleep total setelah 30 detik (jika tidak dimatikan).

2. Kontrol Tombol Fisik (One Button Key pada Pin D3)
Semua fungsi kontrol cepat dapat diakses menggunakan satu tombol tactile dengan indikator pop-up notifikasi di layar OLED:

1x Klik:

Fungsi: Membangunkan layar (Wake Up) jika perangkat sedang dalam mode screensaver atau sleep.

Feedback: Buzzer bunyi 1x pendek, layar memunculkan pop-up "WAKE UP".

2x Klik:

Fungsi: Menyalakan atau mematikan suara Buzzer (Mute).

Feedback: Layar memunculkan pop-up "BUZZER ON" atau "BUZZER OFF".

3x Klik:

Fungsi: Toggle Rekam Data Log GPS (Start/Stop Logging secara instan tanpa buka web).

Feedback:

Saat mulai: Beep 3x dengan frekuensi tinggi 2000Hz, pop-up "LOG START".

Saat berhenti: Beep 3x dengan frekuensi rendah 500Hz, pop-up "LOG STOP".

5x Klik:

Fungsi: Mengaktifkan atau menonaktifkan fitur Auto Screen Saver.

Feedback: Layar memunculkan pop-up "SCR SAVER ON" atau "SCR SAVER OFF".

Tahan 3 Detik (Long Press):

Fungsi: Menyalakan atau mematikan WiFi ESP8266 (Mode Hemat Baterai).

Feedback: Layar memunculkan pop-up "WIFI ON" atau "WIFI OFF".

3. Web Dashboard (IoT via Wi-Fi)
Cara Akses:

Nyalakan perangkat, sambungkan HP atau laptop Anda ke jaringan WiFi bernama RACE_PANEL_PENYU (Password: masuk123).

Buka browser (Chrome/Safari) dan ketik alamat: http://192.168.4.1.

Fitur pada Web Dashboard:

Live Telemetry Dashboard: Memantau kecepatan secara real-time via gauge digital, statistik Top Speed, Avg Speed, Altitudo, Satelit, koordinat GPS, hingga status Voltase, Ampere, Suhu, serta penggunaan memori RAM/ROM ESP8266.

Buzzer Settings: Mengatur status Buzzer (ON/OFF) dan mengubah slider frekuensi suara secara dinamis dari web.

Logger Control: Tombol jarak jauh untuk memulai (REC START) atau menghentikan (REC STOP) perekaman data perjalanan.

File Manager (.CSV): Melihat daftar file log yang tersimpan di memori LittleFS, mengunduh file log (DL), atau menghapusnya (X).

Import & Replay Analyzer: Memungkinkan Anda mengunggah file log CSV dari perjalanan sebelumnya untuk diputar ulang (replay) lengkap dengan grafik visual pergerakan kecepatan dan altitudo.

Tombol Restart: Merestart perangkat ESP8266 secara jarak jauh melalui web.


📌 Skema Jalur Pin (Wiring) ESP8266
1. OLED SSD1306 0.96"

VCC ---> 3.3V (atau 5V)

GND ---> GND

SCL ---> D1 (GPIO 5)

SDA ---> D2 (GPIO 4)

2. Modul GPS NEO-6M

VCC ---> 3.3V atau 5V

GND ---> GND

TX (GPS) ---> D5 (ESP8266)

RX (GPS) ---> D6 (ESP8266)

3. Buzzer

Positif (+) ---> D8 (GPIO 15)

Negatif (-) ---> GND

4. Tactile Button (One Button Key)

Kaki 1 ---> D3 (GPIO 0)

Kaki 2 ---> GND (Bebas bolak-balik)

💡 Catatan Jalur Power (Catu Daya):

Semua pin GND dari OLED, GPS, Buzzer, dan Tombol dapat di-paralel (digabungkan) menuju ke pin GND di ESP8266.

Semua pin VCC untuk OLED dan GPS dapat di-paralel menuju ke sumber daya 3.3V / 5V pada ESP8266.
