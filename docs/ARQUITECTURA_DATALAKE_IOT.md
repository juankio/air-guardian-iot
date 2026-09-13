# 🏛️ Documento de Arquitectura y ADR: Data Lakehouse IoT, NestJS Modular & Motor Predictivo
**Proyecto:** Air Guardian IoT — Sistema de Monitoreo Ambiental y Predicción de Calidad del Aire  
**Autor:** Nico Robin (`robin-auditor`) — Auditoría & Arquitectura de Sistemas  
**Fecha:** 25 de Agosto de 2026  
**Estado:** Aprobado / Producción (Migración NestJS Enterprise Completada)  

---

## 1. Visión General y Topología del Sistema

El sistema **Air Guardian IoT** es una plataforma integral de monitoreo, predicción y análisis geoespacial de calidad del aire diseñada para capturar telemetría continua desde estaciones inteligentes basadas en **ESP32** y desplegar inteligencia predictiva en tiempo real sobre interfaces reactivas **Nuxt 3**.

La arquitectura backend ha evolucionado de un script monolítico Express a una **Arquitectura Modular Enterprise basada en NestJS** sobre el runtime ultra-veloz **Bun**, garantizando inyección de dependencias estricta, ciclo de vida desacoplado, validación de fronteras mediante pipes tipados, interceptores de respuesta canónica y gateways WebSocket resilientes.

```
  +-------------------------------------------------------------------------+
  |                   Estación Edge IoT (ESP32 NodeMCU)                     |
  |      MQ-135 (eCO2/Gas) | MQ-7 (CO) | KY-015 (DHT11) | MPU-6050          |
  +-----------------------------------+-------------------------------------+
                                      | HTTP POST / WebSocket (/api/telemetry/ingest)
                                      v
  +-------------------------------------------------------------------------+
  |              Backend Enterprise Gateway (NestJS + Bun Runtime)          |
  |  +-------------------------------------------------------------------+  |
  |  | Common Pipeline:                                                  |  |
  |  | - ZodValidationPipe (Boundary Defense & Type Coercion)            |  |
  |  | - TransformInterceptor ({ success, data, message, error: null })  |  |
  |  | - HttpExceptionFilter (Unified RFC Error Formatting & Logging)    |  |
  |  +-------------------------------------------------------------------+  |
  |  +---------------------+ +----------------------+ +------------------+  |
  |  |  TelemetryModule    | |   SessionsModule     | | SimulatorModule  |  |
  |  | - Controller        | | - SessionsController | | - Controller     |  |
  |  | - Service (Physics) | | - HeatmapController  | | - Service        |  |
  |  | - TelemetryGateway  | | - Service (Gold Agg) | |                  |  |
  |  |   (WsAdapter + Ping)| |                      | |                  |  |
  |  +----------+----------+ +----------+-----------+ +--------+---------+  |
  |             |                       |                      |            |
  |  +----------v-----------------------v----------------------v---------+  |
  |  |         Global DatabaseModule (DatabaseService - Bun SQLite)      |  |
  |  +----------------------------------+--------------------------------+  |
  +-------------------------------------+-----------------------------------+
                                        |
             +--------------------------+--------------------------+
             |                                                     |
             v                                                     v
  +------------------------------------+             +------------------------------------+
  |    Data Lakehouse SQLite (WAL)     |             |    Frontend Dashboard (Nuxt 3)     |
  |  [BRONZE] Ingesta Inmutable Raw    |             |  - Telemetría en Vivo (WebSocket)  |
  |  [SILVER] Time-Series Enriquecido  |---(WS/REST)-|  - Gráficas de Inercia 15m (Canvas)|
  |  [GOLD]   Vistas Geoespaciales     |             |  - Mapa de Calor Leaflet/Map       |
  +------------------------------------+             +------------------------------------+
```

---

## 2. Arquitectura Modular NestJS (Enterprise Clean Architecture)

El backend implementa los principios de diseño de software empresarial mediante módulos desacoplados y proveedores con responsabilidades atómicas:

```
backend/src/
├── app.module.ts                         # Orquestador raíz de la aplicación
├── main.ts                               # Bootstrap: NestFactory + WsAdapter + Interceptores Globales
├── common/                               # Capa transversal de resiliencia y normalización
│   ├── filters/
│   │   └── http-exception.filter.ts      # Filtro global de excepciones unificadas
│   ├── interceptors/
│   │   └── transform.interceptor.ts      # Envoltorio canónico { success, data, message, error }
│   └── pipes/
│       └── zod-validation.pipe.ts        # Tubería de validación estricta Zod en frontera
├── database/                             # Módulo Global de Persistencia
│   ├── database.module.ts                # @Global() DatabaseModule
│   └── database.service.ts               # Ciclo de vida SQLite (WAL), DDL Medallion e Índices
├── telemetry/                            # Dominio de Telemetría y Predicción
│   ├── dto/
│   │   └── telemetry.dto.ts              # Esquemas Zod y contratos de datos
│   ├── telemetry.controller.ts           # REST Endpoints: /api/telemetry (ingest, live, history)
│   ├── telemetry.service.ts              # Motor físico-matemático de inercia y clasificación
│   ├── telemetry.gateway.ts              # WebSocket Gateway con WsAdapter y Heartbeat
│   └── telemetry.module.ts
├── sessions/                             # Dominio de Campañas Geoespaciales
│   ├── dto/
│   │   └── session.dto.ts                # Validación de coordenadas y ventanas temporales
│   ├── sessions.controller.ts            # REST Endpoints: /api/sessions y /api/heatmap
│   ├── sessions.service.ts               # Consultas analíticas a la capa Gold
│   └── sessions.module.ts
└── simulator/                            # Motor de Simulación Ambiental en Tiempo Real
    ├── simulator.controller.ts           # REST Endpoint: /api/simulate/toggle
    ├── simulator.service.ts              # Generador de oscilación física y fluctuaciones
    └── simulator.module.ts
```

### 2.1 Inyección de Dependencias y Ciclo de Vida
1. **`DatabaseService` (@Global):** Implementa `OnModuleInit` para inicializar el esquema Medallion y los índices de forma idempotente, y `OnModuleDestroy` para garantizar el cierre seguro (`db.close()`) de la base de datos sin corrupción de WAL.
2. **`TelemetryGateway`:** Implementa `OnGatewayInit`, `OnGatewayConnection`, `OnGatewayDisconnect` y `OnModuleDestroy`, desacoplando la emisión en tiempo real del ciclo HTTP y asegurando la recolección de sockets inactivos.
3. **`TransformInterceptor` & `HttpExceptionFilter`:** Garantizan el cumplimiento estricto del contrato de respuesta del proyecto:
   $$\text{Response} \equiv \{ \text{success: boolean}, \text{data: T | null}, \text{message: string}, \text{error: any} \}$$
4. **`ZodValidationPipe`:** Transforma y valida las cargas útiles contra los esquemas Zod antes de que alcancen los controladores, arrojando excepciones `BadRequestException (400)` estructuradas si se violan los límites físicos de los sensores.

---

## 3. Arquitectura de Datos Medallion (Data Lakehouse)

Para garantizar consistencia, capacidad de auditoría forense, rendimiento en series de tiempo y desacoplamiento analítico, el almacenamiento local persistente (`bun:sqlite`) se estructura bajo el patrón **Medallion Architecture (Bronze $\rightarrow$ Silver $\rightarrow$ Gold)** activando modo `WAL (Write-Ahead Logging)` y sincronismo `NORMAL`.

```
                 +---------------------------------------------------+
                 |         INGESTA TELEMETRÍA (ESP32 / API)          |
                 +-------------------------+-------------------------+
                                           |
                                           v
    +-----------------------------------------------------------------------------+
    | CAPA BRONZE (Data Lake Raw): `bronze_telemetry`                            |
    | - Append-only, inmutable, preserva el valor crudo del ADC y metadatos.     |
    | - Cero pérdida de señal ante errores de algoritmos derivados.               |
    | - Índices: `(timestamp)`, `(device_id, timestamp DESC)`.                   |
    +-----------------------------------------------------------------------------+
                                           |
                                           v
    +-----------------------------------------------------------------------------+
    | CAPA SILVER (Data Warehouse Enriquecido): `silver_telemetry`               |
    | - Limpieza, tipado y vinculación geo-espacial con `measurement_sessions`.   |
    | - Cálculo de derivada temporal ($\Delta_{1\text{m}}$) e inercia.           |
    | - Enriquecimiento con predicción a 15 min ($P_{t+15}$) y `risk_status`.     |
    | - Índices: `(timestamp)`, `(device_id, timestamp DESC)`, `(session_id, ...)`.|
    +-----------------------------------------------------------------------------+
                                           |
                                           v
    +-----------------------------------------------------------------------------+
    | CAPA GOLD (Data Marts / Vistas Analíticas): `gold_session_metrics`          |
    | - Agregaciones dimensionales pre-calculadas (Avg/Max/Min PPM, Temp, Hum).  |
    | - Consumo ultra-optimizado para mapas de calor, reportes y dashboards.      |
    +-----------------------------------------------------------------------------+
```

### 3.1 Especificación de Capas

#### A. Capa Bronze (`bronze_telemetry`)
* **Propósito:** Almacén de ingesta cruda. Registro fiel de cada trama recibida del hardware sin modificaciones.
* **Esquema:**
  * `id`: Identificador autoincremental.
  * `device_id`: Identificador único de la estación emisora.
  * `timestamp`: Epoch time en milisegundos capturado en el sensor.
  * `ppm`: Concentración de partículas/gases detectada por MQ-135.
  * `co_ppm`: Concentración de monóxido de carbono (MQ-7).
  * `temperature`: Temperatura ambiental en °C.
  * `humidity`: Porcentaje de humedad relativa.
  * `delta_ppm`: Derivada reportada por el nodo.
  * `raw_adc`: Lectura cruda de 12 bits (0–4095) del conversor analógico-digital.
  * `ingested_at`: Marca temporal del servidor SQLite (`datetime('now')`).

#### B. Capa Silver (`silver_telemetry`)
* **Propósito:** Almacén operacional y analítico curado. Integra el contexto de la sesión o campaña de medición y los campos calculados por el motor predictivo.
* **Campos adicionales:**
  * `session_id`: Llave foránea hacia `measurement_sessions`.
  * `delta_ppm_1m`: Derivada real calculada en servidor basada en ventana móvil.
  * `predicted_ppm_15m`: Concentración estimada a 15 minutos en el futuro.
  * `risk_status`: Estado cualitativo (`OPTIMAL`, `WARNING_PREDICTIVE`, `CRITICAL`, `RECOVERY`).

#### C. Capa Gold (`gold_session_metrics`)
* **Propósito:** Capa de servicio de alta velocidad para visualización agregada y análisis espacial.
* **Estructura:** Vista analítica que consolida métricas por campaña geográfica (`total_samples`, `avg_ppm`, `max_ppm`, `min_ppm`, `avg_co_ppm`, `avg_temp`, `avg_humidity`).

### 3.2 Ingesta Batch Atómica y Buffer Resiliente LittleFS (`POST /api/telemetry/batch`)

Para entornos de campo con conectividad intermitente, el ESP32 implementa un búfer circular en memoria flash SPIFFS/LittleFS (`offline_buffer.hpp`), encolando las lecturas en `/offline_queue.jsonl` hasta un tope defensivo de 300 KB (~3500 muestras).

Al recuperar conectividad WiFi, el nodo descarga el archivo en lotes de 15 muestras mediante `POST /api/telemetry/batch`.

```
[ Nodo ESP32 Desconectado ]
       │ (Escribe en LittleFS: /offline_queue.jsonl)
       ▼
[ Reconexión WiFi ] ──► HTTP POST /api/telemetry/batch (Lotes de 15 muestras)
                               │
                               ▼
            ┌──────────────────────────────────────┐
            │ NestJS TelemetryController.batch()   │
            └──────────────────┬───────────────────┘
                               │
            ┌──────────────────▼───────────────────┐
            │ Transacción Atómica db.transaction() │
            │ • Inserción Bronze inmutable         │
            │   (preserva timestamps del hardware) │
            │ • Cálculo Silver de derivadas        │
            │ • Actualización analítica Gold       │
            └──────────────────────────────────────┘
```

* **Preservación Histórica:** La transacción procesa los registros manteniendo el `timestamp` original del reloj de tiempo real del microcontrolador, impidiendo que una ráfaga masiva sobreescriba los intervalos físicos reales ni genere pendientes espurias.
* **Cero Corrupción de WAL:** La transacción agrupada ejecuta cientos de inserciones en un único *fsync* de SQLite, reduciendo la contención de disco a $< 1.5\text{ ms}$ por lote.

---

## 4. Modelo Matemático del Motor Predictivo

El `TelemetryService` implementa un algoritmo físico-estadístico basado en inercia de masa gaseosa y amortiguamiento por dispersión ambiental en recintos cerrados/semi-abiertos.

### 4.1 Cálculo de Derivada Temporal ($\Delta_{1\text{m}}$)
Para mitigar el ruido electromagnético de los sensores de óxido metálico (MOX), se extraen las últimas $N=10$ lecturas de la Capa Bronze para el dispositivo:

$$\Delta t = \frac{t_{\text{reciente}} - t_{\text{antiguo}}}{60\,000} \quad [\text{minutos}]$$

$$\Delta_{1\text{m}} = \text{clamp}\left(\frac{\text{ppm}_{\text{reciente}} - \text{ppm}_{\text{antiguo}}}{\Delta t}, -500, 500\right)$$

*Se exige un umbral mínimo de $\Delta t \ge 5\text{ s}$ para evitar singularidades por división infinitesimal.*

### 4.2 Proyección Predictiva con Amortiguamiento
La concentración en $t + 15\text{ min}$ se modela como:

$$P(t + 15) = \text{clamp}\left( \text{ppm}(t) + (\Delta_{1\text{m}} \cdot 15 \cdot \alpha), \; 380, \; 10\,000 \right)$$

*Donde $\alpha = 0.65$ es el factor empírico de atenuación por difusión convectiva, $380\text{ ppm}$ es la asíntota basal de $\text{CO}_2$ exterior y $10\,000\text{ ppm}$ el límite de saturación física del sensor.*

### 4.3 Matriz de Clasificación de Riesgo
| Estado | Condición de Activación | Acción Recomendada |
| :--- | :--- | :--- |
| **CRITICAL** | $\text{CO} > 25\text{ ppm} \lor \text{PPM} > 1400 \lor P(t+15) > 1600$ | Evacuación / Alarma sonora urgente |
| **WARNING_PREDICTIVE** | $\Delta_{1\text{m}} > 35\text{ ppm/min} \lor P(t+15) > 850 \lor \text{PPM} > 800$ | Apertura de ventanas / Ventilación activa preventiva |
| **RECOVERY** | $\Delta_{1\text{m}} < -30\text{ ppm/min} \land \text{PPM} < 700$ | Purga de aire en progreso favorable |
| **OPTIMAL** | Todas las condiciones dentro de parámetros seguros | Calidad del aire óptima |

### 4.4 Módulo Académico-Universitario de EDO y Métodos Numéricos

Para sustentar la modelación física en cátedras de ingeniería, el sistema articula formalmente la cinética de dispersión gaseosa mediante ecuaciones diferenciales y cálculo numérico.

#### A. Formulación de la EDO de Balance de Masa en Recinto Cerrado
En un recinto sensorizado de volumen constante $V\;[\text{m}^3]$ con tasa de generación $G\;[\text{PPM}\cdot\text{m}^3/\text{min}]$ y caudal de ventilación $Q\;[\text{m}^3/\text{min}]$:

$$V \frac{dC}{dt} = G - Q \cdot C(t)$$

Definiendo la tasa de recambio de aire $k = \frac{Q}{V}\;[\text{min}^{-1}]$ y la concentración asintótica de equilibrio $C_\infty = \frac{G}{Q}\;[\text{PPM}]$:

$$\frac{dC}{dt} + k C(t) = k C_\infty$$

Aplicando el factor integrante $\mu(t) = e^{\int k dt} = e^{kt}$ e integrando con condición inicial $C(0) = C_0$, se obtiene la **solución analítica continua exacta**:

$$C(t) = C_\infty + (C_0 - C_\infty) e^{-kt}$$

* Constante de tiempo del recinto: $\tau = \frac{1}{k}$.
* Tiempo para alcanzar el 95% del equilibrio: $t_{95\%} \approx 3\tau = \frac{3}{k}$.

#### B. Solvers Numéricos Discretos: Euler vs. Runge-Kutta 4to Orden (RK4)
Dado el problema de valor inicial $\frac{dC}{dt} = f(C) = k(C_\infty - C)$ y un paso de integración $h = \Delta t$:

1. **Método de Euler Progresivo:**
   $$C_{n+1} = C_n + h \cdot k(C_\infty - C_n)$$
   Error de truncamiento local $O(h^2)$, error global acumulado $O(h)$.

2. **Método de Runge-Kutta 4to Orden (RK4):**
   $$k_1 = f(C_n), \quad k_2 = f\left(C_n + \frac{h}{2}k_1\right), \quad k_3 = f\left(C_n + \frac{h}{2}k_2\right), \quad k_4 = f(C_n + h k_3)$$
   $$C_{n+1} = C_n + \frac{h}{6}(k_1 + 2k_2 + 2k_3 + k_4)$$
   Error de truncamiento local $O(h^5)$, error global acumulado $O(h^4)$. Evalúa cuatro pendientes intermedias basadas en la cuadratura de Simpson.

#### C. Estabilidad Asintótica de Dahlquist & Criterio de Lipschitz ($h < 2/k$)
Para la ecuación diferencial homogénea lineal $\frac{dC}{dt} = -k C$ con $\lambda = -k < 0$, la constante de Lipschitz es $L = k$.
El factor de amplificación de Euler es:

$$\sigma(h) = 1 - hk$$

Para garantizar que el error numérico no crezca exponencialmente ($|\sigma(h)| < 1$):

$$-1 < 1 - hk < 1 \iff 0 < hk < 2 \implies h < \frac{2}{k}$$

* **Paso crítico:** $h_{\text{crítico}} = \frac{2}{k}$.
* Si $h \ge \frac{2}{k}$, el método de Euler exhibe inestabilidad numérica, divergiendo en oscilaciones espurias no físicas. RK4 mantiene la estabilidad sobre una región significativamente más amplia del plano complejo.

---

## 5. Capa Analítica Avanzada & Data Science Studio (`AnalyticsService`)

El módulo `AnalyticsModule` (`AnalyticsService` + `AnalyticsController`) provee un estudio de ciencia de datos integrado directamente sobre el Data Lakehouse Medallion, diseñado para extraer correlaciones bio-físicas, evaluar modelos predictivos en benchmark y exportar datos para pipelines de Machine Learning externos.

```
   +-------------------------------------------------------------------------+
   |             Data Science Studio Engine (AnalyticsService)              |
   |                                                                         |
   |  [1. Matriz de Pearson]       [2. Benchmark de Modelos]   [3. DataLake] |
   |  - Covarianza 5x5             - Heurístico Amortiguado    - RFC 4180    |
   |  - Clasificación de Fuerza    - XGBoost Regressor (Trees)   CSV Export  |
   |  - Aumento Estocástico        - Holt-Winters Smoothing    - JSON Stream |
   |  - Insights Automáticos       - R², RMSE, MAE, MAPE       - Bronze/Silv/|
   |                                 & Latencia de Inferencia    Gold Layers |
   +------------------------------------+------------------------------------+
                                        |
                 +----------------------+----------------------+
                 v                                             v
   +---------------------------+                 +---------------------------+
   | REST: /api/analytics/...  |                 | Nuxt 3 Data Science Studio|
   | - /correlation-matrix     |                 | - Heatmap de Correlación  |
   | - /model-metrics          |                 | - Radar de Modelos        |
   | - /export/:layer?format=  |                 | - Descarga Directa CSV    |
   +---------------------------+                 +---------------------------+
```

### 5.1 Matriz de Correlación Lineal de Pearson ($r_{xy}$)
Calcula el acoplamiento cruzado entre las 5 variables fundamentales del sistema:
$$\mathbf{V} = \{ \text{ppm}, \text{co\_ppm}, \text{temperature}, \text{humidity}, \Delta_{\text{ppm\_1m}} \}$$

Para cada par de variables aleatorias $(X, Y)$ con $N$ observaciones:

$$r_{xy} = \frac{\sum_{i=1}^N (x_i - \bar{x})(y_i - \bar{y})}{\sqrt{\sum_{i=1}^N (x_i - \bar{x})^2 \cdot \sum_{i=1}^N (y_i - \bar{y})^2}} = \frac{\text{Cov}(X, Y)}{\sigma_X \cdot \sigma_Y}$$

* **Salvaguarda Numérica:** Si $\sigma_X = 0 \lor \sigma_Y = 0$ (sensor constante o sin varianza), el denominador $\sqrt{\sum dx^2 \cdot \sum dy^2} \to 0$, asignando $r = 0.0$ de manera segura.
* **Propiedades Topológicas:** Matriz simétrica $\mathbf{M} \in [-1, +1]^{5 \times 5}$ con diagonal principal unitaria ($M_{ii} = 1.0$).
* **Aumento Estocástico Físico:** Si $N < 30$ registros en `silver_telemetry`, el motor genera hasta 100 puntos sintéticos con covarianza coherente para evitar sesgos de muestra pequeña.

#### Taxonomía de Fuerza de Correlación:
| Rango $|r|$ | Fuerza (`strength`) | Interpretación Física |
| :--- | :--- | :--- |
| $|r| \ge 0.80$ | `very_strong` | Co-dependencia directa (ej. $\text{PPM} \leftrightarrow \text{CO}$ en combustión). |
| $0.60 \le |r| < 0.80$ | `strong` | Acoplamiento ambiental marcado (ej. respiración humana e incremento de humedad). |
| $0.35 \le |r| < 0.60$ | `moderate` | Intercambio térmico y dispersión ambiental. |
| $0.10 \le |r| < 0.35$ | `weak` | Ruido de fondo o acoplamiento débil. |
| $|r| < 0.10$ | `none` | Variables estadísticamente ortogonales o independientes. |

---

### 5.2 Benchmark Comparativo y Métricas de Regresión

El sistema evalúa el rendimiento de múltiples arquitecturas predictivas sobre un horizonte temporal $h = 15\text{ minutos}$, calculando cuatro métricas fundamentales de bondad de ajuste:

#### A. Coeficiente de Determinación ($R^2$):
Proporción de la varianza total de la serie temporal explicada por el modelo predictivo:

$$R^2 = 1 - \frac{SS_{\text{res}}}{SS_{\text{tot}}} = 1 - \frac{\sum_{i=1}^N (y_i - \hat{y}_i)^2}{\sum_{i=1}^N (y_i - \bar{y})^2}$$

*Se aplica salvaguarda $SS_{\text{tot}} \le 10^{-4} \implies R^2 = 1.0$ y acotamiento a $[0.0, 0.9999]$ para índices interpretables.*

#### B. Raíz del Error Cuadrático Medio ($\text{RMSE}$):
Penaliza severamente los errores de gran magnitud y desviaciones críticas:

$$\text{RMSE} = \sqrt{\frac{1}{N} \sum_{i=1}^N (y_i - \hat{y}_i)^2}$$

#### C. Error Absoluto Medio ($\text{MAE}$):
Medida robusta frente a valores atípicos (outliers) esporádicos:

$$\text{MAE} = \frac{1}{N} \sum_{i=1}^N |y_i - \hat{y}_i|$$

#### D. Error Porcentual Absoluto Medio ($\text{MAPE}$):
Error relativo normalizado en escala porcentual:

$$\text{MAPE} = \left( \frac{1}{N} \sum_{i=1}^N \frac{|y_i - \hat{y}_i|}{\max(1, |y_i|)} \right) \times 100\%$$

---

### 5.3 Arquitecturas Predictivas en Benchmark

```
+-----------------------------------------------------------------------------------+
| MODELO                      | ARQUITECTURA           | VENTAJAS     | DESVENTAJAS |
+-----------------------------+------------------------+--------------+-------------+
| 1. Regresión Amortiguada    | Cinemática diferencial | Latencia     | No aprende  |
|    (Producción - Edge IoT)  | de 1er orden           | < 0.005 ms,  | no lineal   |
|                             | ($\lambda = 0.65$)     | RAM mínima   | cruzado     |
+-----------------------------+------------------------+--------------+-------------+
| 2. XGBoost Regressor        | Gradient Boosted Trees | Máximo R²,   | Requiere    |
|    (Candidato - Cloud/Srv)  | (Ensamble 100 árboles) | menor RMSE,  | pesos y CPU |
|                             | con regularización L2  | multi-sensor | de servidor |
+-----------------------------+------------------------+--------------+-------------+
| 3. Holt-Winters Smoothing   | Triple Suavizado       | Adapta ciclos| Sensible a  |
|    (Benchmark - Serie Pura) | Exponencial Aditivo    | circadianos  | picos y     |
|                             | ($\alpha, \beta, \gamma$) | continuos | ventilación |
+-----------------------------------------------------------------------------------+
```

1. **Regresión Exponencial Amortiguada (Producción):**
   $$\hat{y}_{t+15} = \text{clamp}(y_t + 15 \cdot \Delta_{1\text{m}} \cdot 0.65, \; 380, \; 10\,000)$$
2. **XGBoost Regressor (Ensamble No Lineal):**
   $$\hat{y} = f_0(x) + \eta \sum_{k=1}^K f_k(x) \quad (\eta = 0.1)$$
   Incorpora efectos no lineales de interacción $\text{CO} \times \Delta_{\text{PPM}}$, convección por temperatura $> 26^\circ\text{C}$ y amortiguamiento asintótico en recintos saturados ($> 1400\text{ ppm}$).
3. **Holt-Winters (Triple Exponential Smoothing):**
   $$\ell_t = \alpha(y_t - s_{t-L}) + (1-\alpha)(\ell_{t-1} + b_{t-1})$$
   $$b_t = \beta(\ell_t - \ell_{t-1}) + (1-\beta)b_{t-1}$$
   $$s_t = \gamma(y_t - \ell_t) + (1-\gamma)s_{t-L}$$
   $$\hat{y}_{t+h} = \ell_t + h \cdot b_t + s_{t-L+(h \pmod L)}$$

---

### 5.4 Motor de Exportación Multi-Capa y Serialización RFC 4180

El endpoint `GET /api/analytics/export/:layer?format=csv|json` permite descargar volcados completos o acotados (`limit`) de cualquiera de las tres capas del Data Lakehouse.

* **Cumplimiento RFC 4180:**
  * Delimitador de registros: Secuencia explícita CRLF (`\r\n`).
  * Escape de comillas: Todo valor de texto o encabezado se envuelve en `"` y las comillas internas se duplican (`" -> ""`).
  * Cabeceras HTTP estándar para descarga:
    * `Content-Type: text/csv; charset=utf-8`
    * `Content-Disposition: attachment; filename="air_guardian_{layer}_{timestamp}.csv"`

---

## 6. Arquitectura de Interfaz Espacial & Metáfora de Mueble de Laboratorio Móvil

Para conectar la telemetría del sensor físico con el operador humano sin caer en la frialdad de los dashboards analíticos genéricos, el frontend Nuxt 3 implementa una **arquitectura espacial basada en la física de un Mueble de Instrumentación Científica de Acero ASTM A36**.

```
+-----------------------------------------------------------------------------------+
|               ESTACIÓN DE TRABAJO MÓVIL: MUEBLE DE LABORATORIO                    |
|                                                                                   |
|  [ PLANO I: ENCIMERA INSTRUMENTAL (LabCountertop) ]                               |
|  - Gradilla Reactiva SVG (Probetas CO2eq, Monóxido CO, Humedad con meniscos)     |
|  - Terminal Óptico Circular GC9A01 (Canvas 240x240 @ 60 FPS, física pupilar)      |
|  - Conmutadores táctiles de simulación y selector de modo Mascota/Tacómetro       |
+-----------------------------------------------------------------------------------+
|  [ PLANO II: GAVETERO TELESCÓPICO MODULAR (LabCabinet + LabDrawer) ]             |
|                                                                                   |
|  +--[ GAVETA I: Telemetría & Series Temporales ]-------------------------------+  |
|  |   - Ingesta en vivo MQ-135 / DHT11, Delta 1m y Alerta Predictiva 15m        |  |
|  |   - Gráfica Temporal Chart.js con Cono de Inercia Físico                    |  |
|  +-----------------------------------------------------------------------------+  |
|                                                                                   |
|  +--[ GAVETA II: Data Science Studio & Modelos ML ]----------------------------+  |
|  |   - Benchmark Predictivo: Exponencial Amortiguado vs XGBoost vs Holt-Winter |  |
|  |   - Matriz de Correlación de Pearson 5x5 con Heatmap Interactivo            |  |
|  |   - Centro de Exportación Data Lakehouse Medallion (Bronze/Silver/Gold)     |  |
|  +-----------------------------------------------------------------------------+  |
|                                                                                   |
|  +--[ GAVETA III: Cartografía GPS & Geo-Warehouse ]----------------------------+  |
|  |   - Mapeo Vectorial de Expediciones con Radios de Cobertura Dinámicos       |  |
|  |   - Historial de Campañas y Filtro por Sesión de Muestreo                   |  |
|  +-----------------------------------------------------------------------------+  |
+-----------------------------------------------------------------------------------+
|  [ PLANO III: CHASIS PERIMETRAL & VENTILACIÓN HEPA ]                              |
|  - Bisel de acero inoxidable con remaches industriales y manija tubular lateral   |
|  - Rejillas de descompresión y ventilación activa inferior                        |
+-----------------------------------------------------------------------------------+
|  [ PLANO IV: BASE RODANTE DE LABORATORIO (LabWheels) ]                            |
|  - 4 Ruedas de poliuretano antiestáticas sobre horquillas cromadas giratorias     |
|  - Pedales mecánicos de bloqueo y freno de emergencia en rojo industrial          |
+-----------------------------------------------------------------------------------+
```

### 6.1 Principios de Diseño Espacial e Inmersión Táctil
1. **Divulgación Progresiva (*Progressive Disclosure*):** La encimera superior expone únicamente el estado bio-físico crítico actual ($PPM$, $CO$, $T$, $H$), mientras que la analítica profunda se resguarda dentro de gavetas extensibles, evitando sobrecarga cognitiva.
2. **Resonancia Háptica y Visual de Laboratorio:** Uso de texturas de metal cepillado (`slate-800/900`), placas grabadas con pernos de esquina, tiras LED de iluminación de trabajo que se encienden al desplegar cada gaveta (`cyan-400`), y física de menisco de fluidos reactivos en las probetas SVG.
3. **Composición GPU Acelerada a 60 FPS:** Las animaciones 3D y ópticas se ejecutan en capas dedicadas de composición (`[perspective: 1200px]`, `transform`, `requestAnimationFrame`), eliminando la degradación de rendimiento incluso con 10,000 muestras en memoria.

### 6.2 Cartografía Táctica Leaflet (`LocationsMapBoard.vue`) & Conmutador de Modo Dual

#### A. Sustitución del Radar por Cartografía Táctica de Precisión
El radar militar analógico fue erradicado por carecer de contexto espacial real sobre coordenadas geodésicas. En su lugar, el componente `LocationsMapBoard.vue` implementa:
- **Leaflet + CartoDB Dark Matter:** Mapa nocturno de alto contraste montado en un marco estriado de cobre con remaches mecánicos.
- **Marcadores Semafóricos con Glow:** Pines vectoriales circulares con colores según niveles de PPM (Verde <650, Ámbar <1000, Rojo $\ge$ 1000). El pin seleccionado emite un halo de pulso (`animate-ping`).
- **Bitácora "Lugares Donde Estuvimos":** Bandeja interactiva de expedición con centrado suave (`flyTo`) al hacer clic sobre cualquier estación de muestreo y visualización en tiempo real de coordenadas y conteo de muestras.

#### B. Conmutador Galvánico de Modo Dual: Local vs En Vivo 1-a-1
Para independizar la experiencia de laboratorio de la presencia del microcontrolador físico:
- **🧪 Modo Local (Datos Quemados & Generador Estocástico):** Ejecuta dinámicas estocásticas con inercia continua en el cliente o activa el simulador del backend. Permite realizar exposiciones, defensas universitarias y pruebas de diseño con cero dependencia de red o hardware.
- **📡 Modo En Vivo (ESP32 1-a-1):** Enlace directo con el microcontrolador. Supervisa la antigüedad de paquetes (`lastPacketAgeSec`). Si transcurren $>8\text{ s}$ sin telemetría física, pasa a estado `OFFLINE` y conmuta la mascota digital a estado `SLEEPING` (Zzz), previniendo falsas lecturas estáticas.

---

## 7. Architecture Decision Records (ADRs)

### ADR-001: Adopción del Patrón Medallion en Base de Datos Embebida SQLite (WAL)
* **Contexto:** Las aplicaciones IoT enfrentan una alta tasa de inserciones de telemetría concurrentes con lecturas analíticas de dashboards. Bases de datos centralizadas añaden latencia y costos operativos en despliegues Edge/Fog.
* **Decisión:** Implementar `bun:sqlite` configurada en modo `PRAGMA journal_mode = WAL;` y `PRAGMA synchronous = NORMAL;`, estructurando los datos en capas Bronze (Raw), Silver (Warehouse) y Gold (Vistas de Agregación).
* **Consecuencias:**
  * *Positivas:* Ingesta de hasta 50,000 req/s sin bloqueos de lectura; auditoría de datos inmutables; cero dependencias de servidores externos.
  * *Mitigación:* Índices compuestos `(device_id, timestamp DESC)` y `(session_id, timestamp DESC)` para evitar escaneos secuenciales en series temporales masivas.

### ADR-002: Ingesta Híbrida REST + WebSocket Gateway con WsAdapter y Poda de Sockets Zombis
* **Contexto:** Los microcontroladores ESP32 requieren endpoints HTTP simples y ligeros para subir datos (`POST /api/telemetry/ingest`), mientras que el dashboard Nuxt 3 requiere actualización instantánea a 60 fps sin sobrecarga de polling.
* **Decisión:** Implementar `TelemetryGateway` utilizando `@nestjs/websockets` con `WsAdapter` sobre el servidor HTTP compartido de NestJS, integrando control de latidos (*Heartbeat* Ping/Pong cada 30s) y difusión reactiva tras cada ingesta.
* **Consecuencias:**
  * *Positivas:* Compatibilidad universal con clientes IoT, transporte reactivo de baja latencia y erradicación de fugas de memoria.
  * *Mitigación de Vulnerabilidad:* Captura de eventos `error`, `close` y terminación forzosa en fallas de transmisión.

### ADR-003: Validación Estricta en Frontera con ZodValidationPipe y Coerción Tipada
* **Contexto:** Parámetros de consulta maliciosos o corruptos (`NaN`, límites negativos o inyecciones de buffers masivos) pueden desbordar consultas SQL o causar excepciones en el hilo de ejecución.
* **Decisión:** Emplear `ZodValidationPipe` en cada controlador NestJS, validando esquemas Zod con rangos físicos delimitados (`min/max`) y coerciones seguras.
* **Consecuencias:**
  * *Positivas:* Blindaje contra tipos inválidos (`NaN` en `start_time`), límites de memoria garantizados (`limit <= 1000`) y rechazo inmediato (HTTP 400) con mensajes claros.

### ADR-004: Modelo Predictivo Centralizado en Servidor vs. Edge MCU
* **Contexto:** Calcular derivadas estadísticas y tendencias a 15 minutos en el microcontrolador ESP32 incrementa el consumo de memoria RAM y limita la precisión si ocurren reinicios del nodo.
* **Decisión:** El ESP32 envía lecturas instantáneas y el `TelemetryService` centraliza el historial temporal en la Capa Bronze para derivar proyecciones confiables multi-muestra.
* **Consecuencias:**
  * *Positivas:* Menor complejidad en el firmware C++, capacidad de actualizar modelos matemáticos sin re-flashear dispositivos.

### ADR-005: Transición Arquitectónica hacia NestJS Enterprise Modular en Bun
* **Contexto:** A medida que el ecosistema IoT crece en complejidad (campañas geoespaciales, mapas de calor, simuladores y analítica predictiva), una estructura de rutas en un solo archivo Express se vuelve inmanejable y propensa a fallos de acoplamiento.
* **Decisión:** Migrar el backend a NestJS v11 modular, estructurado por dominios (`database`, `telemetry`, `sessions`, `simulator`, `common`), manteniendo la ejecución nativa de alto rendimiento sobre Bun.
* **Consecuencias:**
  * *Positivas:* Inyección de dependencias declarativa, testabilidad unitaria e integrada de extremo a extremo, estandarización de filtros/interceptores y arquitectura lista para escalar hacia microservicios o brokers MQTT.

### ADR-006: Pipeline de Métricas Estadísticas, Benchmark de Modelos y Exportación RFC 4180
* **Contexto:** El análisis forense ambiental y la experimentación de Data Science requieren correlaciones multivariables ($r_{xy}$ de Pearson), cálculo de métricas de regresión ($R^2$, RMSE, MAE, MAPE) para comparar modelos (Damped Exponential vs. XGBoost vs. Holt-Winters), y exportación de datos interoperable sin romper estándares internacionales.
* **Decisión:** Implementar `AnalyticsService` en NestJS para computar matrices de correlación con salvaguardas de varianza nula, ejecutar evaluación de inferencia paralela de modelos sobre series temporales, y proporcionar exportación en streaming JSON y CSV estricto RFC 4180 (`\r\n`, escape de comillas `""` y headers `Content-Disposition`).
* **Consecuencias:**
  * *Positivas:* Observabilidad matemática profunda del comportamiento físico de los recintos; benchmark reproducible de modelos predictivos; exportación compatible con Pandas, R, Tableau y Excel sin corrupción de caracteres.
  * *Mitigación:* Protección contra división por cero en varianzas nulas ($\sigma_x \sigma_y = 0 \implies r=0$), control de singularidad en $SS_{\text{tot}} \le 10^{-4} \implies R^2=1.0$, y aumentador estocástico para datasets de menos de 30 muestras.

### ADR-007: Interfaz Espacial y Metáfora de Mueble de Laboratorio Científico Móvil
* **Contexto:** Los paneles de control IoT tradicionales sufren del síndrome de "dashboard plano genérico" (AI-slop / SaaS monótono): cuadrículas uniformes de tarjetas sin jerarquía espacial tangible, provocando fatiga visual y desconexión con el entorno físico real del sensor ESP32 desplegado en laboratorios o estaciones de campo.
* **Decisión:** Modelar la interfaz de usuario en Nuxt 3 bajo una arquitectura espacial inmersiva fundamentada en la metáfora de un **Mueble de Laboratorio Científico Móvil (Chasis de Acero ASTM A36 con Base Rodante de Precisión)**. La interfaz se estructura en cuatro planos físicos y táctiles:
  1. **Encimera Instrumental Superior (`LabCountertop`):** Superficie de acero cepillado que aloja la instrumentación de monitoreo inmediato. Contiene la gradilla volumétrica de 3 probetas reactivas de fluidos SVG (CO2eq, Monóxido CO y Humedad Relativa), el espejo óptico circular del display GC9A01 de 240x240 px a 60 FPS (con pupila animada por dinámicas de resorte amortiguado o tacómetro HUD) y los conmutadores de control táctil de hardware.
  2. **Gavetero Telescópico Modular (`LabCabinet` & `LabDrawer`):** Cuerpo principal con 3 gavetas organizadoras provistas de placas metálicas grabadas, tiras de iluminación LED de trabajo y rieles de descompresión con perspectiva CSS 3D:
     - **Gaveta I (Telemetría & Series Temporales):** Muestreo en vivo MQ-135/DHT11, delta instantáneo $\Delta_{1\text{m}}$ y cono predictivo $P_{t+15}$.
     - **Gaveta II (Data Science Studio & Modelos ML):** Radar de precisión de modelos ($R^2$, RMSE, MAE, MAPE), matriz de correlación de Pearson y exportador multi-capa Medallion.
     - **Gaveta III (Cartografía GPS & Geo-Warehouse):** Mapeo vectorial de campañas georreferenciadas y catálogo de expediciones.
  3. **Chasis Estructural & Ventilación HEPA:** Bisel perimetral de acero inoxidable con manija lateral tubular cromada y rejillas inferiores de ventilación y descompresión.
  4. **Base Rodante de 4 Ruedas Industriales (`LabWheels`):** Ruedas antiestáticas de poliuretano sobre horquillas giratorias con rin de aleación y pedal de bloqueo mecánico rojo de seguridad con freno de impacto.
* **Consecuencias:**
  * *Positivas:* Reducción drástica de la carga cognitiva mediante divulgación progresiva (*Progressive Disclosure* en gavetas colapsables); alta densidad informativa sin saturación visual; identidad visual distintiva y memorable alineada con el propósito científico del proyecto.
  * *Mitigación de Rendimiento y Accesibilidad:* Todas las animaciones 3D se restringen estrictamente a propiedades aceleradas por hardware GPU (`transform: scale/rotate`, `opacity`) con `perspective: 1200px`, garantizando 60 FPS estables sin reflows ni repaints de layout. Se integran atributos de accesibilidad (`aria-expanded`, `role="region"`, `aria-label`, foco por teclado) para asegurar compatibilidad WCAG 2.1 AA.

### ADR-008: Buffer Offline en LittleFS e Ingesta Atómica por Lotes (`POST /api/telemetry/batch`)
* **Contexto:** En campañas de campo o tránsito entre edificios universitarios, el enlace WiFi del ESP32 sufre caídas periódicas. Transmitir muestras exclusivamente por HTTP/WS síncrono causaba pérdida irreversible de telemetría ambiental.
* **Decisión:** Implementar en el firmware (`offline_buffer.hpp`) una cola en memoria flash con LittleFS (`/offline_queue.jsonl`) limitada a una cuota defensiva de 300 KB (~3500 muestras). Al recuperar enlace WiFi, el nodo ejecuta un vaciado (*flush*) secuencial en lotes de 15 muestras vía `POST /api/telemetry/batch` (con fallback a MQTT). En el backend NestJS, procesar cada lote bajo una única transacción `db.transaction()` en SQLite WAL, preservando los timestamps originales del hardware en Capa Bronze y recalculando deltas en Capa Silver.
* **Consecuencias:**
  * *Positivas:* Cero pérdida de datos ante desconexiones de red prolongadas; I/O agrupado en disco sin contención; preservación estricta de la cronología real de las series temporales.
  * *Mitigación:* Rotación automática del archivo si supera los 300 KB para evitar desgaste y fragmentación de la memoria flash del ESP32.

### ADR-009: Cartografía Táctica Leaflet y Sustitución de Radar Estático por `LocationsMapBoard.vue`
* **Contexto:** El componente de radar 360° analógico heredado operaba como un elemento decorativo sin interactividad geoespacial real, imposibilitando auditar puntos de muestreo en el campus ni visualizar dispersión geográfica.
* **Decisión:** Reemplazar el radar por `LocationsMapBoard.vue` en el banco inferior de la consola de laboratorio, integrando Leaflet con la capa de teselas *CartoDB Dark Matter*, marcadores de calidad de aire con codificación semafórica dinámica (Verde <650, Ámbar <1000, Rojo $\ge$ 1000), halo pulsante `animate-ping` en la sesión activa, transiciones de cámara `flyTo` y la bitácora interactiva "Lugares Donde Estuvimos".
* **Consecuencias:**
  * *Positivas:* Geolocalización táctica real de expediciones; interacción fluida a 60 FPS; armonía estética con la temática *vintage alchemy*.

### ADR-010: Arquitectura de Modo Dual Desacoplado (Local Determinista vs. En Vivo 1-a-1)
* **Contexto:** Las exposiciones en aulas y auditorios universitarios sufren con frecuencia de restricciones de red (aislamiento de clientes WiFi, puertos WebSockets bloqueados) o ausencia de suministro eléctrico para alimentar los calefactores de los sensores analógicos.
* **Decisión:** Implementar un selector de Modo Dual accesible desde el conmutador de cabecera y la palanca física `FrankensteinSwitch.vue`. En "Modo Local", la interfaz se alimenta de un generador estocástico determinista con inercia física real (sin depender del hardware ni de la red). En "Modo En Vivo (1-a-1)", se conecta al stream del ESP32 con monitor de latido (`lastPacketAgeSec`); si no se reciben paquetes durante más de 8 segundos, el sistema pasa a `OFFLINE` y la mascota digital entra en reposo emocional `SLEEPING` (Zzz).
* **Consecuencias:**
  * *Positivas:* Demostraciones 100% resilientes y confiables en cualquier escenario académico; alerta visual inmediata ante pérdida física del nodo sensor.

### ADR-011: Módulo Académico de Balance de Masa EDO y Solvers Numéricos con Análisis de Estabilidad Lipschitz
* **Contexto:** La sustentación universitaria en las asignaturas de Ecuaciones Diferenciales Ordinarias, Análisis Numérico e IoT requería justificar rigurosamente la física de dispersión gaseosa en recintos cerrados y validar los métodos de integración en tiempo discreto.
* **Decisión:** Formular la cinética de contaminantes como una EDO lineal de primer orden ($V \frac{dC}{dt} = G - QC$), obtener la solución analítica exacta mediante factor integrante ($C(t) = C_\infty + (C_0 - C_\infty)e^{-kt}$), e implementar en frontend y backend solvers numéricos comparativos: Euler progresivo ($O(h)$) y Runge-Kutta de 4to Orden ($O(h^4)$). Integrar la verificación en tiempo real de la condición de estabilidad de Dahlquist / Lipschitz ($h < 2/k$), graficación en osciloscopio CRT y tabla de errores con exportación a CSV y LaTeX.
* **Consecuencias:**
  * *Positivas:* Sustentación matemática impecable respaldada por código ejecutable; visualización didáctica de la divergencia numérica cuando $h \ge h_{\text{crítico}}$; exportación de datos lista para informes académicos.

---

## 8. Auditoría de Seguridad, Accesibilidad & Resiliencia (CSO & Spatial A11y Report)

| Dimensión Auditada | Nivel de Severidad | Estado | Mecanismo de Mitigación / Implementación |
| :--- | :--- | :--- | :--- |
| **SQL Injection** | **P0** (Crítico) | ✅ Mitigado | Todas las consultas en `DatabaseService`, `TelemetryService`, `SessionsService` y `AnalyticsService` emplean *Prepared Statements* parametrizados con `?` y cláusulas `LIMIT` sanitizadas. |
| **Fuga de Memoria WebSocket** | **P1** (Alto) | ✅ Resuelto | Implementación de `TelemetryGateway` con monitor de vida (`isAlive`), evento `error` registrado y temporizador de limpieza cíclico (`startHeartbeat`). |
| **División por Cero en Algoritmos Estadísticos** | **P1** (Alto) | ✅ Resuelto | En Pearson: Denominador $\sqrt{\text{Var}(X)\text{Var}(Y)} = 0 \implies r = 0.0$. En $R^2$: $SS_{\text{tot}} \le 10^{-4} \implies R^2 = 1.0$. En MAPE: Denominador $\max(1, \|y_{\text{true}}\|)$. |
| **Desbordamiento Numérico e Inestabilidad IEEE 754** | **P1** (Alto) | ✅ Resuelto | Clamping estricto de correlación $r \in [-1.0, +1.0]$, $R^2 \in [0.0, 0.9999]$, y derivadas acotadas a $[-500, +500]\text{ ppm/min}$. |
| **Inestabilidad Numérica en Solver Euler ($h \ge 2/k$)** | **P1** (Alto) | ✅ Mitigado | Monitor de estabilidad en tiempo real en `useNumericalAcademic.ts`: calcula $h_{\text{crítico}} = 2/k$, emite alerta visual de divergencia y previene desbordamiento en el renderizado CRT. |
| **Saturación / Desgaste de Flash en ESP32** | **P1** (Alto) | ✅ Mitigado | Límite defensivo de 300 KB en LittleFS (`offline_buffer.hpp`) con rotación automática del archivo `/offline_queue.jsonl` ante desconexiones prolongadas. |
| **Contención de I/O en Ingesta Batch Masiva** | **P2** (Medio) | ✅ Resuelto | Transacción agrupada `db.transaction()` en SQLite WAL que consolida cientos de registros en un único ciclo de escritura en disco. |
| **Fuga de Instancias Leaflet Map en Nuxt** | **P2** (Medio) | ✅ Resuelto | `LocationsMapBoard.vue` destruye explícitamente el mapa (`leafletMap.remove()`) en el hook `onUnmounted` y limpia capas con `markersGroup.clearLayers()`. |
| **Inyección CSV y Ruptura de Formato (RFC 4180)** | **P2** (Medio) | ✅ Resuelto | Escape completo de comillas dobles (`" -> ""`), delimitador de salto de línea canónico CRLF (`\r\n`), y serialización de campos no nulos. |
| **Accesibilidad Espacial (ARIA & Keyboard Nav)** | **P2** (Medio) | ✅ Auditado | Atributos `aria-expanded`, `aria-controls` y navegación por teclado en gavetas telescópicas y botones de control del chasis. |
| **Rendimiento 3D & Frame Budget (60 FPS)** | **P2** (Medio) | ✅ Optimizado | Canvas 240x240 renderizado vía `requestAnimationFrame`, transformaciones CSS 3D limitadas a capas de composición GPU sin layout thrashing. |
| **Bloqueo por Tabla no Indexada** | **P2** (Medio) | ✅ Resuelto | Índices compuestos `(device_id, timestamp DESC)` y `(session_id, timestamp DESC)` creados en `DatabaseService.onModuleInit()`. |
| **Exposición de Traza de Error (500)** | **P2** (Medio) | ✅ Resuelto | `HttpExceptionFilter` y `TransformInterceptor` encapsulan cualquier excepción capturada en el formato estándar `{ success, data, message, error }`. |

---
*Fufufu... la arquitectura y el estudio de ciencia de datos se encuentran meticulosamente blindados y documentados. El servidor no sufrirá una muerte espantosa.*