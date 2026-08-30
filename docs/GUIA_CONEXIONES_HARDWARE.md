# 🔌 Guía Maestra de Conexiones de Hardware (Air Guardian IoT)

Este documento detalla paso a paso cómo conectar todos los componentes comprados al **ESP32 NodeMCU (38 o 30 pines)**.

---

## 📦 Lista de Componentes del Proyecto

1. **ESP32 NodeMCU (Dual-Core 240MHz)**
2. **Pantalla Circular GC9A01 (240x240 SPI)**
3. **Sensor Inercial MPU-6050 (GY-521 - Acelerómetro / Giroscopio I2C)**
4. **Sensor de Calidad de Aire MQ-135 (eCO2, NH3, Humo, Benceno)**
5. **Sensor de Monóxido de Carbono MQ-7 (CO)**
6. **Sensor de Temperatura y Humedad KY-015 (DHT11)**
7. **Protoboard y Cables Jumper (Macho-Hembra y Macho-Macho)**

---

## ⚡ Regla de Oro Eléctrica (Alimentación)

> ⚠️ **MUY IMPORTANTE:** Los sensores **MQ-135** y **MQ-7** tienen resistencias calefactoras internas que consumen ~150-200mA cada uno. 
> - **CONÉCTALOS AL PIN `VIN` (5V directo del cable USB)**.
> - **NUNCA** los alimentes desde el pin `3V3` del ESP32, ya que quemarás el regulador interno.
> - La pantalla **GC9A01** y el sensor **MPU-6050** funcionan a **3.3V** (conéctalos a `3V3`).

---

## 🗺️ Mapa de Pines Completo (Pinout Table)

### 1. Pantalla Circular GC9A01 (Bus SPI Rápido)
| Pin Pantalla GC9A01 | Pin ESP32 | Descripción |
| :--- | :--- | :--- |
| **VCC** | **3V3** | Alimentación 3.3V |
| **GND** | **GND** | Tierra común |
| **SCL (CLK / SCK)** | **GPIO 18** | Reloj SPI (Hardware SPI Clock) |
| **SDA (MOSI / DIN)** | **GPIO 23** | Datos SPI (Hardware SPI MOSI) |
| **DC (Data/Command)**| **GPIO 2**  | Selector Comando / Datos |
| **CS (Chip Select)** | **GPIO 5**  | Selección de Pantalla SPI |
| **RST (Reset)**      | **GPIO 4**  | Reset de Pantalla |
| **BLK (Backlight)**  | **3V3**     | Retroiluminación activa (o GPIO 15 para control PWM) |

---

### 2. Sensor Inercial MPU-6050 (Bus I2C)
| Pin MPU-6050 (GY-521) | Pin ESP32 | Descripción |
| :--- | :--- | :--- |
| **VCC** | **3V3** | Alimentación 3.3V |
| **GND** | **GND** | Tierra común |
| **SCL** | **GPIO 22** | Reloj I2C |
| **SDA** | **GPIO 21** | Datos I2C |
| **AD0** | **GND** | Fija dirección I2C en `0x68` |
| **INT** | *No Conectar* (Opcional) | |

---

### 3. Sensor Ambiental DHT11 (KY-015)
| Pin DHT11 (KY-015) | Pin ESP32 | Descripción |
| :--- | :--- | :--- |
| **+ (VCC)** | **3V3** | Alimentación 3.3V |
| **- (GND)** | **GND** | Tierra común |
| **S (Señal / Data)**| **GPIO 15** | Línea de datos digital One-Wire |

---

### 4. Sensores de Gas Analógicos (MQ-135 y MQ-7)
| Sensor | Pin Sensor | Pin ESP32 | Descripción |
| :--- | :--- | :--- | :--- |
| **MQ-135** | **VCC** | **VIN (5V)** | Alimentación del calefactor (5V) |
| **MQ-135** | **GND** | **GND** | Tierra común |
| **MQ-135** | **A0 (Analógico)** | **GPIO 34** | Canal ADC1_CH6 (No interfiere con WiFi) |
| **MQ-135** | **D0** | *No Conectar* | |
| **MQ-7**   | **VCC** | **VIN (5V)** | Alimentación del calefactor (5V) |
| **MQ-7**   | **GND** | **GND** | Tierra común |
| **MQ-7**   | **A0 (Analógico)** | **GPIO 35** | Canal ADC1_CH7 (No interfiere con WiFi) |
| **MQ-7**   | **D0** | *No Conectar* | |

> **Nota de ingeniería:** Usamos `GPIO 34` y `GPIO 35` porque pertenecen al bloque **ADC1**. Los pines de ADC2 (como GPIO 4, 0, 2, 12, 14, 25, 26, 27) se deshabilitan cuando el chip activa el WiFi; ADC1 funciona permanentemente con WiFi activo.

---

## 🛠️ Procedimiento de Calibración Inicial (Burn-In Time)

1. **Precalentamiento de Sensores:**
   * La primera vez que conectes los sensores MQ, déjalos alimentados durante **12 a 24 horas** continuas para eliminar la humedad y residuos del sensor químico.
2. **Medición de $R_0$ en Aire Limpio:**
   * Coloca el dispositivo en un espacio exterior ventilado con aire fresco.
   * El firmware calculará automáticamente la resistencia base $R_0$ y la guardará en la memoria no volátil (NVS/Flash) del ESP32.
