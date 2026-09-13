# 🌬️ Air Guardian IoT — Sistema de Monitoreo Ambiental, Predicción & Cátedra Universitaria

> **Plataforma Integral IoT de Monitoreo de Calidad del Aire, Data Lakehouse Medallion, Cartografía Táctica Leaflet y Auditorio de Ecuaciones Diferenciales & Análisis Numérico.**

---

## 🏛️ Índice General

1. [Visión General del Proyecto](#-visión-general-del-proyecto)
2. [Articulación de las Tres Asignaturas Universitarias](#-articulación-de-las-tres-asignaturas-universitarias)
   - [Materia 1: Ecuaciones Diferenciales Ordinarias (EDO)](#materia-1-ecuaciones-diferenciales-ordinarias-edo)
   - [Materia 2: Análisis Numérico](#materia-2-análisis-numérico)
   - [Materia 3: Sistemas Embebidos / IoT y Arquitectura de Software](#materia-3-sistemas-embebidos--iot-y-arquitectura-de-software)
3. [Cartografía Táctica & Bitácora de Campo (`LocationsMapBoard.vue`)](#-cartografía-táctica--bitácora-de-campo-locationsmapboardvue)
   - [Eliminación del Radar Legacy](#eliminación-del-radar-legacy)
   - [Integración Leaflet + CartoDB Dark Matter](#integración-leaflet--cartodb-dark-matter)
   - [Bitácora "Lugares Donde Estuvimos"](#bitácora-lugares-donde-estuvimos)
4. [Selector de Modo Dual: Local vs En Vivo 1-a-1](#-selector-de-modo-dual-local-vs-en-vivo-1-a-1)
   - [Modo Local (Datos Quemados & Generador Estocástico)](#modo-local-datos-quemados--generador-estocástico)
   - [Modo En Vivo (ESP32 1-a-1 & Heartbeat)](#modo-en-vivo-esp32-1-a-1--heartbeat)
5. [Buffer Offline en LittleFS & Endpoint Batch](#-buffer-offline-en-littlefs--endpoint-batch)
   - [Arquitectura Resiliente en Firmware (`offline_buffer.hpp`)](#arquitectura-resiliente-en-firmware-offline_bufferhpp)
   - [Procesamiento Atómico en Backend (`POST /api/telemetry/batch`)](#procesamiento-atómico-en-backend-post-apitelemetrybatch)
6. [Auditorio Académico: Módulo EDO y Métodos Numéricos](#-auditorio-académico-módulo-edo-y-métodos-numéricos)
   - [Modelo Matemático de Balance de Masa](#modelo-matemático-de-balance-de-masa)
   - [Euler Progresivo vs Runge-Kutta 4to Orden (RK4)](#euler-progresivo-vs-runge-kutta-4to-orden-rk4)
   - [Estabilidad Asintótica de Dahlquist & Criterio de Lipschitz ($h < 2/k$)](#estabilidad-asintótica-de-dahlquist--criterio-de-lipschitz-h--2k)
   - [Tabla de Errores y Exportación Científica (CSV / LaTeX)](#tabla-de-errores-y-exportación-científica-csv--latex)
7. [Diagrama de Arquitectura Global](#-diagrama-de-arquitectura-global)
8. [Mapa de Hardware & Conexiones del ESP32](#-mapa-de-hardware--conexiones-del-esp32)
9. [Guía de Puesta en Marcha Rápida (Bun Only)](#-guía-de-puesta-en-marcha-rápida-bun-only)
10. [Estructura del Repositorio](#-estructura-del-repositorio)

---

## 🌟 Visión General del Proyecto

**Air Guardian IoT** es un ecosistema tecnológico ciberfísico diseñado para resolver la problemática del monitoreo, mitigación y predicción de la calidad del aire en interiores y recintos urbanos. Integra:

- **Nodo Físico Edge IoT (ESP32 NodeMCU):** Equipado con sensores químicos de óxido metálico (MQ-135 para eCO2/gases nocivos y MQ-7 para monóxido de carbono CO), sensor ambiental DHT11 (humedad/temperatura), acelerómetro/giroscopio MPU-6050 y display circular GC9A01 (240x240 px SPI).
- **Firmware Resiliente con LittleFS:** Almacenamiento local en memoria flash (`offline_buffer.hpp`) que encola lecturas ante desconexiones de red y las sincroniza masivamente al recuperar conectividad mediante ráfagas HTTP/MQTT.
- **Backend Modular Enterprise (NestJS + Bun + SQLite WAL):** Data Lakehouse estructurado bajo la arquitectura Medallion (Bronze, Silver, Gold), validación de fronteras tipadas con Zod, transacciones atómicas por lotes (`POST /api/telemetry/batch`) y gateway WebSocket reactivo.
- **Frontend Alquímico Steampunk (Nuxt 3 + Tailwind CSS):** Consola de laboratorio estilo *Mad Scientist* con arquitectura App Shell Zero-Scroll, cartografía interactiva Leaflet (`LocationsMapBoard.vue`), conmutador de Modo Dual y Auditorio Académico interactivo para sustentación universitaria.

---

## 🎓 Articulación de las Tres Asignaturas Universitarias

El proyecto no es únicamente un prototipo IoT funcional, sino la consolidación empírica y teórica de tres asignaturas de la carrera de Ingeniería:

```
+---------------------------------------------------------------------------------------+
|                       TRILOGÍA ACADÉMICA AIR GUARDIAN IOT                             |
+---------------------------------------------------------------------------------------+
| 1. ECUACIONES DIFERENCIALES    | 2. ANÁLISIS NUMÉRICO        | 3. SISTEMAS EMBEBIDOS /|
|    ORDINARIAS (EDO)           |                             |    ARQUITECTURA IOT    |
+--------------------------------+-----------------------------+------------------------+
| • Balance de Masa Dinámico:    | • Discretización PVI:       | • Firmware ESP32 C++   |
|   V · dC/dt = G - Q·C(t)       |   Euler (O(h)) vs           | • Bus ADC1 sin WiFi bug|
| • Factor Integrante μ(t)=e^kt  |   Runge-Kutta 4 (O(h⁴))     | • LittleFS Offline Queue|
| • Solución Analítica Continua: | • Criterio de Lipschitz:    | • Data Lake Medallion  |
|   C(t)=C_inf+(C0-C_inf)e^(-kt) |   Estabilidad h < 2/k       | • Ingesta Batch Atómica|
| • Constante τ y t_95%          | • Tabla de Errores & LaTeX  | • Leaflet Táctico Dark |
+--------------------------------+-----------------------------+------------------------+
```

### Materia 1: Ecuaciones Diferenciales Ordinarias (EDO)
Se formula la cinética de difusión de gases en un volumen de control cerrado $V$ mediante una **EDO lineal de primer orden de coeficientes constantes**:
$$V \frac{dC}{dt} = G - Q \cdot C(t)$$
Donde $G$ es la tasa de generación interna de contaminantes (respiración, aerosoles, combustión), $Q$ es el caudal volumétrico de ventilación o aire fresco, y $C(t)$ es la concentración en PPM al tiempo $t$. Al normalizar respecto al volumen ($k = Q/V$ y $C_\infty = G/Q$), se deduce la trayectoria analítica continua mediante el método del factor integrante $\mu(t) = e^{kt}$:
$$C(t) = C_\infty + (C_0 - C_\infty) e^{-kt}$$

### Materia 2: Análisis Numérico
Los microcontroladores y computadores operan en tiempo discreto. Se resuelven las ecuaciones cinéticas comparando:
1. **Método de Euler Progresivo:** Esquema de primer orden ($O(h)$ global) basado en la primera derivada de la serie de Taylor.
2. **Método de Runge-Kutta de 4to Orden (RK4):** Esquema de cuatro pendientes intermedias ($O(h^4)$ global) análogo a la regla de Simpson.
3. **Análisis de Estabilidad de Dahlquist / Criterio de Lipschitz:** Demostración formal de la cota crítica del paso temporal $h < \frac{2}{k}$ para garantizar que el error numérico de Euler no diverja asintóticamente.

### Materia 3: Sistemas Embebidos / IoT y Arquitectura de Software
Implementación física y de red:
- Control de periféricos en bus SPI, I2C y One-Wire bajo FreeRTOS.
- Mitigación de la limitación de silicio del ESP32 utilizando exclusivamente canales ADC1 (`GPIO 34` y `GPIO 35`) para permitir conversiones analógicas concurrentes con el radio WiFi activo.
- Sistema de archivos embebido LittleFS (`offline_buffer.hpp`) con rotación defensiva y sincronización batch.
- Backend en Bun SQLite bajo patrón Medallion (Bronze/Silver/Gold) y frontend reactivo en Nuxt 3.

---

## 🗺️ Cartografía Táctica & Bitácora de Campo (`LocationsMapBoard.vue`)

### Eliminación del Radar Legacy
En versiones tempranas, la interfaz incluía un widget de radar circular estático de inspiración militar que no permitía una interacción geográfica real con las coordenadas de muestreo de campo. 

Dicho componente fue completamente retirado y sustituido por **`LocationsMapBoard.vue`**, un módulo cartográfico de alta fidelidad que se integra en el banco inferior de la consola de laboratorio.

### Integración Leaflet + CartoDB Dark Matter
- **Motor de Render:** **Leaflet** ejecutado en cliente sobre un contenedor desacoplado con sombras profundas e iluminación perimetral de latón.
- **Capa Base (Tiles):** *CartoDB Dark Matter* (`https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png`), que proporciona un mapa táctico oscuro y de alto contraste idóneo para la atmósfera *vintage steampunk*.
- **Marcadores Semafóricos con Glow:** Pines circulares SVG dinámicos cuyo color refleja la calidad del aire promedio de la sesión:
  - 🟢 **Esmeralda (`#10b981`):** PPM < 650 (Óptimo).
  - 🟡 **Ámbar (`#f59e0b`):** 650 $\le$ PPM < 1000 (Alerta moderada).
  - 🔴 **Rojo (`#ef4444`):** PPM $\ge$ 1000 (Crítico / No ventilado).
- **Animación Ping & FlyTo:** La sesión actualmente seleccionada emite una onda pulsante (`animate-ping`) y al hacer clic en cualquier tarjeta de la bitácora, la cámara del mapa se traslada suavemente con `leafletMap.flyTo([lat, lng], 15, { duration: 0.8 })`.
- **Popups de Alquimia:** Popups estilizados con tipografía monoespaciada, coordenadas geográficas y recuento de muestras acumuladas.

### Bitácora "Lugares Donde Estuvimos"
Directamente debajo del mapa se encuentra la bandeja de **Fichas Metálicas de Expedición**:
- Despliega todas las campañas registradas (ej. *Universidad - Campus Central*, *Parque Central Los Libertadores*, *Laboratorio de Prototipado IoT*, *Hogar - Base Operativa*).
- Permite seleccionar cualquier punto geográfico para sincronizar los gráficos de series temporales, concentraciones promedio y deltas de inercia.
- Botón **"Marcar Lugar"** para dar de alta nuevas sesiones de muestreo con geolocalización GPS activa.

---

## ⚡ Selector de Modo Dual: Local vs En Vivo 1-a-1

Para garantizar la autonomía del sistema tanto en exposiciones académicas sin red como en despliegues con hardware real, la aplicación cuenta con un **Conmutador Galvánico Dual** en el encabezado (`app.vue`) y una palanca física en la mesa de instrumentos (`FrankensteinSwitch.vue`):

```
+─────────────────────────────────────────────────────────────────────────────+
|               CONMUTADOR DE MODO DE OPERACIÓN DUAL                          |
+─────────────────────────────────────────────────────────────────────────────+
|  [🧪 MODO LOCAL (DATOS QUEMADOS)]   |   [📡 MODO EN VIVO (ESP32 1-A-1)]     |
+-------------------------------------+---------------------------------------+
| • Opera 100% desconectado de red.   | • Vinculado al ESP32 físico por WS.   |
| • Generador estocástico determinista| • Heartbeat continuo (< 8s timeout).  |
|   con dinámicas cíclicas realistas. | • Mascota detecta estado:             |
| • Simulador activado en backend     |   - ONLINE: Feliz / Alerta según PPM  |
|   o en cliente (cero dependencias). |   - OFFLINE: Mascota Zzz Dormida      |
| • Ideal para defensas de cátedra.   | • Tolerante a caídas de WiFi.         |
+─────────────────────────────────────+───────────────────────────────────────+
```

### Modo Local (Datos Quemados & Generador Estocástico)
- Se activa mediante el botón **"🧪 Modo Local"** o accionando la **Palanca de Frankenstein**.
- Desacopla la interfaz del socket del hardware y ejecuta un generador estocástico determinista en el composable `useAirGuardian.ts` (función `runLocalMockStep`).
- Simula variaciones físicas continuas entre 415 PPM y 1200 PPM con inercia, derivando $\Delta \text{PPM}_{1m}$, proyección a 15 minutos y fluctuaciones de temperatura y humedad.
- Garantiza que durante una presentación o sustentación universitaria, la aplicación no falle si el router de la universidad bloquea WebSockets o si el hardware no está alimentado.

### Modo En Vivo (ESP32 1-a-1 & Heartbeat)
- Se activa con el botón **"📡 En Vivo (1-a-1)"**.
- Suspende el simulador sintético e ignora cualquier paquete marcado como `source: 'simulator'`.
- Establece un seguimiento estricto del reloj de paquetes: calcula reactivamente la edad del último paquete (`lastPacketAgeSec = (Date.now() - lastPacketTime) / 1000`).
- **Estados de Enlace:**
  - Si el paquete proviene de `'HARDWARE'` y tiene una edad $< 8\text{ s}$, el indicador muestra `🟢 ESP32 FÍSICO ONLINE (BATERÍA OK)`.
  - Si transcurren más de 8 segundos sin telemetría física, la insignia conmuta a `🔴 ESP32 DESCONECTADO (ESPERANDO SEÑAL)` y la mascota circular del display pasa automáticamente al estado emocional **`SLEEPING` (Zzz)** con ojos cerrados y respiración lenta, alertando al usuario sobre la desconexión física.

---

## 💾 Buffer Offline en LittleFS & Endpoint Batch

Uno de los mayores retos en la recolección de datos ambientales de campo es la pérdida de conectividad WiFi al desplazarse entre edificios, campus universitarios o zonas rurales. Para erradicar la pérdida de información, se diseñó una arquitectura **Offline-First**.

### Arquitectura Resiliente en Firmware (`offline_buffer.hpp`)
- **Sistema de Archivos LittleFS:** Almacena los registros en la memoria flash interna del ESP32 en formato JSON Lines (`/offline_queue.jsonl`).
- **Cuota Defensiva de Flash:** Se fija una cota máxima estricta de **300 KB** (~3500 muestras con timestamp, ppm, co_ppm, temp, hum, delta y raw_adc). Si el buffer se llena por desconexión prolongada, se activa una rotación controlada para proteger la vida útil de la flash y prevenir pánicos de memoria.
- **Algoritmo de Flush & Vaciado:**
  1. Al recuperarse la conexión WiFi (`WiFi.status() == WL_CONNECTED`), la tarea de telemetría invoca `offlineBuffer.flushToBackend(backendUrl, mqttClient)`.
  2. Lee secuencialmente el archivo `/offline_queue.jsonl` y construye lotes de **15 muestras**.
  3. Realiza un `HTTP POST` contra el endpoint `/api/telemetry/batch`.
  4. Si la petición HTTP devuelve `200` o `201`, continúa con el siguiente lote con una pausa inter-bloque de 150 ms para no congestionar la pila TCP.
  5. Si HTTP falla, aplica un mecanismo de *fallback* automático transmitiendo el lote por MQTT sobre el tópico `airguardian/telemetry/batch`.
  6. Únicamente cuando la totalidad de los registros ha sido confirmada por el servidor, se elimina de forma segura el archivo `/offline_queue.jsonl`.

### Procesamiento Atómico en Backend (`POST /api/telemetry/batch`)
El controlador NestJS `TelemetryController` implementa la ruta batch:
- **Validación Zod:** Acepta tanto un arreglo plano de muestras como un objeto contenedor `{ device_id, samples: [...] }`. Valida cada elemento contra `TelemetryIngestSchema`.
- **Transacción Atómica SQLite (`db.transaction()`):**
  - Inserta masivamente todas las muestras en la tabla inmutable **Capa Bronze** (`bronze_telemetry`) preservando exactamente el `timestamp` registrado por el reloj de tiempo real del hardware.
  - Calcula las derivadas temporales $\Delta \text{PPM}_{1m}$ y la predicción a 15 minutos en la **Capa Silver** (`silver_telemetry`) asociándolas a la sesión geoespacial correspondiente.
  - Actualiza los agregados analíticos de la **Capa Gold** (`gold_session_metrics`).
  - Emite un evento WebSocket `TELEMETRY_BATCH` para refrescar los historiales en el frontend sin generar falsos saltos temporales.

---

## 🎓 Auditorio Académico: Módulo EDO y Métodos Numéricos

El modal **Cátedra Universitaria** (`AcademicAuditoriumModal.vue`) proporciona una interfaz interactiva de nivel superior dividida en tres pestañas temáticas:

```
[ Pestaña 1: Deducción EDO ]  ──►  [ Pestaña 2: Simulación Numérica ]  ──►  [ Pestaña 3: Análisis de Errores ]
• Balance de Masa Físico          • Control Deslizante de Paso h           • Tabla Iterativa Discreta
• Normalización k = Q/V           • Osciloscopio CRT: Exacta vs Euler vs RK4• MAE, RMSE y Boost Factor
• Solución Analítica Continua     • Diagnóstico Estabilidad h < 2/k        • Exportación CSV y LaTeX
```

### Modelo Matemático de Balance de Masa
Considerando un recinto de volumen $V\;[\text{m}^3]$:
$$V \frac{dC}{dt} = G - Q \cdot C(t)$$
Dividiendo entre $V$:
$$\frac{dC}{dt} + \left(\frac{Q}{V}\right) C(t) = \frac{G}{V}$$
Definiendo la constante de renovación $k = \frac{Q}{V}\;[\text{min}^{-1}]$ y la concentración de saturación exterior asintótica $C_\infty = \frac{G}{Q}\;[\text{PPM}]$:
$$\frac{dC}{dt} + k C(t) = k C_\infty$$
Multiplicando por el factor integrante $\mu(t) = e^{\int k dt} = e^{kt}$:
$$\frac{d}{dt} \left[ C(t) e^{kt} \right] = k C_\infty e^{kt}$$
Integrando en $[0, t]$ con condición inicial $C(0) = C_0$:
$$C(t) e^{kt} - C_0 = C_\infty (e^{kt} - 1)$$
Despejando $C(t)$:
$$C(t) = C_\infty + (C_0 - C_\infty) e^{-kt}$$
- **Constante de tiempo:** $\tau = \frac{1}{k}$ (tiempo requerido para alcanzar el $63.2\%$ del cambio).
- **Tiempo de estabilización al 95%:** $t_{95\%} \approx 3\tau = \frac{3}{k}$.

### Euler Progresivo vs Runge-Kutta 4to Orden (RK4)
Dado el problema de valor inicial $\frac{dC}{dt} = f(t, C) = k(C_\infty - C)$, con paso temporal discretizado $h = \Delta t$:

1. **Método de Euler:**
   $$C_{n+1} = C_n + h \cdot k(C_\infty - C_n) = (1 - hk) C_n + hk C_\infty$$
   - Error de truncamiento local: $O(h^2)$. Error global acumulado: $O(h)$.

2. **Método de Runge-Kutta de 4to Orden (RK4):**
   $$k_1 = f(C_n) = k(C_\infty - C_n)$$
   $$k_2 = f\left(C_n + \frac{h}{2}k_1\right) = k\left(C_\infty - \left(C_n + \frac{h}{2}k_1\right)\right)$$
   $$k_3 = f\left(C_n + \frac{h}{2}k_2\right) = k\left(C_\infty - \left(C_n + \frac{h}{2}k_2\right)\right)$$
   $$k_4 = f(C_n + h k_3) = k\left(C_\infty - (C_n + h k_3)\right)$$
   $$C_{n+1} = C_n + \frac{h}{6}(k_1 + 2k_2 + 2k_3 + k_4)$$
   - Error de truncamiento local: $O(h^5)$. Error global acumulado: $O(h^4)$.

### Estabilidad Asintótica de Dahlquist & Criterio de Lipschitz ($h < 2/k$)
La ecuación homogénea asociada es $\frac{dC}{dt} = -k C$, donde $\lambda = -k < 0$.
Aplicando el método de Euler:
$$C_{n+1} = C_n - hk C_n = (1 - hk) C_n = (1 - hk)^{n+1} C_0$$
Para que la solución discreta no amplifique exponencialmente el error y converja asintóticamente hacia cero ($C_n \to 0$ cuando $n \to \infty$), se exige que el factor de amplificación $\sigma = (1 - hk)$ pertenezca al interior del círculo unitario en el plano complejo:
$$|1 - hk| < 1 \iff -1 < 1 - hk < 1$$
Restando 1:
$$-2 < -hk < 0$$
Multiplicando por $-1$ (e invirtiendo desigualdades):
$$0 < hk < 2 \implies h < \frac{2}{k}$$

- **Paso Crítico ($h_{crítico} = 2/k$):**
  - Si $h < 2/k$: El esquema es numéricamente estable.
  - Si $h = 2/k$: El esquema oscila de forma permanente entre valores simétricos sin amortiguación.
  - Si $h > 2/k$: El esquema se vuelve inestable, diverge y genera oscilaciones catastróficas que desbordan los límites físicos.
- El módulo incluye un detector en tiempo real que evalúa si $h$ supera $h_{crítico}$, encendiendo una alerta roja parpadeante en la consola CRT.

### Tabla de Errores y Exportación Científica (CSV / LaTeX)
- **Métricas:**
  - Error Absoluto: $E_{abs} = |C_{\text{exacta}} - C_{\text{aprox}}|$
  - Error Relativo Porcentual: $E_{rel}\% = \frac{E_{abs}}{C_{\text{exacta}}} \times 100\%$
  - Error Absoluto Medio (MAE): $\text{MAE} = \frac{1}{N} \sum E_{abs}$
  - Raíz del Error Cuadrático Medio (RMSE): $\text{RMSE} = \sqrt{\frac{1}{N} \sum (E_{abs})^2}$
  - Multiplicador de Precisión (Accuracy Boost): $\frac{\text{RMSE}_{\text{Euler}}}{\text{RMSE}_{\text{RK4}}}$.
- **Exportación CSV:** Descarga directa de la tabla completa de $N$ pasos para procesamiento en Python/Pandas o MATLAB.
- **Exportación LaTeX:** Copia al portapapeles el bloque `\begin{table} ... \end{table}` formateado con librerías `booktabs` listo para pegar en Overleaf.

---

## 📐 Diagrama de Arquitectura Global

```
   +-------------------------------------------------------------------------+
   |                    NODO HARDWARE EDGE (ESP32 NODEMCU)                   |
   |   MQ-135 (ADC1-34) | MQ-7 (ADC1-35) | DHT11 (GPIO 15) | GC9A01 (SPI)    |
   |                                                                         |
   |   +-----------------------------------------------------------------+   |
   |   | Memoria Flash SPIFFS / LittleFS: offline_buffer.hpp             |   |
   |   | Cola: /offline_queue.jsonl (Cuota: 300 KB, ~3500 muestras)      |   |
   |   +-----------------------------------------------------------------+   |
   +-----------------------+-------------------------+-----------------------+
                           |                         |
              (WiFi OK: HTTP POST / Ingest)   (WiFi OK: Reconexión Lote 15)
              POST /api/telemetry/ingest      POST /api/telemetry/batch
                           |                         |
                           v                         v
   +-------------------------------------------------------------------------+
   |             BACKEND ENTERPRISE GATEWAY (NestJS + Bun SQLite)            |
   |                                                                         |
   |  • ZodValidationPipe: Sanitización estricta de tipos y física          |
   |  • TelemetryService: Transacción atómica db.transaction()              |
   |  • TelemetryGateway: WebSocket bidireccional (Heartbeat 30s)           |
   |  • AnalyticsService: Solvers EDO, Euler, RK4, Pearson & Benchmark      |
   |                                                                         |
   |  +-------------------------------------------------------------------+  |
   |  |                  DATA LAKEHOUSE MEDALLION (WAL)                   |  |
   |  |  [BRONZE] Ingesta cruda inmutable (raw_adc, timestamps originales)|  |
   |  |  [SILVER] Enriquecimiento (ΔPPM 1m, Predicción 15m, Risk Status)  |  |
   |  |  [GOLD]   Vistas agregadas por sesión geográfica                  |  |
   |  +-------------------------------------------------------------------+  |
   +------------------------------------+------------------------------------+
                                        |
                         +--------------+--------------+
                         |                             |
                         v (WebSocket 60 FPS)          v (REST / Inferencia)
   +------------------------------------+   +--------------------------------+
   |        FRONTEND DASHBOARD          |   |      AUDITORIO UNIVERSITARIO   |
   |             (Nuxt 3)               |   |   (AcademicAuditoriumModal)    |
   |                                    |   |                                |
   | • Encimera: Matraz SVG, Manómetro  |   | • Pizarra Deducción EDO        |
   | • Selector Dual: Local vs Live 1:1 |   | • Solvers Euler vs RK4 (h)     |
   | • LocationsMapBoard (Leaflet Tácto)|   | • Estabilidad h < 2/k          |
   | • Bitácora "Lugares Donde Estuvimos|   | • Tabla de Errores & LaTeX     |
   +------------------------------------+   +--------------------------------+
```

---

## 🔌 Mapa de Hardware & Conexiones del ESP32

> ⚠️ **Regla Eléctrica de Oro:** Los sensores de gas **MQ-135** y **MQ-7** consumen entre 150 y 200 mA para sus resistencias calefactoras. Deben conectarse obligatoriamente a **`VIN` (5V directo del bus USB)**, nunca a `3V3`.

| Dispositivo / Sensor | Pin Sensor | Pin ESP32 NodeMCU | Tipo de Conexión / Bus |
| :--- | :--- | :--- | :--- |
| **Pantalla Circular GC9A01** | VCC | 3V3 | Alimentación lógica 3.3V |
| | GND | GND | Tierra común |
| | SCL / SCK | **GPIO 18** | Bus SPI Clock |
| | SDA / MOSI | **GPIO 23** | Bus SPI MOSI |
| | DC | **GPIO 2** | Selector Comando / Datos |
| | CS | **GPIO 5** | Chip Select |
| | RST | **GPIO 4** | Reset de pantalla |
| | BLK | 3V3 | Backlight continuo |
| **Sensor Gas MQ-135 (eCO2/Nox)**| VCC | **VIN (5V)** | Calefactor de óxido metálico |
| | GND | GND | Tierra común |
| | A0 (Analógico) | **GPIO 34** | Canal ADC1_CH6 (Compatible con WiFi) |
| **Sensor Gas MQ-7 (CO)** | VCC | **VIN (5V)** | Calefactor monóxido de carbono |
| | GND | GND | Tierra común |
| | A0 (Analógico) | **GPIO 35** | Canal ADC1_CH7 (Compatible con WiFi) |
| **Sensor Temp/Hum DHT11** | VCC (+) | 3V3 | Alimentación 3.3V |
| | GND (-) | GND | Tierra común |
| | Señal (S) | **GPIO 15** | Digital One-Wire con Pull-up |
| **Sensor Inercial MPU-6050** | VCC | 3V3 | Alimentación 3.3V |
| | GND | GND | Tierra común |
| | SCL | **GPIO 22** | Bus I2C Clock |
| | SDA | **GPIO 21** | Bus I2C Data |

---

## 🚀 Guía de Puesta en Marcha Rápida (Bun Only)

> **Cero Tolerancia a NPM:** Este proyecto está optimizado y estandarizado para el runtime **Bun**. Queda estrictamente prohibido el uso de `npm`, `npx` o la generación de archivos `package-lock.json`.

### 1. Iniciar el Backend (NestJS + Bun)
```bash
cd /home/juankio/air-guardian-iot/backend
bun install
bun run dev
```
*El servidor levantará en `http://localhost:3002` con WebSocket activo en `ws://localhost:3002`.*

### 2. Iniciar el Frontend (Nuxt 3)
```bash
cd /home/juankio/air-guardian-iot/frontend
bun install
bun run dev
```
*La consola de laboratorio estará accesible en `http://localhost:3000`.*

### 3. Iniciar el Servidor Unificado (Concurrente)
Desde la raíz del repositorio:
```bash
cd /home/juankio/air-guardian-iot
bun start.ts
```

### 4. Compilar Firmware del ESP32 (PlatformIO)
```bash
cd /home/juankio/air-guardian-iot/firmware
pio run -t upload
```

---

## 📁 Estructura del Repositorio

```
air-guardian-iot/
├── README.md                              # Documento maestro y mapa de navegación
├── start.ts                               # Orquestador concurrente de servicios
├── docs/                                  # Documentación técnica profunda (Diataxis)
│   ├── ARQUITECTURA_DATALAKE_IOT.md       # Arquitectura Enterprise, Lakehouse y ADRs
│   ├── GUIA_CONEXIONES_HARDWARE.md        # Diagrama de pines, esquemáticos y voltajes
│   └── MODULO_ACADEMICO_EDO_NUMERICO.md   # Referencia matemática de EDO y Numérico
├── firmware/                              # Código fuente en C++ para ESP32
│   ├── platformio.ini                     # Configuración de compilación y librerías
│   └── src/
│       ├── main.cpp                       # Ciclo de lectura FreeRTOS y despacho
│       └── offline_buffer.hpp             # Gestor de cola LittleFS y vaciado batch
├── backend/                               # Servidor NestJS Enterprise en Bun
│   ├── src/
│   │   ├── common/                        # Pipes Zod, interceptores y filtros RFC
│   │   ├── database/                      # Conexión SQLite WAL y esquema Medallion
│   │   ├── telemetry/                     # Ingesta live, batch y WebSocket gateway
│   │   ├── sessions/                      # Geo-warehouse, campañas y mapas
│   │   ├── analytics/                     # Métricas Pearson, Benchmark y Solvers EDO
│   │   └── simulator/                     # Inyector de fluctuaciones sintéticas
│   └── test/                              # Suite de pruebas unitarias con bun test
└── frontend/                              # Dashboard Nuxt 3 App Shell Zero-Scroll
    ├── app.vue                            # Estación Mad Scientist y selector dual
    ├── composables/
    │   ├── useAirGuardian.ts              # Estado reactivo, WebSocket y conmutador
    │   └── useNumericalAcademic.ts        # Solvers reactivos Euler, RK4 y estabilidad
    └── components/
        ├── madlab/                        # Instrumentación Steampunk de laboratorio
        │   ├── LocationsMapBoard.vue      # Cartografía táctica Leaflet & Bitácora
        │   ├── TeslaAntenna.vue           # Monitor de enlace RF y batería
        │   ├── MascotChamber.vue          # Gemelo digital GC9A01 con IA emocional
        │   ├── AlchemyFlask.vue           # Matraz Erlenmeyer con líquido reactivo
        │   ├── AnalogPressureGauge.vue    # Manómetro analógico de aguja
        │   ├── FrankensteinSwitch.vue     # Palanca galvánica de Modo Dual
        │   ├── CrtOscilloscope.vue        # Pantalla catódica con inercia predictiva
        │   └── AlchemyMlBoard.vue         # Tubos Nixie con métricas de modelos
        └── academic/                      # Cátedra Universitaria
            ├── AcademicAuditoriumModal.vue# Modal de Cátedra EDO y Numérico
            ├── DifferentialEquationsTab.vue# Balance de masa y deducción analítica
            ├── NumericalSimulationTab.vue # Sliders de paso h y CRT comparativo
            └── ErrorAnalysisTab.vue       # Tabla paso a paso, exportación CSV/LaTeX
```

---
*Nami — Navegadora y Cartógrafa de Arquitectura. Todos los mapas trazados, rutas enlazadas y recursos asegurados.*
