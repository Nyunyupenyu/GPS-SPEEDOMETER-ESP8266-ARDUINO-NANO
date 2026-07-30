# 🏁 Speedometer GPS IoT "Penyu" + Racing Dashboard

Projek ini adalah sistem speedometer pintar berbasis GPS menggunakan mikrokontroler ESP8266. Alat ini dilengkapi dengan layar OLED, buzzer, tombol kontrol, dan LED indikator status. Sistem ini tidak hanya menampilkan kecepatan, ketinggian, dan waktu secara *real-time*, tetapi juga dapat merekam perjalanan ke memori internal (*Data Logger* format `.csv`) serta memancarkan sinyal WiFi untuk menyajikan **Racing Dashboard** interaktif yang bisa diakses langsung melalui HP atau laptop.

---

## 🔌 Skematik Pin (Wiring Diagram)

Berikut adalah daftar sambungan kabel antar komponen ke NodeMCU ESP8266:

* **Modul GPS NEO-6M:**
  * Pin VCC $\rightarrow$ **3V3** (atau 5V sesuai modul)
  * Pin GND $\rightarrow$ **GND**
  * Pin TX $\rightarrow$ Pin ESP8266 **D5** (SoftwareSerial RX)
  * Pin RX $\rightarrow$ Pin ESP8266 **D6** (SoftwareSerial TX)
* **Layar OLED 0.96" (I2C):**
  * Pin VCC $\rightarrow$ **3V3**
  * Pin GND $\rightarrow$ **GND**
  * Pin SDA $\rightarrow$ Pin ESP8266 **D1** (GPIO 5)
  * Pin SCL $\rightarrow$ Pin ESP8266 **D2** (GPIO 4)
* **Tactile Button (Tombol Kontrol):**
  * Kaki 1 $\rightarrow$ Pin ESP8266 **D4** (Menggunakan fungsi internal `INPUT_PULLUP`)
  * Kaki 2 $\rightarrow$ **GND**
* **Buzzer:**
  * Kaki Positif (+) $\rightarrow$ Pin ESP8266 **D8** (Disarankan tambah resistor 100 ohm)
  * Kaki Negatif (-) $\rightarrow$ **GND**
* **LED Indikator Status:**
  * Kaki Panjang (+) $\rightarrow$ Pin ESP8266 **D7** (Dilengkapi resistor 220/330 ohm)
  * Kaki Pendek (-) $\rightarrow$ **GND**

---

## 💡 Indikator LED & Cara Kerjanya

Lampu LED pada Pin D7 berfungsi sebagai indikator visual status perangkat:
* **GPS Searching (Satelit < 3):** LED berkedip perlahan dengan jeda **5 detik** (menyala kilat selama 100ms setiap 5 detik sekali).
* **GPS Lock (Satelit $\ge$ 3):** LED menyala stabil terus-menerus (*standby*).
* **Record Log (Merekam Perjalanan):** LED berkedip sangat cepat (*blinking* cepat setiap 150ms).
* **Screen Saver ON:** LED kembali bekerja normal mengikuti status GPS.
* **Screen Saver OFF / Sleep Mode:** LED mati total untuk menghemat daya.

---

## 🎮 Cara Mengakses Fitur Sistem (Tombol Kontrol)

Semua fitur dikendalikan menggunakan **1 Buah Tombol Tactile (Pin D4)** dengan sistem multi-klik:

* **Klik 1x:** Membangunkan layar (*Wake Up*) dari mode *Screensaver* atau *Sleep*.
* **Klik 2x:** Menghidupkan/mematikan suara sistem (*Buzzer Mute/Unmute*).
* **Klik 3x:** Memulai (*Start*) atau menghentikan (*Stop*) perekaman data perjalanan (*Data Logger*).
* **Klik 5x:** Mengaktifkan atau menonaktifkan fitur *Screen Saver* otomatis.
* **Tekan & Tahan 3 Detik (Long Press):** Menyalakan atau mematikan jaringan WiFi ESP8266.

---

## 🌐 Cara Mengakses Web Dashboard (Racing Dashboard)

Untuk memantau telemetri secara nirkabel melalui HP atau Laptop:
1. Nyalakan perangkat, lalu buka pengaturan WiFi di ponsel/laptop Anda.
2. Cari dan sambungkan ke jaringan WiFi: **`RACE_PANEL_PENYU`**
3. Masukkan kata sandi (Password): **`masuk123`**
4. Buka *browser* (Chrome/Safari), lalu akses alamat IP: **`192.168.4.1`**
5. Fitur di dalam Web Dashboard:
   * **Live Gauge Telemetry:** Memantau kecepatan analog secara *real-time*, Top Speed, rata-rata kecepatan, ketinggian (MDPL), jumlah satelit, voltase, suhu, dan koordinat GPS.
   * **Logger Control & File Manager:** Tombol rekam, melihat daftar file log `.csv`, download, hapus, atau upload file log lama.
   * **Telemetry Replay Analyzer:** Klik ikon putar (▶) pada file log untuk melihat grafik interaktif (Chart) perjalanan masa lalu dan menggeser *slider* waktu.
   * **Buzzer Settings:** Mengatur status aktif/non-aktif buzzer serta menggeser *slider* frekuensi suara (100 Hz - 5000 Hz).
