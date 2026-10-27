#include <Servo.h>
#include <stdlib.h>
#include <string.h>

// Üç servo: CPU yükü D5, RAM kullanımı D6, GPU yükü D7
Servo servolar[3];
const byte SERVO_PINLER[3] = {5, 6, 7};

// Yük göstergesi LED'leri: CPU D2, RAM D3, GPU D4
const byte LED_PINLER[3] = {2, 3, 4};
const float LED_ESIK_YUZDESI = 90.0;

// Veri sırası: CPU yükü, RAM kullanımı, GPU yükü
const byte CPU_YUK = 0;
const byte RAM_KULLANIMI = 1;
const byte GPU_YUK = 2;

// Satır tamponu
char satir[32];
byte satirUzunluk = 0;
bool kimlikDogrulandi = false;

// Ölçümler: her geçerli seri paketindeki son değerler
int hamDeger[3] = {0, 0, 0};
float filtreliDeger[3] = {0.0, 0.0, 0.0};
bool yeniVeriVar = false;
bool ilkOlcum = true;

// Her kadranın servo durumu
float mevcutAci[3] = {160.0, 160.0, 160.0};
float hedefAci[3] = {160.0, 160.0, 160.0};
int sonKademe[3] = {-1, -1, -1};
int sonServoKomutu[3] = {-1, -1, -1};

// Zamanlama
unsigned long oncekiHareketZamani = 0;
const unsigned long HAREKET_ARALIGI = 60;  // Yaklaşık 16.7 derece/saniye

// Her yeni seri paketi başına tek, sabit filtre katsayısı.
// Filtre yalnızca yeni ölçüm geldiğinde güncellenir; aynı değer 50 ms'de bir tekrar işlenmez.
const float FILTRE_KATSAYISI = 0.20;

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

  // Üç göstergenin ham değerlerini aynı şekilde al.
  // Kısa süreli sıçramaları filtre, yüzde kademeleri ve servo hız sınırı yumuşatır.
  for (byte i = 0; i < 3; i++) {
    hamDeger[i] = degerler[i];
  }
  yeniVeriVar = true;
}

void setup() {
  Serial.begin(9600);

  for (byte i = 0; i < 3; i++) {
    servolar[i].attach(SERVO_PINLER[i]);
    servolar[i].write(ACI_MAX);

    pinMode(LED_PINLER[i], OUTPUT);
    digitalWrite(LED_PINLER[i], LOW);
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

  // Filtreyi yalnızca yeni ölçüm paketi geldiğinde güncelle.
  // Böylece aynı ham değer her 50 ms'de tekrar tekrar filtrelenmez.
  if (yeniVeriVar) {
    yeniVeriVar = false;

    for (byte i = 0; i < 3; i++) {
      // İlk pakette ibreleri gerçek ölçüme yaklaştır; sonraki paketlerde yumuşat.
      if (ilkOlcum) {
        filtreliDeger[i] = hamDeger[i];
      } else {
        // CPU, RAM ve GPU aynı sabit katsayıyla filtrelenir.
        filtreliDeger[i] +=
            (hamDeger[i] - filtreliDeger[i]) * FILTRE_KATSAYISI;
      }
      filtreliDeger[i] = constrain(filtreliDeger[i],
                                   DEGER_MIN[i], DEGER_MAX[i]);

      // İlgili LED, filtrelenmiş kullanım %90 veya üzerindeyse yanar.
      digitalWrite(LED_PINLER[i],
                   filtreliDeger[i] >= LED_ESIK_YUZDESI ? HIGH : LOW);

      // Yüzdeyi 5'lik kademelerle açıya çevir. Her kademe yeni hedef açısı belirler.
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
    ilkOlcum = false;
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
