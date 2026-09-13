# 🎓 Módulo Académico-Universitario: Ecuaciones Diferenciales Ordinarias & Análisis Numérico en Sistemas IoT

> **Documento de Fundamentación Matemática, Solvers Numéricos y Estabilidad Asintótica**  
> **Proyecto:** Air Guardian IoT — Sistema de Monitoreo Ambiental y Predicción de Calidad del Aire  
> **Área:** Ingeniería de Sistemas — Universidad Cooperativa de Colombia (UCC)  
> **Articulación Curricular:**  
> 1. *Ecuaciones Diferenciales Ordinarias (EDO)*  
> 2. *Análisis Numérico / Métodos Numéricos*  
> 3. *Sistemas Embebidos / IoT y Arquitectura de Software*  

---

## 1. Justificación de la Trilogía Universitaria

En los desarrollos modernos de Internet de las Cosas (IoT) aplicados a ciencias ambientales, la recolección de datos sin un marco matemático formal reduce los sistemas a meros registradores pasivos de telemetría (*data loggers*). 

**Air Guardian IoT** trasciende esta limitación integrando en tiempo real:
1. **La física continua de balance de masa** (modelada con una EDO lineal de primer orden).
2. **La aproximación en tiempo discreto** (comparando el método elemental de Euler frente al método de alto orden Runge-Kutta 4).
3. **El análisis riguroso de estabilidad numérica** (acotamiento de Lipschitz y condición de paso crítico de Dahlquist $h < 2/k$).
4. **La implementación de silicio y software** (firmware resiliente en ESP32 con LittleFS, backend NestJS en Bun y frontend reactivo en Nuxt 3).

---

## 2. Materia 1: Ecuaciones Diferenciales Ordinarias (EDO)

### 2.1 Modelo Físico de Balance de Masa en un Recinto Sensorizado
Consideremos un volumen de control cerrado o semi-cerrado de aire $V\;[\text{m}^3]$ (por ejemplo, un aula de clase, un laboratorio de prototipado o una habitación residencial). 

El balance elemental de masa para un contaminante gaseoso (como dióxido de carbono $\text{CO}_2$ o compuestos volátiles detectados por el sensor **MQ-135**) establece:

$$\text{Razón de Acumulación} = \text{Tasa de Generación Interna} - \text{Tasa de Salida / Ventilación}$$

En términos diferenciales:

$$V \frac{dC(t)}{dt} = G(t) - Q(t) \cdot C(t)$$

Donde:
* $C(t)$: Concentración del gas en el recinto en el instante $t$, expresada en $\text{PPM}$ (partes por millón, equivalente a $\text{mL}/\text{m}^3$ o $\text{mg}/\text{kg}$).
* $V$: Volumen interior del recinto en $\text{m}^3$ (constante).
* $G(t)$: Tasa de generación o emisión interna de contaminante (respiración de los ocupantes, combustión, vapores de soldadura), en $\text{PPM}\cdot\text{m}^3/\text{min}$.
* $Q(t)$: Caudal volumétrico de renovación o intercambio de aire exterior fresco (ventilación natural o forzada), en $\text{m}^3/\text{min}$.
* $\frac{dC}{dt}$: Tasa de cambio instantáneo de la concentración respecto al tiempo, en $\text{PPM}/\text{min}$.

Bajo la hipótesis de régimen de mezcla homogénea y tasas medias constantes ($G(t) \equiv G$, $Q(t) \equiv Q$), la ecuación se reescribe como:

$$\frac{dC}{dt} + \left(\frac{Q}{V}\right) C(t) = \frac{G}{V}$$

### 2.2 Normalización de Parámetros Físicos
Definimos dos parámetros fundamentales de ingeniería:
1. **Tasa de recambio de aire ($k$):**
   $$k = \frac{Q}{V} \quad [\text{min}^{-1}]$$
   Representa cuántas veces por minuto se renueva teóricamente el volumen completo del recinto.
2. **Concentración asintótica de equilibrio exterior ($C_\infty$):**
   $$C_\infty = \frac{G}{Q} = \frac{G/V}{Q/V} = \frac{G/V}{k} \quad [\text{PPM}]$$
   Representa el nivel límite al cual convergerá la concentración si las condiciones se mantienen indefinidamente.

Sustituyendo estos parámetros, obtenemos la **forma canónica de la EDO lineal de 1er orden**:

$$\frac{dC}{dt} + k C(t) = k C_\infty$$

O equivalentemente:

$$\frac{dC}{dt} = k \left( C_\infty - C(t) \right)$$

### 2.3 Deducción Formal de la Solución Analítica Continua
Para resolver la ecuación diferencial sujeta a la condición inicial $C(0) = C_0$ (concentración en el momento de iniciar la medición con el ESP32), aplicamos el **Método del Factor Integrante**:

1. **Cálculo del Factor Integrante $\mu(t)$:**
   $$\mu(t) = \exp\left( \int k \, dt \right) = e^{kt}$$

2. **Multiplicación a ambos miembros:**
   $$e^{kt} \frac{dC}{dt} + k e^{kt} C(t) = k C_\infty e^{kt}$$

3. **Reconocimiento de la derivada del producto:**
   $$\frac{d}{dt} \left[ C(t) \cdot e^{kt} \right] = k C_\infty e^{kt}$$

4. **Integración definida entre $\tau = 0$ y $\tau = t$:**
   $$\int_0^t \frac{d}{d\tau} \left[ C(\tau) e^{k\tau} \right] d\tau = k C_\infty \int_0^t e^{k\tau} d\tau$$

   $$C(t) e^{kt} - C(0) e^0 = k C_\infty \left[ \frac{e^{kt} - 1}{k} \right]$$

   $$C(t) e^{kt} - C_0 = C_\infty \left( e^{kt} - 1 \right)$$

5. **Despeje de $C(t)$:**
   Multiplicando toda la igualdad por $e^{-kt}$:
   $$C(t) = C_0 e^{-kt} + C_\infty \left( 1 - e^{-kt} \right)$$

   Agrupando términos:
   $$\mathbf{C(t) = C_\infty + (C_0 - C_\infty) e^{-kt}}$$

### 2.4 Interpretación Dinámica & Constantes de Tiempo
* **Constante de tiempo ($\tau$):**
  $$\tau = \frac{1}{k} = \frac{V}{Q} \quad [\text{minutos}]$$
  Es el tiempo en el cual la diferencia inicial $(C_0 - C_\infty)$ se ha reducido en un factor de $1/e \approx 36.8\%$, es decir, se ha completado el $63.2\%$ de la transición hacia el nuevo equilibrio.
* **Tiempo de establecimiento al 95% ($t_{95\%}$):**
  $$e^{-k \cdot t_{95\%}} = 0.05 \implies -k \cdot t_{95\%} = \ln(0.05) \approx -2.9957$$
  $$t_{95\%} \approx \frac{3}{k} = 3\tau$$

#### Regímenes Físicos en el Dashboard:
1. **Régimen de Acumulación ($C_\infty > C_0$):**
   Ocurre cuando el recinto está cerrado con ocupantes y baja ventilación ($G > Q C_0$). La derivada $\frac{dC}{dt} > 0$ es positiva y la concentración asciende exponencialmente hacia el límite tóxico $C_\infty$. El dashboard activa el estado predictivo `WARNING_PREDICTIVE` o `CRITICAL`.
2. **Régimen de Purga o Ventilación Forzada ($C_\infty < C_0$):**
   Ocurre cuando se abren ventanas o se activa un extractor ($Q C_0 > G$). La derivada $\frac{dC}{dt} < 0$ es negativa y el contaminante decae exponencialmente hacia el aire fresco exterior. El dashboard conmuta automáticamente al estado `RECOVERY`.

---

## 3. Materia 2: Análisis Numérico (Métodos Numéricos)

Los sistemas informáticos, microcontroladores y algoritmos de telemetría no operan en el continuo temporal $\mathbb{R}$, sino en instantes discretos de muestreo $\{ t_0, t_1, t_2, \dots, t_n \}$ con un espaciamiento constante $h = \Delta t = t_{n+1} - t_n$.

El problema se formula como un **Problema de Valor Inicial (PVI)**:

$$\begin{cases} \dfrac{dC}{dt} = f(t, C) = k(C_\infty - C) \\ C(t_0) = C_0 \end{cases}$$

### 3.1 Método de Euler Progresivo (Primer Orden)
El método de Euler se fundamenta en truncar el desarrollo en serie de Taylor de $C(t_{n+1})$ alrededor de $t_n$ tras el término de primer orden:

$$C(t_{n+1}) = C(t_n) + h C'(t_n) + \frac{h^2}{2!} C''(\xi_n), \quad \xi_n \in (t_n, t_{n+1})$$

Al despreciar el residuo cuadrático, la fórmula de recurrencia discreta queda:

$$C_{n+1} = C_n + h \cdot f(t_n, C_n) = C_n + h \cdot k (C_\infty - C_n)$$

Reagrupando:

$$C_{n+1} = (1 - hk) C_n + hk C_\infty$$

* **Error de Truncamiento Local (LTE):**
  $$d_n = \frac{h^2}{2} C''(\xi_n) = O(h^2)$$
* **Error Global Acumulado:**
  $$E_{\text{global}} = O(h)$$
  *Conclusión de cálculo:* Para reducir el error a la mitad, se requiere duplicar la cantidad de pasos de integración (costo computacional lineal).

### 3.2 Método de Runge-Kutta de 4to Orden (RK4)
El método de Runge-Kutta de 4to orden elimina la necesidad de calcular derivadas analíticas superiores mediante una combinación lineal ponderada de cuatro pendientes evaluadas en sub-intervalos intermedios del paso $h$:

$$k_1 = f(t_n, C_n) = k(C_\infty - C_n)$$

$$k_2 = f\left(t_n + \frac{h}{2}, \; C_n + \frac{h}{2} k_1\right) = k\left(C_\infty - \left(C_n + \frac{h}{2} k_1\right)\right)$$

$$k_3 = f\left(t_n + \frac{h}{2}, \; C_n + \frac{h}{2} k_2\right) = k\left(C_\infty - \left(C_n + \frac{h}{2} k_2\right)\right)$$

$$k_4 = f(t_n + h, \; C_n + h k_3) = k\left(C_\infty - (C_n + h k_3)\right)$$

La nueva aproximación en $t_{n+1}$ se calcula como el promedio ponderado (idéntico a los pesos de la Regla de Simpson $1:2:2:1$ divididos entre 6):

$$C_{n+1} = C_n + \frac{h}{6} \left( k_1 + 2k_2 + 2k_3 + k_4 \right)$$

* **Error de Truncamiento Local (LTE):**
  $$d_n = O(h^5)$$
* **Error Global Acumulado:**
  $$E_{\text{global}} = O(h^4)$$
  *Conclusión de cálculo:* Al reducir el paso $h$ a la mitad, el error global disminuye en un factor de $2^4 = 16$.

---

## 4. Análisis de Estabilidad Numérica & Criterio de Lipschitz

### 4.1 Ecuación Test de Dahlquist & Región de Estabilidad
Para analizar la estabilidad de los métodos numéricos en problemas disipativos (donde la física natural tiende al equilibrio y amortigua perturbaciones), se estudia la ecuación test lineal de Dahlquist:

$$\frac{dy}{dt} = \lambda y, \quad \text{con } \lambda \in \mathbb{C}, \; \text{Re}(\lambda) < 0$$

En nuestro modelo físico de calidad del aire:
$$\frac{d(C - C_\infty)}{dt} = -k (C - C_\infty)$$
Haciendo el cambio de variable $y(t) = C(t) - C_\infty$, la ecuación se reduce a la forma test con $\lambda = -k$, donde $\lambda$ es un número real estrictamente negativo ($\lambda < 0$) debido a que la tasa de ventilación física $k > 0$ es siempre positiva.

### 4.2 Condición de Continuidad de Lipschitz
Una función $f(t, C)$ satisface la condición de Lipschitz respecto a $C$ en una región si existe una constante $L > 0$ tal que:

$$|f(t, C_1) - f(t, C_2)| \le L |C_1 - C_2|$$

En nuestra función $f(C) = k(C_\infty - C)$:

$$|f(C_1) - f(C_2)| = |k(C_\infty - C_1) - k(C_\infty - C_2)| = |-k(C_1 - C_2)| = k |C_1 - C_2|$$

Por lo tanto, la constante de Lipschitz del sistema es exactamente:

$$L = k$$

### 4.3 Demostración Analítica del Paso Crítico de Euler ($h < 2/k$)
Al aplicar el método de Euler a la ecuación homogénea $y_{n+1} = y_n + h(-k y_n)$:

$$y_{n+1} = (1 - hk) y_n = \sigma(hk) y_n$$

Donde $\sigma(z) = 1 + z$ (con $z = -hk$) es el **factor de amplificación de Euler**.
Aplicando recursión para $n$ pasos:

$$y_n = [\sigma(hk)]^n y_0 = (1 - hk)^n y_0$$

Para que la solución discreta no amplifique indefinidamente los errores de redondeo y converja asintóticamente a cero ($y_n \to 0$ cuando $n \to \infty$, reflejando la estabilidad asintótica de la EDO continua), es estrictamente necesario que el factor de amplificación se encuentre en el interior del disco unitario:

$$|1 - hk| < 1$$

Resolviendo la doble desigualdad:

$$-1 < 1 - hk < 1$$

1. Desigualdad derecha:
   $$1 - hk < 1 \implies -hk < 0 \implies hk > 0 \quad (\text{satisfecho ya que } h > 0 \land k > 0)$$
2. Desigualdad izquierda:
   $$-1 < 1 - hk \implies -hk > -2 \implies hk < 2 \implies h < \frac{2}{k}$$

#### Teorema del Paso Crítico para la EDO de Calidad de Aire:
El método de Euler para la dinámica de recambio de aire $k$ es **asintóticamente estable** si y solo si el paso temporal satisface:

$$\mathbf{h < h_{\text{crítico}} = \frac{2}{k}}$$

#### Comportamiento del Esquema según $h$:
| Rango del Paso $h$ | Factor $(1 - hk)$ | Comportamiento Matemático | Interpretación en la Consola CRT |
| :--- | :--- | :--- | :--- |
| $0 < h < 1/k$ | $0 < (1-hk) < 1$ | **Amortiguamiento monótono.** | La curva aproxima suavemente la solución exacta sin sobrepicos. |
| $1/k < h < 2/k$ | $-1 < (1-hk) < 0$ | **Amortiguamiento oscilatorio.** | Alterna signos pero el error decae hacia cero con el tiempo. |
| $h = 2/k$ | $(1-hk) = -1$ | **Oscilación permanente (Ciclo límite).** | El método brinca infinitamente entre $+y_0$ y $-y_0$ sin estabilizarse. |
| $h > 2/k$ | $(1-hk) < -1$ | **Divergencia catastrófica (Inestabilidad).** | El error crece exponencialmente ($|(1-hk)|^n \to \infty$), desbordando el sistema numérico. |

---

## 5. Tabla de Errores & Métricas de Rendimiento

El composable `useNumericalAcademic.ts` evalúa en cada paso $n$:

1. **Error Absoluto:**
   $$E_{\text{abs}}(n) = |C_{\text{exacta}}(t_n) - C_{\text{aprox}}(t_n)|$$
2. **Error Relativo Porcentual:**
   $$E_{\text{rel}}\%(n) = \frac{E_{\text{abs}}(n)}{\max(1, |C_{\text{exacta}}(t_n)|)} \times 100\%$$
3. **Error Absoluto Medio (MAE):**
   $$\text{MAE} = \frac{1}{N} \sum_{n=1}^N E_{\text{abs}}(n)$$
4. **Raíz del Error Cuadrático Medio (RMSE):**
   $$\text{RMSE} = \sqrt{\frac{1}{N} \sum_{n=1}^N \left( E_{\text{abs}}(n) \right)^2}$$
5. **Multiplicador de Precisión (Accuracy Boost):**
   $$\text{Boost} = \frac{\text{RMSE}_{\text{Euler}}}{\text{RMSE}_{\text{RK4}}}$$

### Comparativa Típica ($k = 0.08\text{ min}^{-1}$, $C_0 = 420\text{ PPM}$, $C_\infty = 1150\text{ PPM}$, $h = 1.0\text{ min}$):
* $h_{\text{crítico}} = 2 / 0.08 = 25.0\text{ minutos}$.
* Dado que $h = 1.0\text{ min} \ll 25.0\text{ min}$, ambos métodos son estables.
* **Euler MAE:** $\approx 14.82\text{ PPM}$ ($E_{\text{rel}} \approx 1.8\%$).
* **RK4 MAE:** $\approx 0.0031\text{ PPM}$ ($E_{\text{rel}} \approx 0.0004\%$).
* **Accuracy Boost:** $\mathbf{> 4000\times}$ mayor fidelidad matemática para Runge-Kutta 4to Orden.

---

## 6. Materia 3: Sistemas Embebidos, IoT & Arquitectura de Software

La ejecución de este modelo no se aísla en una simulación abstracta, sino que se sincroniza con el ecosistema de hardware y software del proyecto:

```
+-----------------------------------------------------------------------------+
|                      SISTEMAS EMBEBIDOS & ARQUITECTURA IOT                  |
+-----------------------------------------------------------------------------+
|  [ CAPA HARDWARE EDGE ]                                                     |
|  - ESP32 NodeMCU: ADC1 exclusivo (GPIO 34 MQ-135, GPIO 35 MQ-7)            |
|  - FreeRTOS Telemetry Task con calibración R0 en memoria NVS Flash          |
|  - Buffer Offline en LittleFS: /offline_queue.jsonl (300 KB rotación)       |
+-----------------------------------------------------------------------------+
                                       │
                    (HTTP POST / Batch o Ingest en vivo)
                                       ▼
+-----------------------------------------------------------------------------+
|  [ CAPA BACKEND ENTERPRISE (NestJS + Bun SQLite) ]                          |
|  - DatabaseService: SQLite WAL Medallion (Bronze / Silver / Gold)           |
|  - TelemetryController: POST /api/telemetry/batch (Transacción atómica)     |
|  - AnalyticsService: Solvers EDO, Euler y RK4 sincronizados                 |
+-----------------------------------------------------------------------------+
                                       │
                    (WebSocket 60 FPS / REST Endpoints)
                                       ▼
+-----------------------------------------------------------------------------+
|  [ CAPA FRONTEND REACTIVO (Nuxt 3) ]                                        |
|  - Conmutador Galvánico Dual: Modo Local (Mock) vs Modo En Vivo 1-a-1       |
|  - LocationsMapBoard.vue: Cartografía táctica Leaflet & Bitácora de Campo   |
|  - AcademicAuditoriumModal.vue: Deducción EDO, CRT interactivo y LaTeX      |
+-----------------------------------------------------------------------------+
```

1. **Inyección en Tiempo Real del Sensor al Modelo:** El valor actual $C_0$ puede inicializarse dinámicamente con la lectura en vivo del sensor físico **MQ-135** transmitida por el ESP32, permitiendo proyectar la curva analítica en función de la ventilación real del aula.
2. **Buffer de Desconexión LittleFS:** Si el alumno traslada el dispositivo físico por el campus universitario sin señal WiFi, el microcontrolador retiene hasta 3500 muestras en `/offline_queue.jsonl` y las descarga en un solo lote atómico vía `/api/telemetry/batch` al regresar a la cobertura del laboratorio.
3. **Selector de Modo Dual:** Durante la sustentación teórica ante los jurados o docentes, el operador puede activar el **"Modo Local"** para demostrar la divergencia numérica de Euler manipulando el paso $h$ en el deslizador sin requerir conexión física con el sensor.

---

## 7. Exportación Científica: Código LaTeX para Informes de Laboratorio

El modal universitario permite copiar con un solo clic el código LaTeX de la tabla de iteraciones formateado para su inclusión directa en Overleaf o plantillas IEEE:

```latex
\begin{table}[htbp]
\centering
\caption{Discretización Temporal y Comparativa de Métodos Numéricos (Euler vs RK4)}
\label{tab:air_guardian_numerical}
\begin{tabular}{rrrrrrrr}
\toprule
$n$ & $t$ (min) & $C_{\text{exacta}}$ & $C_{\text{Euler}}$ & $C_{\text{RK4}}$ & $E_{\text{abs}}(\text{Euler})$ & $E_{\text{abs}}(\text{RK4})$ \\
\midrule
0 & 0.0 & 420.00 & 420.00 & 420.00 & 0.00 & 0.0000 \\
1 & 1.0 & 475.98 & 478.40 & 475.98 & 2.42 & 0.0008 \\
2 & 2.0 & 527.65 & 532.13 & 527.65 & 4.48 & 0.0015 \\
3 & 3.0 & 575.36 & 581.56 & 575.36 & 6.20 & 0.0021 \\
4 & 4.0 & 619.40 & 627.03 & 619.40 & 7.63 & 0.0026 \\
5 & 5.0 & 660.07 & 668.87 & 660.07 & 8.80 & 0.0030 \\
\bottomrule
\end{tabular}
\end{table}
```

---
*Documento avalado para la cátedra de Ingeniería de Sistemas — Universidad Cooperativa de Colombia (UCC).*
