# Panduan Lengkap Edge Impulse untuk Pemula
## Proyek: Alat Bantu Tuna Rungu — Deteksi Klakson & Sirine
### Hardware: Seeed XIAO ESP32-S3 + INMP441 + LR7843 + Motor Vibrasi 3V

---

## Daftar Isi
1. [Apa itu Edge Impulse dan mengapa kita pakai ini?](#1-apa-itu-edge-impulse)
2. [Bagaimana AI mengenali suara?](#2-bagaimana-ai-mengenali-suara)
3. [Persiapan sebelum mulai](#3-persiapan)
4. [Buat akun dan project baru](#4-buat-akun--project-baru)
5. [Kumpulkan data audio (Data Collection)](#5-kumpulkan-data-audio)
6. [Rancang cara kerja AI (Design Impulse)](#6-rancang-cara-kerja-ai-design-impulse)
7. [Konfigurasi pengolahan audio (MFCC)](#7-konfigurasi-mfcc)
8. [Training model AI](#8-training-model-ai)
9. [Uji model sebelum deploy](#9-uji-model)
10. [Export library ke PlatformIO](#10-export-library)
11. [Aktifkan AI di code](#11-aktifkan-ai-di-code)
12. [Build, upload, dan test akhir](#12-build-upload-dan-test-akhir)
13. [Troubleshooting lengkap](#13-troubleshooting)

---

## 1. Apa itu Edge Impulse?

**Edge Impulse** adalah platform online gratis untuk membuat dan melatih AI yang berjalan langsung di perangkat kecil seperti ESP32 — tanpa perlu server atau internet saat digunakan.

### Analogi sederhana:
Bayangkan kamu ingin mengajari seorang anak mengenali suara. Caranya:
- Kamu perdengarkan banyak contoh suara klakson → "ini namanya klakson"
- Kamu perdengarkan banyak contoh sirine → "ini namanya sirine"
- Kamu perdengarkan suara lain → "ini bukan keduanya"
- Setelah cukup latihan, anak itu bisa mengenali sendiri

Edge Impulse bekerja persis seperti itu. Kita **memberi contoh suara** (data), lalu Edge Impulse **melatih AI** untuk mengenalinya, kemudian AI tersebut **ditanam ke ESP32** agar bisa bekerja secara mandiri.

### Kenapa tidak pakai cara lain?
- FFT biasa (cara lama) hanya mendeteksi **frekuensi** — bisa salah jika ada suara lain di frekuensi yang sama
- AI dengan Edge Impulse belajar dari **pola suara secara keseluruhan** — jauh lebih akurat
- Edge Impulse gratis, mudah, dan mendukung ESP32-S3 dengan baik

---

## 2. Bagaimana AI Mengenali Suara?

Proses yang terjadi di dalam device kamu saat berjalan:

```
Mikrofon INMP441
      ↓
  Rekam audio 1 detik (16.000 titik data per detik)
      ↓
  MFCC — Ubah gelombang suara jadi "sidik jari" angka
  (seperti mengubah foto menjadi deskripsi fitur wajah)
      ↓
  Neural Network — Cocokkan "sidik jari" dengan pola yang sudah dipelajari
      ↓
  Hasil: "klakson 94%" / "sirine 87%" / "noise 99%"
      ↓
  Jika confidence > 75%: aktifkan motor vibrasi
```

**MFCC (Mel-Frequency Cepstral Coefficients)** adalah teknik standar yang digunakan di hampir semua aplikasi pengenalan suara (termasuk Siri dan Google Assistant). Ia mengubah suara menjadi angka-angka yang menggambarkan karakteristik unik suara tersebut.

---

## 3. Persiapan

### Yang kamu butuhkan sebelum mulai:
- [ ] Komputer dengan browser (Chrome direkomendasikan)
- [ ] Koneksi internet
- [ ] File audio klakson dan sirine (dijelaskan di Tahap 5)
- [ ] Email untuk daftar akun Edge Impulse

### Software yang perlu diinstall (opsional, untuk cara rekam langsung):
- **Node.js** — https://nodejs.org (pilih versi LTS)
- Setelah install Node.js, buka Command Prompt dan ketik:
  ```
  npm install -g edge-impulse-cli
  ```
  Tunggu hingga selesai. Ini menginstall alat untuk menghubungkan ESP32 ke Edge Impulse.

> **Catatan:** Jika kamu tidak ingin install Node.js, tidak apa-apa. Ada cara upload file audio langsung tanpa perlu menghubungkan ESP32. Dijelaskan di Tahap 5.

---

## 4. Buat Akun & Project Baru

### Langkah 4.1 — Daftar akun
1. Buka browser, pergi ke **https://studio.edgeimpulse.com**
2. Klik tombol **"Sign up"** (pojok kanan atas)
3. Isi:
   - Email: gunakan email aktif kamu
   - Password: buat password yang kuat
   - Name: nama kamu
4. Cek email → klik link verifikasi yang dikirim Edge Impulse
5. Login ke studio

### Langkah 4.2 — Buat project baru
1. Setelah login, kamu akan melihat halaman **"Projects"**
2. Klik tombol **"+ Create new project"**
3. Muncul dialog seperti ini:

   **"Enter the name for your new project"**
   → Ketik: `klakson-sirine-detector`

   **"Choose your project type"**
   → Pilih **Personal** (gratis, cukup untuk project ini)
   > Personal memberi batas 4GB data dan 60 menit training per job — lebih dari cukup untuk project kita.

   **"Choose your project setting"**
   → Pilih **Private**
   > Private artinya hanya kamu yang bisa melihat dan mengedit project. Pilih ini agar data tidak dipublikasikan ke internet.

4. Klik **"Create new project"**

> **Catatan:** Pilihan tipe "Audio classification" yang dulu ada di halaman ini sudah dihapus oleh Edge Impulse. Tipe data audio akan diatur otomatis nanti saat kamu upload data dan Design Impulse di Tahap 6. Jadi tidak perlu khawatir.

Sekarang kamu masuk ke dashboard project. Di sidebar kiri akan ada beberapa menu — kita akan melewatinya satu per satu.

---

## 5. Kumpulkan Data Audio

Ini adalah tahap **paling penting dan paling menentukan akurasi AI**. Semakin banyak dan bervariasi data yang kamu berikan, semakin pintar AI-nya.

### Konsep "kelas" (label):
Kita perlu 3 kelas suara:

| Nama Kelas | Isi | Minimum Klip |
|------------|-----|--------------|
| `klakson` | Suara klakson kendaraan (motor, mobil, truk, bus) | 50 klip |
| `sirine` | Suara sirine (ambulans, polisi, pemadam kebakaran) | 50 klip |
| `noise` | Suara latar yang BUKAN keduanya (orang bicara, angin, diam, musik, dll) | 75 klip |

> **Mengapa `noise` lebih banyak?** Karena di dunia nyata, suara "bukan klakson dan bukan sirine" jauh lebih sering terjadi. Jika data noise kurang, AI akan terlalu sering salah mendeteksi.

> **Soal jumlah klip — jangan khawatir!** Kamu tidak perlu mencari ratusan file terpisah. Cukup download beberapa file audio panjang (1-5 menit), lalu potong otomatis menggunakan fitur **Split sample** di Edge Impulse. Cara lengkapnya ada di bawah.

### Dari mana mendapat file audio?

**Sumber A — FreeSound.org (Direkomendasikan, GRATIS):**
1. Buka https://freesound.org → buat akun gratis
2. Cari dan download (pilih yang durasinya panjang, 30 detik hingga beberapa menit):
   - Klakson: `car horn`, `bicycle horn`, `truck horn`
   - Sirine: `ambulance siren`, `police siren`, `fire truck siren`
   - Noise: `street noise`, `crowd noise`, `rain`, `wind`

**Sumber B — YouTube:**
1. Cari video kompilasi klakson atau sirine yang panjang
2. Download audio-nya, convert ke WAV 16kHz mono menggunakan Audacity

**Format yang dibutuhkan sebelum upload:**
- Format: `.wav`
- Sample rate: `16000 Hz`
- Channel: Mono

---

### Cara Upload dan Split Sample di Edge Impulse

Ini alur yang paling efisien — upload file panjang, lalu potong otomatis jadi banyak klip kecil.

#### Langkah 1 — Masuk ke Data Acquisition
1. Di sidebar kiri Edge Impulse, klik **"Data acquisition"**
2. Kamu akan melihat dua tab di bagian atas: **"Training"** dan **"Test"**

   > **Training** = data untuk melatih AI (80% dari total data)
   > **Test** = data untuk menguji AI setelah training (20% dari total data)

   Untuk sekarang, pastikan tab **"Training"** yang aktif.

#### Langkah 2 — Upload file audio panjang
1. Klik tombol **"Upload data"** (ikon awan ↑, biasanya di pojok kanan atas area data)
2. Di halaman upload yang muncul, isi sebagai berikut:
   - **"Upload mode"** → pilih `Upload existing data`
   - **"Select files"** → klik `Choose files` → pilih file `.wav` klakson kamu
   - **"Label"** → ketik: `klakson`
   - **"Upload into category"** → pilih `Training`
3. Klik **"Begin upload"** → tunggu sampai selesai
4. Ulangi untuk file sirine (label: `sirine`) dan noise (label: `noise`)

#### Langkah 3 — Split sample jadi klip-klip pendek

Setelah upload, file panjang kamu masih berupa 1 sampel utuh. Sekarang kita potong:

1. Di halaman **"Data acquisition"**, kamu akan melihat daftar sampel yang baru diupload
2. Klik nama sampel yang ingin dipotong (misal file klakson 2 menit tadi)
3. Sampel akan terbuka — kamu bisa melihat gelombang suaranya
4. Cari dan klik tombol **"⋮"** (tiga titik / more options) di pojok kanan sampel tersebut
5. Pilih **"Split sample"**

6. Muncul dialog Split sample. Atur sebagai berikut:
   - **"Segment length (ms)"** → ketik `1000` (artinya setiap potongan = 1 detik)
   - **"Overlap (ms)"** → biarkan `0` atau isi `500` jika ingin potongan saling tumpang tindih
   - Kamu akan melihat preview garis-garis vertikal yang memotong gelombang suara

   > **Apa itu Overlap?**
   > Jika kamu set overlap 500ms, setiap klip 1 detik akan "berbagi" 0.5 detik dengan klip berikutnya.
   > Ini menghasilkan lebih banyak klip dari file yang sama.
   > Contoh: File 10 detik tanpa overlap → 10 klip. Dengan overlap 500ms → 19 klip.

7. Klik **"Split"**
8. Edge Impulse akan otomatis membuat banyak sampel baru dari file panjang tadi
9. File asli yang panjang akan hilang, digantikan oleh klip-klip pendek

#### Langkah 4 — Hapus klip yang jelek (opsional tapi dianjurkan)

Setelah split, mungkin ada beberapa klip yang isinya tidak ideal — misalnya klip klakson yang ternyata isinya hening, atau ada suara lain yang mengganggu.

1. Klik tiap sampel untuk mendengarkannya (ada tombol play ▶)
2. Jika isi klip tidak sesuai label → klik ikon **tempat sampah 🗑** untuk hapus
3. Ini memastikan kualitas data tetap bersih

#### Langkah 5 — Cek distribusi data

Setelah semua kelas selesai diupload dan di-split:
1. Lihat panel di kanan halaman Data Acquisition
2. Ada grafik yang menunjukkan total durasi per kelas
3. Idealnya tiap kelas punya durasi yang **seimbang** — jika salah satu jauh lebih sedikit, tambah data untuk kelas itu

#### Contoh alur nyata:

```
Kamu download dari FreeSound:
- "city_traffic_horns.wav"  → durasi 3 menit  → split 1 detik → ~180 klip → label: klakson
- "ambulance_compilation.wav" → durasi 2 menit → split 1 detik → ~120 klip → label: sirine
- "street_ambience.wav"    → durasi 5 menit  → split 1 detik → ~300 klip → label: noise

Total: ~600 klip dari hanya 3 file!
Hapus klip yang kosong atau tidak relevan → sisa ~400 klip yang berkualitas
```

### Tips tambahan:
- Untuk noise, rekam juga suara di **ruangan/tempat yang sama** dengan di mana alat akan dipakai — ini membuat AI lebih akurat di kondisi nyata
- Rekam klakson dari **jarak berbeda** (dekat dan jauh) agar AI bisa mendeteksi dari berbagai situasi
- Rekam sirine saat **mendekat** dan **menjauh** karena suaranya berbeda (efek Doppler)

---

## 6. Rancang Cara Kerja AI (Design Impulse)

**Impulse** adalah "resep" bagaimana AI kamu memproses data dari awal hingga akhir.

1. Di sidebar kiri, klik **"Impulse design"** → klik **"Create impulse"**

2. Kamu akan melihat 3 kotak berderet:
   - **Input block** (kotak merah/orange): sumber data
   - **Processing block** (kotak biru): cara mengolah data
   - **Learning block** (kotak hijau): cara belajar/mengenali

### Langkah 6.1 — Konfigurasi Input
Di kotak **"Time series data"** (sudah ada otomatis):
- **Window size**: `1000 ms` → AI akan menganalisis potongan suara sepanjang 1 detik
- **Window increase**: `500 ms` → setiap 0.5 detik, AI menganalisis 1 detik baru (ada tumpang tindih 0.5 detik)
- **Frequency**: `16000 Hz` → harus sama dengan sample rate mikrofon kamu
- **Zero-pad data**: centang ✓

> **Analogi Window size:** Seperti jendela yang bergeser di atas sebuah rekaman panjang. Setiap kali jendela bergeser 0.5 detik, AI menganalisis isi jendela 1 detik tersebut.

### Langkah 6.2 — Tambah Processing Block
1. Klik tombol **"Add a processing block"**
2. Muncul daftar pilihan — pilih **"MFCC"**

   > **Mengapa MFCC?** MFCC (Mel-Frequency Cepstral Coefficients) adalah teknik yang meniru cara telinga manusia mendengar. Ia lebih baik dalam membedakan suara daripada FFT biasa karena mempertimbangkan cara otak kita memproses bunyi.

3. Klik **"Add"**

### Langkah 6.3 — Tambah Learning Block
1. Klik tombol **"Add a learning block"**
2. Pilih **"Classification"**

   > **Classification** artinya AI akan memutuskan: "suara ini masuk kategori mana?" — persis yang kita butuhkan.

3. Klik **"Add"**

### Langkah 6.4 — Simpan
1. Pastikan tampilan menunjukkan: `Audio (MFE)` → `MFCC` → `Classification`
2. Klik **"Save Impulse"**

---

## 7. Konfigurasi MFCC

Sekarang kita atur cara MFCC mengolah audio.

1. Di sidebar kiri, klik **"MFCC"** (muncul setelah save impulse)

2. Kamu melihat halaman dengan parameter dan grafik visualisasi. Gunakan setting ini:
   - **Num coefficients**: `13`
   - **Frame length**: `0.02` (artinya setiap frame = 20 milidetik audio)
   - **Frame stride**: `0.01` (setiap frame bergeser 10 milidetik)
   - **Filter number**: `32`
   - **FFT length**: `256`
   - **Low frequency**: `0`
   - **High frequency**: `8000`
   - **Noise floor (dB)**: `-52`
   - **Pre-coff**: `0.97`

   > Setting default di atas sudah cukup baik untuk suara klakson dan sirine. Jangan ubah dulu jika ini pertama kali kamu training.

3. Klik **"Save parameters"**

4. Setelah disimpan, klik **"Generate features"**
   - Proses ini bisa memakan waktu 2-10 menit tergantung jumlah data
   - Edge Impulse akan mengubah semua file audio menjadi representasi MFCC

5. Setelah selesai, kamu akan melihat **"Feature explorer"** — grafik 3D berwarna-warni.

   > **Cara membaca Feature explorer:**
   > Setiap titik berwarna adalah satu sampel audio. Warna berbeda = kelas berbeda.
   > - Jika titik-titik **mengelompok rapi** berdasarkan warna → bagus, AI akan mudah membedakannya
   > - Jika titik-titik **bercampur acak** → perlu data lebih banyak atau lebih bervariasi

   Idealnya kamu melihat 3 kelompok titik yang terpisah jelas.

---

## 8. Training Model AI

Ini tahap di mana AI benar-benar "belajar" dari data yang sudah kamu siapkan.

1. Di sidebar kiri, klik **"Classifier"**

2. Kamu akan melihat konfigurasi neural network. Atur sebagai berikut:

   **Training settings:**
   - **Number of training cycles**: `100`
     > Ini berapa kali AI membaca ulang seluruh data. Lebih banyak = lebih lama tapi biasanya lebih akurat. Mulai dengan 100.
   - **Learning rate**: `0.0005`
     > Seberapa besar langkah belajar AI. Nilai terlalu besar = belajar terburu-buru. Nilai terlalu kecil = belajar terlalu lambat. 0.0005 adalah titik tengah yang baik.
   - **Validation set size**: `20` (persen)
     > AI akan menyisihkan 20% data training untuk mengecek dirinya sendiri.

   **Neural network architecture** (biarkan default):
   - Edge Impulse sudah mengatur arsitektur yang cocok secara otomatis
   - Kamu bisa lihat ada beberapa layer (Dense, Dropout, dll) — ini adalah "otak" AI

3. Klik **"Start training"**

4. Tunggu proses selesai. Lamanya tergantung jumlah data:
   - 300 sampel: ~3-5 menit
   - 1000 sampel: ~10-20 menit

5. Setelah selesai, lihat hasil di bagian bawah:

   **Confusion matrix** — tabel yang menunjukkan seberapa akurat AI:
   ```
   Contoh confusion matrix yang baik:
   
              klakson  sirine  noise
   klakson  [  95%      2%      3%  ]
   sirine   [   1%     93%      6%  ]
   noise    [   0%      1%     99%  ]
   ```
   > Angka besar di diagonal (pojok kiri atas ke kanan bawah) = bagus.
   > Angka besar di luar diagonal = AI sering keliru.

   **Target akurasi:**
   - > 85% = bagus, bisa dipakai
   - 75-85% = cukup, pertimbangkan tambah data
   - < 75% = perlu diperbaiki (lihat tips di bawah)

### Jika akurasi rendah:

**Masalah 1: Akurasi klakson rendah**
→ Tambah lebih banyak variasi klakson (berbagai jenis kendaraan, jarak berbeda)

**Masalah 2: Klakson sering terdeteksi sebagai sirine atau sebaliknya**
→ Pastikan label saat upload sudah benar. Dengarkan ulang sampel yang salah.
→ Tambah data yang lebih "jelas" karakteristiknya

**Masalah 3: Semua terdeteksi sebagai noise**
→ Data noise terlalu banyak dibanding kelas lain — seimbangkan jumlahnya

**Solusi umum:**
- Naikkan training cycles ke 200
- Tambah data minimal 50 sampel per kelas yang bermasalah
- Pastikan kualitas audio bersih (tidak terlalu banyak noise di rekaman)

---

## 9. Uji Model

Sebelum deploy ke ESP32, uji dulu apakah model bekerja baik.

### Uji dengan data test:
1. Di sidebar, klik **"Model testing"**
2. Klik **"Classify all"**
3. Tunggu proses, lihat akurasi di data test

   > Data test adalah data yang TIDAK pernah dilihat AI saat training. Ini menunjukkan performa AI di dunia nyata.
   > Jika akurasi test jauh lebih rendah dari training (misal training 90% tapi test 60%), berarti AI "hafal" data tapi tidak benar-benar "mengerti" — perlu lebih banyak data yang bervariasi.

### Uji live (jika ESP32 terhubung):
1. Di sidebar, klik **"Live classification"**
2. Jika ESP32 sudah terhubung via edge-impulse-cli, klik **"Start sampling"**
3. Suarakan klakson di dekat mikrofon dan lihat hasilnya real-time
4. Uji juga sirine dan suara latar

---

## 10. Export Library ke PlatformIO

Model yang sudah ditraining perlu "dikemas" menjadi library yang bisa dipakai di PlatformIO.

### Langkah 10.1 — Export dari Edge Impulse
1. Di sidebar kiri, klik **"Deployment"**
2. Di kotak pencarian, ketik `Arduino` → pilih **"Arduino library"**
3. Di bagian **"Select optimizations"**:
   - Pilih **"Quantized (int8)"** — ini mengecilkan ukuran model agar muat di ESP32
   > Quantized artinya angka-angka dalam model dikompres dari 32-bit ke 8-bit. Ukuran model bisa berkurang 4x dengan akurasi yang hampir sama.
4. Klik tombol **"Build"**
5. Tunggu beberapa menit hingga muncul tombol **"Download"**
6. Klik **"Download"** — kamu akan mendapat file `.zip`

   Nama filenya akan seperti: `klakson-sirine-detector_inferencing.zip`

### Langkah 10.2 — Extract dan copy ke project

1. **Extract** (klik kanan file zip → Extract All / Extract Here)
2. Setelah di-extract, kamu akan mendapat sebuah **folder** dengan nama seperti:
   `klakson-sirine-detector_inferencing`
3. **Copy** folder tersebut ke dalam folder `lib` di project kamu:

   **Dari:**
   ```
   Downloads/
   └── klakson-sirine-detector_inferencing/   ← folder ini
   ```

   **Ke:**
   ```
   C:\Users\ACER\Documents\PlatformIO\Projects\audiotovibrationbaseAi\lib\
   └── klakson-sirine-detector_inferencing/   ← paste di sini
   ```

4. Struktur akhir folder lib seharusnya:
   ```
   lib/
   ├── README
   └── klakson-sirine-detector_inferencing/
       ├── klakson-sirine-detector_inferencing.h  ← file header utama
       ├── src/
       ├── edge-impulse-sdk/
       ├── model-parameters/
       └── tflite-model/
   ```

   > **Penting:** Nama folder ini yang akan kamu pakai di `#include`. Catat namanya.

---

## 11. Aktifkan AI di Code

Buka file `src/main.cpp` di PlatformIO/VS Code dan lakukan 3 perubahan:

### Perubahan 1 — Tambahkan header library

Cari baris ini di bagian atas file (sekitar baris 14):
```cpp
// #include <nama-project_inferencing.h>
```

Hapus `//` di depannya dan ganti `nama-project` dengan nama folder library kamu:
```cpp
#include <klakson-sirine-detector_inferencing.h>
```

> **Cara tahu nama yang benar:** Buka folder `lib/klakson-sirine-detector_inferencing/` dan lihat file `.h` yang ada. Nama file itu (tanpa `.h`) adalah yang kamu tulis di `#include`.

### Perubahan 2 — Aktifkan fungsi runInference

Cari blok komentar yang dimulai dengan `/*` dan diakhiri `*/` di sekitar fungsi `runInference()`:

```cpp
/*
void runInference() {
    ...
}
*/
```

Hapus baris `/*` dan baris `*/` saja. Sisanya biarkan apa adanya.

### Perubahan 3 — Ganti pemanggilan fungsi di loop()

Cari baris ini di dalam fungsi `loop()`:
```cpp
runFFTDemo();
```

Ganti menjadi:
```cpp
runInference();
```

Dan hapus atau beri komentar baris `runFFTDemo()` agar tidak terpanggil.

### Hasil akhir bagian loop() yang benar:
```cpp
void loop() {
    if (!captureAudio()) {
        Serial.println("[WARN] Gagal baca audio, coba lagi...");
        delay(100);
        return;
    }

    runInference();  // ← AI aktif
}
```

---

## 12. Build, Upload, dan Test Akhir

### Langkah 12.1 — Build project
1. Di VS Code dengan PlatformIO, tekan **Ctrl + Alt + B** atau klik ikon centang (✓) di toolbar bawah
2. Tunggu proses compile. Pertama kali bisa lama (5-10 menit) karena library Edge Impulse besar
3. Jika ada error, lihat bagian Troubleshooting di bawah

**Output build yang berhasil:**
```
Building .pio/build/seeed_xiao_esp32s3/firmware.elf
RAM:   [=         ]  12.3% (used 40312 bytes from 327680 bytes)
Flash: [========  ]  78.4% (used 1027408 bytes from 1310720 bytes)
========================= [SUCCESS] =========================
```

### Langkah 12.2 — Upload ke ESP32-S3
1. Hubungkan XIAO ESP32-S3 ke komputer via USB
2. Tekan **Ctrl + Alt + U** atau klik ikon panah (→) di toolbar bawah
3. Tunggu upload selesai

### Langkah 12.3 — Buka Serial Monitor
1. Tekan **Ctrl + Alt + S** atau klik ikon colokan (🔌) di toolbar bawah
2. Pastikan baud rate = **115200**
3. Kamu akan melihat output seperti:

**Saat pertama menyala:**
```
========================================
  Audio-to-Vibration AI - Tuna Rungu
  Seeed XIAO ESP32-S3 + INMP441
========================================
[MOTOR] Test startup...
[MOTOR] OK
[I2S] Microphone INMP441 siap
[SYSTEM] Siap mendengarkan...
```

**Saat suara normal (tidak ada klakson/sirine):**
```
--- Hasil Deteksi ---
  noise     : 96.20%
  klakson   :  2.10%
  sirine    :  1.70%
```

**Saat ada klakson:**
```
--- Hasil Deteksi ---
  noise     :  2.30%
  klakson   : 94.80%
  sirine    :  2.90%
[AI] KLAKSON! (95%)
[MOTOR] Pola KLAKSON
```

**Saat ada sirine:**
```
--- Hasil Deteksi ---
  noise     :  5.10%
  klakson   :  8.20%
  sirine    : 86.70%
[AI] SIRINE! (87%)
[MOTOR] Pola SIRINE
```

### Langkah 12.4 — Test di lapangan
1. Test dengan suara klakson sungguhan (atau rekaman dari HP)
2. Test dengan suara sirine dari video di HP
3. Perhatikan apakah motor bergetar sesuai pola yang benar

---

## 13. Troubleshooting

### Error saat Compile

**Error: `xxx_inferencing.h: No such file or directory`**
```
Penyebab: Nama di #include tidak sesuai nama file .h di folder lib
Solusi:
1. Buka folder: lib/[nama-folder-library]/
2. Lihat nama file .h yang ada
3. Sesuaikan #include di main.cpp dengan nama file tersebut
```

**Error: `'EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE' was not declared`**
```
Penyebab: Library Edge Impulse belum terdeteksi PlatformIO
Solusi:
1. Pastikan folder library ada langsung di /lib (bukan /lib/lib/ atau /src/)
2. Coba Build ulang: Ctrl+Alt+B
3. Coba Project: Full Clean → Build
```

**Error: `region 'dram0_0_seg' overflowed`**
```
Penyebab: Model terlalu besar untuk RAM ESP32
Solusi:
1. Di Edge Impulse, saat export pilih "Quantized (int8)" bukan "Float32"
2. Di platformio.ini pastikan sudah ada: -DBOARD_HAS_PSRAM
```

**Error: `multiple definition of...`**
```
Penyebab: Ada konflik antara library
Solusi: Hapus library arduinoFFT dari lib_deps di platformio.ini
(tidak dibutuhkan lagi setelah Edge Impulse aktif)
```

---

### Masalah saat Runtime (Serial Monitor)

**Pesan: `[WARN] Gagal baca audio, coba lagi...` terus menerus**
```
Penyebab: Mikrofon INMP441 tidak terdeteksi
Cek:
- Pin I2S_WS (D1), I2S_SCK (D2), I2S_SD (D3) sudah terhubung benar?
- Tegangan VCC ke INMP441 = 3.3V (bukan 5V!)
- Ground sudah terhubung?
- Coba ganti kabel jumper
```

**Motor tidak bergetar sama sekali**
```
Penyebab: Masalah di rangkaian motor atau pin D0
Cek:
- Pin D0 ke gate LR7843 sudah terhubung?
- Source LR7843 ke GND?
- Drain LR7843 ke motor negatif?
- Motor positif ke sumber daya 3V?
- Coba ukur tegangan di pin D0 saat deteksi (harusnya 3.3V)
```

**Selalu mendeteksi klakson/sirine meski tidak ada suara**
```
Penyebab: Model terlalu sensitif atau data noise kurang
Solusi:
1. Naikkan CONFIDENCE_THRESHOLD di main.cpp dari 0.75 menjadi 0.85
2. Tambah data noise yang direkam di lingkungan yang sama
3. Re-training model di Edge Impulse
```

**Klakson nyata tidak terdeteksi**
```
Penyebab: Model belum kenal jenis klakson tersebut
Solusi:
1. Rekam klakson tersebut, tambahkan ke dataset di Edge Impulse
2. Re-training model
3. Sementara: turunkan CONFIDENCE_THRESHOLD ke 0.65
```

**Akurasi bagus di Serial Monitor tapi motor tidak merespons**
```
Penyebab: CONFIDENCE_THRESHOLD terlalu tinggi
Solusi: Buka main.cpp, cari baris:
   #define CONFIDENCE_THRESHOLD 0.75f
Ubah ke 0.65f lalu upload ulang
```

---

### Tabel Pengaturan CONFIDENCE_THRESHOLD

| Nilai | Efek | Kapan dipakai |
|-------|------|---------------|
| `0.90f` | Sangat ketat, jarang trigger | Lingkungan dengan banyak suara mirip |
| `0.80f` | Ketat, akurat | Kondisi normal, direkomendasikan awal |
| `0.75f` | Seimbang (default) | Setting default project ini |
| `0.65f` | Lebih sensitif | Jika terlalu sering tidak terdeteksi |
| `0.55f` | Sangat sensitif, sering false alarm | Hindari kecuali sangat perlu |

---

## Ringkasan Urutan Kerja

```
TAHAP PERSIAPAN (lakukan sekali):
┌─────────────────────────────────────────────┐
│ 1. Daftar akun Edge Impulse                 │
│ 2. Buat project "Audio classification"      │
│ 3. Kumpulkan audio klakson, sirine, noise   │
│ 4. Upload ke Edge Impulse dengan label benar│
│ 5. Design Impulse: MFCC + Classification    │
│ 6. Generate features (MFCC)                 │
│ 7. Training model                           │
│ 8. Cek akurasi > 85%                        │
│ 9. Export sebagai Arduino library (.zip)    │
│ 10. Copy folder ke /lib di project          │
└─────────────────────────────────────────────┘

TAHAP AKTIVASI DI CODE:
┌─────────────────────────────────────────────┐
│ 11. Uncomment #include di main.cpp          │
│ 12. Uncomment fungsi runInference()         │
│ 13. Ganti runFFTDemo() → runInference()     │
│ 14. Build + Upload ke ESP32-S3              │
│ 15. Test dengan suara nyata                 │
└─────────────────────────────────────────────┘

PENGEMBANGAN LANJUTAN (opsional):
┌─────────────────────────────────────────────┐
│ 16. Tambah data jika ada suara yang meleset │
│ 17. Re-train dan export ulang               │
│ 18. Sesuaikan CONFIDENCE_THRESHOLD          │
│ 19. Tambah kelas suara baru (misal: bel)    │
└─────────────────────────────────────────────┘
```

---

*Panduan ini dibuat untuk project Audio-to-Vibration AI pada Seeed XIAO ESP32-S3.*
*Jika ada pertanyaan, tanyakan langsung melalui sesi Cowork.*
