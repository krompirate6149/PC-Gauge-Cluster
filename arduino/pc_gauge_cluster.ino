#include <Servo.h>
#include <stdlib.h>
#include <string.h>

// Üç servo: CPU yükü D5, RAM kullanımı D6, GPU yükü D7
Servo servolar[3];
const byte SERVO_PINLER[3] = {5, 6, 7};

// Veri sırası: CPU yükü, RAM kullanımı, GPU yükü
const byte CPU_YUK = 0;
const byte RAM_KULLANIMI = 1;
const byte GPU_YUK = 2;

// Satır tamponu
char satir[32];
byte satirUzunluk = 0;
bool kimlikDogrulandi = false;

// Ölçümler
int hamDeger[3] = {0, 0, 0};

// CPU için son 5 geçerli ölçümün kayan ortalaması
const byte CPU_ORNEK_SAYISI = 5;
int cpuOrnekler[CPU_ORNEK_SAYISI] = {0, 0, 0, 0, 0};
byte cpuOrnekIndeksi = 0;
byte cpuOrnekAdedi = 0;
long cpuOrnekToplami = 0;
float filtreliDeger[3] = {0.0, 0.0, 0.0};

// Her kadranın servo durumu
float mevcutAci[3] = {160.0, 160.0, 160.0};
float hedefAci[3] = {160.0, 160.0, 160.0};
int sonKademe[3] = {-1, -1, -1};
int sonServoKomutu[3] = {-1, -1, -1};

// Ani yükseliş doğrulaması yalnızca GPU yüküne uygulanır; CPU kayan ortalama, RAM doğrudan filtre kullanır.
int spikeAdayi[3] = {0, 0, 0};
byte spikeSayaci[3] = {0, 0, 0};
const byte SPIKE_ONAY_SAYISI = 3;
const int SPIKE_FARKI = 20;

// Zamanlama
unsigned long oncekiHareketZamani = 0;
unsigned long sonFiltreZamani = 0;
const unsigned long HAREKET_ARALIGI = 15;
const unsigned long FILTRE_ARALIGI = 50;

// EMA katsayıları
const float YUKSELME_KATSAYISI = 0.18;
const float DUSME_KATSAYISI = 0.06;

// Açı sınırları ve ölü bölge
const int ACI_MIN = 20;
const int ACI_MAX = 160;
const float ACI_OLU_BOLGE = 2.0;

// Gösterge aralıkları: CPU yükü, RAM kullanımı ve GPU yükü %0-100.
const int DEGER_MIN[3] = {0, 0, 0};
const int DEGER_MAX[3] = {100, 100, 100};

void satiriIsle() {
  satir[satirUzunluk] = '\0';

  if (satirUzunluk == 0) return;

  if (strcmp(satir, "KADRAN?") == 0) {
    kimlikDogrulandi = true;
    Serial.println("KADRAN_OK");
    return;
  }

  if (!kimlikDogrulandi) return;

  // Beklenen format: cpu_yuk,ram_kullanimi,gpu_yuk
  int degerler[3];
  byte alan = 0;
  char *parca = strtok(satir, ",");

  while (parca != NULL && alan < 3) {
    // Alanın yalnızca rakamlardan oluştuğunu doğrula.
    if (*parca == '\0') return;
    for (byte i = 0; parca[i] != '\0'; i++) {
      if (parca[i] < '0' || parca[i] > '9') return;
    }

    degerler[alan] = atoi(parca);
    alan++;
    parca = strtok(NULL, ",");
  }

  // Tam olarak üç alan bulunmalı.
  if (alan != 3 || parca != NULL) return;

  // Her ölçüm kendi aralığında olmalı.
  if (degerler[CPU_YUK] < 0 || degerler[CPU_YUK] > 100) return;
  if (degerler[RAM_KULLANIMI] < 0 || degerler[RAM_KULLANIMI] > 100) return;
  if (degerler[GPU_YUK] < 0 || degerler[GPU_YUK] > 100) return;

  // CPU: son 5 ölçümün kayan ortalamasını hesapla.
  // İlk 5 örnek tamamlanmadan, mevcut örneklerin ortalaması kullanılır.
  cpuOrnekToplami -= cpuOrnekler[cpuOrnekIndeksi];
  cpuOrnekler[cpuOrnekIndeksi] = degerler[CPU_YUK];
  cpuOrnekToplami += cpuOrnekler[cpuOrnekIndeksi];
  cpuOrnekIndeksi = (cpuOrnekIndeksi + 1) % CPU_ORNEK_SAYISI;
  if (cpuOrnekAdedi < CPU_ORNEK_SAYISI) cpuOrnekAdedi++;
  hamDeger[CPU_YUK] = (int)((cpuOrnekToplami + cpuOrnekAdedi / 2) / cpuOrnekAdedi);

  // GPU için mevcut ani yükseliş doğrulamasını koru.
  byte i = GPU_YUK;
  int yeniDeger = degerler[i];

  if (yeniDeger >= filtreliDeger[i] + SPIKE_FARKI) {
    if (spikeSayaci[i] == 0 ||
        abs(yeniDeger - spikeAdayi[i]) <= 10) {
      spikeAdayi[i] = yeniDeger;
      if (spikeSayaci[i] < SPIKE_ONAY_SAYISI) spikeSayaci[i]++;
    } else {
      spikeAdayi[i] = yeniDeger;
      spikeSayaci[i] = 1;
    }

    if (spikeSayaci[i] >= SPIKE_ONAY_SAYISI) {
      hamDeger[i] = yeniDeger;
      spikeSayaci[i] = 0;
    }
  } else {
    hamDeger[i] = yeniDeger;
    spikeSayaci[i] = 0;
  }

  // RAM doğrudan kullanılır; CPU zaten kayan ortalamayla işlendi.
  hamDeger[RAM_KULLANIMI] = degerler[RAM_KULLANIMI];
}

void setup() {
  Serial.begin(9600);

  for (byte i = 0; i < 3; i++) {
    servolar[i].attach(SERVO_PINLER[i]);
    servolar[i].write(ACI_MAX);
  }
}

void loop() {
  // Seri veriyi satır satır oku.
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      satiriIsle();
      satirUzunluk = 0;
    } else if (satirUzunluk < sizeof(satir) - 1) {
      satir[satirUzunluk++] = c;
    } else {
      // Çok uzun satırı at.
      satirUzunluk = 0;
    }
  }

  if (!kimlikDogrulandi) return;

  // Üç ölçümü ayrı ayrı filtrele ve hedef açılarını güncelle.
  if (millis() - sonFiltreZamani >= FILTRE_ARALIGI) {
    sonFiltreZamani = millis();

    for (byte i = 0; i < 3; i++) {
      // CPU'da 5 örnekli ortalama zaten gürültüyü azalttığı için
      // EMA daha hızlı tutulur. RAM/GPU ayarları değiştirilmez.
      float katsayi;
      if (i == CPU_YUK) {
        katsayi = (hamDeger[i] > filtreliDeger[i]) ? 0.45 : 0.25;
      } else {
        katsayi = (hamDeger[i] > filtreliDeger[i])
                  ? YUKSELME_KATSAYISI
                  : DUSME_KATSAYISI;
      }

      filtreliDeger[i] += (hamDeger[i] - filtreliDeger[i]) * katsayi;
      filtreliDeger[i] = constrain(filtreliDeger[i],
                                   DEGER_MIN[i], DEGER_MAX[i]);

      // Yüzdeyi doğrudan açıya çevir; 10'luk kademeler yerine sürekli hareket.
      // İbre yalnızca her %10'luk eşik değiştiğinde yeni hedef alır.
      float yuzde = filtreliDeger[i];
      if (yuzde < 0.0) yuzde = 0.0;
      if (yuzde > 100.0) yuzde = 100.0;
      int kademe = (int)(yuzde / 5.0);
      if (kademe > 20) kademe = 20;
      if (kademe != sonKademe[i]) {
        hedefAci[i] = map(kademe, 0, 20, ACI_MAX, ACI_MIN);
        sonKademe[i] = kademe;
      }

    }
  }

  // Üç servoyu ayrı durumlarıyla kademeli hareket ettir.
  if (millis() - oncekiHareketZamani >= HAREKET_ARALIGI) {
    oncekiHareketZamani = millis();

    for (byte i = 0; i < 3; i++) {
      float fark = hedefAci[i] - mevcutAci[i];

      if (fark >= ACI_OLU_BOLGE) {
        mevcutAci[i] += 1.0;
      } else if (fark <= -ACI_OLU_BOLGE) {
        mevcutAci[i] -= 1.0;
      } else {
        mevcutAci[i] = hedefAci[i];
      }

      mevcutAci[i] = constrain(mevcutAci[i], ACI_MIN, ACI_MAX);
      int komut = (int)(mevcutAci[i] + 0.5);

      if (komut != sonServoKomutu[i]) {
        servolar[i].write(komut);
        sonServoKomutu[i] = komut;
      }
    }
  }
}
