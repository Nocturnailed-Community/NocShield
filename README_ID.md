# NocShield

[![Arduino Lint](https://github.com/Nocturnailed-Community/NocShield/actions/workflows/lint.yml/badge.svg)](https://github.com/Nocturnailed-Community/NocShield/actions/workflows/lint.yml)

*[Read documentation in English (README.md)](README.md)*

**NocShield** adalah library yang modular dan ringan untuk simulasi Network Security (Keamanan Jaringan) menggunakan ESP32. Library ini membawa kapabilitas serangan (offensive) dan pertahanan (defensive) jaringan ke level mikrokontroler, menjadikannya alat yang sangat baik untuk mempelajari kerentanan dan mitigasi jaringan.

> **⚠️ PERINGATAN**
> Library ini dibuat **HANYA UNTUK TUJUAN EDUKASI**. Jangan gunakan alat ini pada jaringan atau perangkat yang bukan milik Anda atau tanpa izin eksplisit untuk diuji. Penulis dan pengelola tidak bertanggung jawab atas penyalahgunaan atau kerusakan yang disebabkan oleh perangkat lunak ini.

## Fitur

*   **Simulasi Offensive (Serangan):**
    *   **Deauthentication Attacks:** Mengirim frame 802.11 Deauth untuk memutus koneksi klien dari router.
    *   **Captive Portal (Evil Twin):** Membuat Access Point palsu dengan halaman login *phishing* untuk mencuri kredensial.
    *   **Hash Cracking:** Memanfaatkan akselerasi perangkat keras ESP32 untuk simulasi serangan *dictionary/brute-force* terhadap *hash* SHA.
*   **Simulasi Defensive (Pertahanan):**
    *   **Packet Monitoring (Sniffer):** Menggunakan *Promiscuous Mode* untuk menangkap dan menganalisis frame Wi-Fi mentah di udara.
    *   **Deauth Detector (IDS):** Mendeteksi serangan deautentikasi bertubi-tubi dan memicu alarm.
*   **Komunikasi Aman (Secure Communications):**
    *   **Hardware Cryptography:** Enkripsi/Dekripsi AES dan Hashing SHA-256 super cepat menggunakan mesin `mbedtls` bawaan ESP32.
    *   **Secure Clients:** Contoh yang menunjukkan penggunaan sertifikat Root CA yang benar untuk mencegah serangan Man-in-the-Middle (MitM).

## Instalasi

1.  Unduh repositori ini sebagai file ZIP.
2.  Buka Arduino IDE Anda.
3.  Pilih menu **Sketch** -> **Include Library** -> **Add .ZIP Library...**
4.  Pilih file ZIP yang baru saja diunduh.

*Catatan: Library ini saat ini hanya mendukung mikrokontroler ESP32.*

## Contoh Penggunaan (Examples)

Anda dapat menemukan beberapa contoh praktik langsung di menu **File** -> **Examples** -> **NocShield**:

*   **Offensive:**
    *   `DeauthAttack`: Simulasi memutus klien spesifik dari sebuah AP.
    *   `CaptivePortal`: Membuat AP palsu yang menangkap kredensial yang dimasukkan pengguna.
    *   `HashCracker`: Menunjukkan serangan kamus (*dictionary attack*) terhadap hash SHA-256.
*   **Defensive:**
    *   `PacketMonitor`: Menangkap frame *Management* 802.11 secara mentah.
    *   `DeauthDetector`: Memicu alarm fisik (LED) ketika serangan Deauth terdeteksi.
*   **SecureComms:**
    *   `SecureClient`: Menunjukkan cara melakukan *request* HTTPS yang aman menggunakan validasi Root CA.
    *   `HardwareCrypto`: Menunjukkan cara menggunakan fungsi enkripsi AES dan SHA dengan akselerasi *hardware*.

## Referensi API

### Inisialisasi
```cpp
NocShield nocShield;
nocShield.begin();
```

### Operasi Offensive
```cpp
// Mengirim Frame Deauth
bool success = nocShield.sendDeauth(targetMac, apMac, channel, reasonCode);

// Memulai Captive Portal (Evil Twin)
nocShield.startCaptivePortal("Free Public WiFi", htmlContent);
nocShield.handleCaptivePortal(); // Panggil ini di dalam void loop()
```

### Operasi Defensive
```cpp
// Mulai menangkap paket mentah (Sniffer)
nocShield.startPacketMonitor(myCallbackFunction);

// Berhenti menangkap paket
nocShield.stopPacketMonitor();
```

### Kriptografi Perangkat Keras (Hardware Cryptography)
```cpp
// Menghasilkan angka acak sejati (True Random)
uint32_t rng = nocShield.generateRandomNumber();

// Hashing SHA-256
String hash = nocShield.hashSHA256("password_rahasia_saya");

// Enkripsi/Dekripsi AES-256-CBC
String encryptedHex = nocShield.encryptAES("Data Penting", aesKey, aesIV);
String decryptedStr = nocShield.decryptAES(encryptedHex, aesKey, aesIV);
```

## Kontribusi

Kontribusi selalu diterima! Pastikan Anda menguji kode Anda dan menjalankan Arduino Lint sebelum mengirimkan *Pull Request*.

## Lisensi

Proyek ini dikelola oleh [Nocturnailed Community](https://github.com/Nocturnailed-Community).
