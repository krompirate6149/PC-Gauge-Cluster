import time
import ctypes
from ctypes import wintypes

import psutil
import serial
import serial.tools.list_ports

# Arduino seri haberlesmesi
BAUD = 9600
UPDATE_INTERVAL = 0.25
GPU_UPDATE_INTERVAL = 1.0
HANDSHAKE_COMMAND = b"KADRAN?\n"
HANDSHAKE_RESPONSE = b"KADRAN_OK"

# Windows PDH
PDH_FMT_DOUBLE = 0x00000200
PDH_CSTATUS_VALID_DATA = 0
PDH_CSTATUS_NEW_DATA = 1
PDH_MORE_DATA = 0x800007D2

PDH_HQUERY = wintypes.HANDLE
PDH_HCOUNTER = wintypes.HANDLE

pdh = ctypes.WinDLL("pdh.dll")

class PDH_FMT_COUNTERVALUE_UNION(ctypes.Union):
    _fields_ = [
        ("longValue", ctypes.c_long),
        ("doubleValue", ctypes.c_double),
        ("largeValue", ctypes.c_longlong),
        ("ansiStringValue", ctypes.c_char_p),
        ("wideStringValue", wintypes.LPWSTR),
    ]

class PDH_FMT_COUNTERVALUE(ctypes.Structure):
    _anonymous_ = ("value",)
    _fields_ = [
        ("CStatus", wintypes.DWORD),
        ("value", PDH_FMT_COUNTERVALUE_UNION),
    ]

class PDH_FMT_COUNTERVALUE_ITEM_W(ctypes.Structure):
    _fields_ = [
        ("szName", wintypes.LPWSTR),
        ("FmtValue", PDH_FMT_COUNTERVALUE),
    ]

# API imzalarini acikca tanimla (64-bit Python dahil)
pdh.PdhOpenQueryW.argtypes = [
    wintypes.LPCWSTR, ctypes.c_size_t, ctypes.POINTER(PDH_HQUERY)
]
pdh.PdhOpenQueryW.restype = wintypes.DWORD

pdh.PdhAddEnglishCounterW.argtypes = [
    PDH_HQUERY, wintypes.LPCWSTR, ctypes.c_size_t,
    ctypes.POINTER(PDH_HCOUNTER)
]
pdh.PdhAddEnglishCounterW.restype = wintypes.DWORD

pdh.PdhCollectQueryData.argtypes = [PDH_HQUERY]
pdh.PdhCollectQueryData.restype = wintypes.DWORD

pdh.PdhGetFormattedCounterArrayW.argtypes = [
    PDH_HCOUNTER, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD),
    ctypes.POINTER(wintypes.DWORD), ctypes.c_void_p
]
pdh.PdhGetFormattedCounterArrayW.restype = wintypes.DWORD

pdh.PdhCloseQuery.argtypes = [PDH_HQUERY]
pdh.PdhCloseQuery.restype = wintypes.DWORD

query_handle = PDH_HQUERY()
counter_handle = PDH_HCOUNTER()
pdh_hazir = False
gpu_sayaci_hazir = False

def pdh_baslat():
    """GPU Engine joker sayacini acar; ornekler daha sonra dizi olarak okunur."""
    global pdh_hazir, gpu_sayaci_hazir

    result = pdh.PdhOpenQueryW(None, 0, ctypes.byref(query_handle))
    if result != 0:
        print(f"Uyari: PDH sorgusu acilamadi (0x{result & 0xFFFFFFFF:08X}).")
        return

    pdh_hazir = True
    # Jokerli sayaç: bütün motor örneklerini PdhGetFormattedCounterArrayW ile alacağız.
    path = r"\GPU Engine(*)\Utilization Percentage"
    result = pdh.PdhAddEnglishCounterW(
        query_handle, path, 0, ctypes.byref(counter_handle)
    )
    if result != 0:
        print(f"Uyari: GPU sayaci eklenemedi (0x{result & 0xFFFFFFFF:08X}).")
        print("Windows'ta GPU Engine sayaci bulunamiyor olabilir.")
        return

    gpu_sayaci_hazir = True
    # Ilk örneklem; sonraki okumalar motor değerlerini günceller.
    pdh.PdhCollectQueryData(query_handle)
    time.sleep(0.15)
    pdh.PdhCollectQueryData(query_handle)

def gpu_yukunu_oku():
    """
    Jokerli GPU Engine sayacinin tum orneklerini okur.
    engtype_3D motorlarindaki en yuksek degeri kullanir; motor yuzdelerini
    toplamaz, cunku bu degerler toplandiginda %100'u asabilir.
    """
    if not pdh_hazir or not gpu_sayaci_hazir:
        return None

    result = pdh.PdhCollectQueryData(query_handle)
    if result != 0:
        return None

    item_size = wintypes.DWORD(0)
    item_count = wintypes.DWORD(0)

    # İlk çağrı gerekli tampon boyutunu ve öğe sayısını verir.
    result = pdh.PdhGetFormattedCounterArrayW(
        counter_handle, PDH_FMT_DOUBLE,
        ctypes.byref(item_size), ctypes.byref(item_count), None
    )
    if result not in (PDH_MORE_DATA, 0) or item_count.value == 0:
        return None

    # Boyut, PDH tarafından bayt cinsinden bildirilir.
    buffer = ctypes.create_string_buffer(item_size.value)
    result = pdh.PdhGetFormattedCounterArrayW(
        counter_handle, PDH_FMT_DOUBLE,
        ctypes.byref(item_size), ctypes.byref(item_count),
        ctypes.cast(buffer, ctypes.c_void_p)
    )
    if result != 0:
        return None

    items = ctypes.cast(
        buffer, ctypes.POINTER(PDH_FMT_COUNTERVALUE_ITEM_W)
    )
    values = []

    for i in range(item_count.value):
        item = items[i]
        name = item.szName or ""
        value = item.FmtValue

        # Yalnızca 3D motorlarını seç; video decode/copy/compute motorlarını
        # 3D kadranına karıştırma.
        if "engtype_3d" not in name.lower():
            continue
        if value.CStatus not in (PDH_CSTATUS_VALID_DATA, PDH_CSTATUS_NEW_DATA):
            continue

        number = value.doubleValue
        if number == number and number >= 0:
            values.append(number)

    if not values:
        return None

    return max(0, min(100, round(max(values))))

def ram_kullanimini_oku():
    return max(0, min(100, round(psutil.virtual_memory().percent)))

def porttan_arduino_bul():
    portlar = list(serial.tools.list_ports.comports())
    if not portlar:
        print("Hata: Seri port bulunamadi.")
        return None, None

    portlar.sort(key=lambda p: 0 if any(
        tag in (p.description or "").upper()
        for tag in ("ARDUINO", "CH340", "USB", "CP210", "FTDI")
    ) else 1)

    for bilgi in portlar:
        port = bilgi.device
        print(f"Port deneniyor: {port} ({bilgi.description})")
        try:
            cihaz = serial.Serial(port, BAUD, timeout=0.35, write_timeout=1)
            time.sleep(2)
            cihaz.reset_input_buffer()
            cihaz.reset_output_buffer()
            cihaz.write(HANDSHAKE_COMMAND)
            cihaz.flush()

            son_zaman = time.monotonic() + 2.0
            while time.monotonic() < son_zaman:
                if cihaz.readline().strip() == HANDSHAKE_RESPONSE:
                    print(f"Arduino dogrulandi: {port}")
                    return cihaz, port

            print(f"Kimlik yaniti alinamadi: {port}")
            cihaz.close()
        except (serial.SerialException, OSError) as hata:
            print(f"Port kullanilamadi ({port}): {hata}")

    print("Hata: KADRAN_OK yaniti veren Arduino bulunamadi.")
    return None, None

def main():
    arduino, port = porttan_arduino_bul()
    if arduino is None:
        return

    pdh_baslat()
    if gpu_sayaci_hazir:
        print("GPU kaynagi: Windows PDH; 3D motor ornekleri ayri okunuyor.")
    else:
        print("UYARI: GPU sayaci hazir degil; GPU kadrani 0 kalabilir.")
    print("Orta kadran: RAM kullanimi.")

    try:
        psutil.cpu_percent(interval=None)
        print(f"Uc kadran baslatildi ({port}). Ctrl+C ile durdurun.")

        sonraki_guncelleme = time.monotonic()
        sonraki_gpu_guncelleme = 0.0
        gpu_yuk_onbellek = 0
        gpu_hata_yazildi = False

        while True:
            sonraki_guncelleme += UPDATE_INTERVAL
            cpu_yuk = max(0, min(100, round(psutil.cpu_percent(interval=None))))
            ram_yuk = ram_kullanimini_oku()

            simdi = time.monotonic()
            if simdi >= sonraki_gpu_guncelleme:
                okunan_gpu = gpu_yukunu_oku()
                if okunan_gpu is not None:
                    gpu_yuk_onbellek = okunan_gpu
                    gpu_hata_yazildi = False
                elif not gpu_hata_yazildi:
                    print("\nUyari: GPU sayacindan gecerli 3D verisi okunamadi.")
                    gpu_hata_yazildi = True
                sonraki_gpu_guncelleme = simdi + GPU_UPDATE_INTERVAL

            gpu_yuk = gpu_yuk_onbellek
            arduino.write(f"{cpu_yuk},{ram_yuk},{gpu_yuk}\n".encode("ascii"))
            print(
                f"\rCPU: %{cpu_yuk:3d} | RAM: %{ram_yuk:3d} | GPU: %{gpu_yuk:3d}",
                end="", flush=True
            )

            bekleme = sonraki_guncelleme - time.monotonic()
            if bekleme > 0:
                time.sleep(bekleme)
            else:
                sonraki_guncelleme = time.monotonic()

    except KeyboardInterrupt:
        print("\nUc kadran durduruldu.")
    except (serial.SerialException, OSError) as hata:
        print(f"\nHata: {hata}")
    finally:
        if pdh_hazir:
            pdh.PdhCloseQuery(query_handle)
        if arduino.is_open:
            arduino.close()

if __name__ == "__main__":
    if hasattr(ctypes, "WinDLL"):
        main()
    else:
        print("Bu kod yalnizca Windows'ta calisir.")
