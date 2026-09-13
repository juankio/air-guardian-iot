# 🦾 Manual de Fabricación 3D y Montaje Mecánico: Air Guardian IoT
**Arquitectura de Carcasa:** Franky (Master Shipwright)  
**Versión de Hardware:** 1.0.0-PRO  
**Tecnología:** Impresión 3D FDM (Fused Deposition Modeling) / OpenSCAD Paramétrico  
**Fecha de Certificación:** Septiembre 2026  

---

## 🎯 1. Filosofía de Diseño y Tolerancias Críticas

Para evitar el desperdicio de filamento y garantizar que todos los módulos electrónicos encajen al primer intento sin rebabas ni holguras indeseadas:
* **Tolerancia de Interferencia/Holgura FDM:** $+0.30\,\text{mm}$ diametral/lineal en todas las ranuras y alojamientos.
* **Espesor de Pared Estructural:** $2.80\,\text{mm}$ (sólido y robusto, equivalente a 7 perímetros de boquilla de $0.4\,\text{mm}$).
* **Unión Frontal-Trasera:** Labio machihembrado perimetral (*Lip & Groove*) de $1.5\,\text{mm}$ de altura con $0.25\,\text{mm}$ de tolerancia perimetral para sellado hermético al polvo y alineación perfecta de bordes.
* **Zero-Support Geometry:** Ambas piezas fueron diseñadas para imprimirse **sin necesidad de soportes complejos** (voladizos $\le 45^\circ$, ranuras con puentes limpios).

---

## 📐 2. Cotas y Dimensiones Exactas de Sensores

| Módulo / Componente | Cota Crítica Nominal | Cota en Modelo 3D (con Tolerancia) | Función Mecánica |
| :--- | :--- | :--- | :--- |
| **Pantalla GC9A01 (1.28" SPI)** | $\varnothing 35.5\,\text{mm}$ (Cuerpo) | Bisel visible: $\varnothing 33.0\,\text{mm}$<br>Rebaje PCB: $\varnothing 36.4\,\text{mm} \times 2.6\,\text{mm}$ | El bisel retiene el cristal hacia el frente. El rebaje trasero aloja el PCB sin presionar el panel LCD. Canal de $18 \times 6\,\text{mm}$ para cables SPI. |
| **Sensores MQ-135 y MQ-7** | Cúpula: $\varnothing 19.0 - 19.4\,\text{mm}$<br>PCB: $32.5 \times 20.5\,\text{mm}$ | Orificio cúpula: $\varnothing 19.8\,\text{mm}$<br>Rieles guía PCB: $33.0 \times 21.0\,\text{mm}$ | La malla metálica sobresale al exterior. Rieles laterales sostienen el PCB sin tornillos adicionales. |
| **Sensor DHT11 (KY-015)** | $15.5 \times 27.0 \times 12.0\,\text{mm}$ | Cavidad: $16.2 \times 27.5 \times 12.0\,\text{mm}$ | Cámara aislada térmicamente con 5 branquias frontales y 3 laterales directas al aire ambiente. |
| **MPU-6050 (GY-521)** | Taladros M3 a $16.5\,\text{mm}$ | 2 Postes $\varnothing 6.2\,\text{mm}$ con núcleo $\varnothing 2.9\,\text{mm}$ (paso $16.5\,\text{mm}$) | Montaje 100% rígido directo al chasis frontal. Cero holgura para captar sismos, vibraciones e inclinación sin amortiguamiento elástico. |
| **ESP32 DevKit V1 (30/38p)** | $28.5 \times 51.5\,\text{mm}$ | Cuna: $29.2 \times 52.5\,\text{mm}$<br>Ventana USB: $12.5 \times 8.0\,\text{mm}$ | Rieles de encaje en la tapa trasera con topes y chaflán avellanado a $45^\circ$ en la toma USB para cables gruesos. |
| **Postes de Chasis (4 Esquinas)** | Tornillo M3 DIN 912 | Frontal: Núcleo $\varnothing 2.9\,\text{mm}$<br>Trasera: Pasante $\varnothing 3.4\,\text{mm}$ + cajeado $\varnothing 6.2 \times 3.5\,\text{mm}$ | Autorroscado directo sobre plástico o apto para insertos térmicos de latón M3 ($4.2\,\text{mm}$ OD). |

---

## 🔥 3. Blindaje Térmico y Convección (Chimney Effect)

> ⚠️ **REGLA DE ORO DE INGENIERÍA TÉRMICA:**  
> Los sensores químicos **MQ-135** y **MQ-7** integran filamentos calefactores que consumen cerca de $150-200\,\text{mA}$ a $5\,\text{V}$ cada uno ($\approx 1.5\,\text{W}$ de calor continuo).
> 
> * **Tabique Deflector Interno (*Thermal Baffle*):** Una pared sólida de $2.0\,\text{mm}$ divide la carcasa en dos cámaras verticales independientes. La cámara izquierda (MQ) queda completamente aislada de la cámara derecha (DHT11 y MPU-6050).
> * **Tiro Convectivo (Efecto Chimenea):** El aire frío entra por las branquias inferiores y laterales izquierdas, baña las cúpulas de los sensores MQ, y el aire caliente asciende y se evacúa al exterior mediante 4 toberas superiores en el techo del chasis.
> * **Lecturas Fiables:** El DHT11 mide la temperatura y humedad del aire ambiente exterior real sin sufrir la radiación calórica de los sensores de gas.

---

## 🖨️ 4. Parámetros de Laminación (Slicer / Cura / PrusaSlicer / OrcaSlicer)

### Orientación de Piezas en la Cama de Impresión
1. **`air_guardian_front.stl`:**
   * **Orientación:** Colocar con la cara frontal lisa (bisel circular) apoyada directamente contra la placa de impresión ($Z=0$).
   * **Soportes:** **DESACTIVADOS (None)**. Todos los orificios, rejillas y postes crecen verticalmente a $90^\circ$ o chaflán a $45^\circ$.
2. **`air_guardian_back.stl`:**
   * **Orientación:** Colocar con la cara trasera plana apoyada contra la placa de impresión.
   * **Soportes:** **DESACTIVADOS (None)**.

### Perfil de Impresión Recomendado

| Parámetro | Valor Óptimo (PLA+) | Valor Alternativo (PETG) |
| :--- | :--- | :--- |
| **Temperatura de Boquilla** | $205^\circ\text{C} - 215^\circ\text{C}$ | $230^\circ\text{C} - 240^\circ\text{C}$ |
| **Temperatura de Cama** | $60^\circ\text{C}$ (PEI Texturizada recomendada) | $75^\circ\text{C} - 80^\circ\text{C}$ |
| **Altura de Capa (Layer Height)** | $0.20\,\text{mm}$ (Primera capa $0.24\,\text{mm}$) | $0.20\,\text{mm}$ |
| **Perímetros de Pared (Walls)** | **4 perímetros** (mínimo $1.6\,\text{mm}$) | **4 perímetros** |
| **Capas Superiores / Inferiores** | 5 capas superiores / 4 capas inferiores | 5 superiores / 4 inferiores |
| **Densidad de Relleno (Infill)** | **$20\% - 25\%$** | **$20\% - 25\%$** |
| **Patrón de Relleno** | **Giroide (Gyroid)** o Cúbico | **Giroide (Gyroid)** |
| **Ventilador de Capa** | $100\%$ (a partir de la capa 3) | $40\% - 50\%$ |
| **Velocidad de Impresión** | $50 - 60\,\text{mm/s}$ (Paredes ext: $35\,\text{mm/s}$) | $45 - 55\,\text{mm/s}$ |

---

## 🔩 5. Lista de Materiales y Tornillería (BOM)

### Tornillería y Elementos de Unión
* **4x Tornillos M3 x 20mm** (Cabeza cilíndrica Allen DIN 912 o botón ISO 7380) para unir la tapa trasera con el frontal.
* **2x Tornillos M3 x 6mm** (o M3 x 5mm) para anclar el sensor inercial GY-521 a los postes rígidos del chasis.
* *(Opcional Pro)* **4x Insertos roscados de latón M3** ($\varnothing 4.2\,\text{mm}$ exterior $\times 4.0\,\text{mm}$ largo) para insertar con la punta de un soldador caliente en los postes del frontal si se desea desmontar recurrentemente. Si no se usan insertos, el orificio de $\varnothing 2.9\,\text{mm}$ autorrosca perfectamente con el propio tornillo M3 en PLA/PETG.

### Accesorios de Montaje
* **4x Almohadillas de silicona autoadhesivas** ($\varnothing 8.0\,\text{mm} - 8.5\,\text{mm} \times 1.5\,\text{mm}$) para las cunas de las patas de sobremesa en la tapa trasera.
* **1x Trozo pequeño de cinta doble cara ultrafina** ($1\,\text{mm}$) o 2 gotas de adhesivo tipo B-7000 en el borde del rebaje para asegurar la pantalla GC9A01.
* **Cables jumper Dupont hembra-hembra** (longitud recomendada: $10\,\text{cm}$ a $15\,\text{cm}$) o cableado trenzado soldado. La carcasa cuenta con más de $18\,\text{mm}$ de holgura interna libre para ordenar los mazos sin estrés mecánico.

---

## 🛠️ 6. Secuencia de Ensamblaje Paso a Paso

1. **Instalación de la Pantalla GC9A01:**
   * Deslizar la pantalla circular en el alojamiento superior del chasis frontal. El cristal asienta firmemente contra el bisel frontal de $\varnothing 33\,\text{mm}$.
   * Pasar los 7 cables SPI a través del canal de paso superior ($18 \times 6\,\text{mm}$).
2. **Instalación de los Sensores de Gas (MQ-135 y MQ-7):**
   * Insertar las cúpulas metálicas a través de los orificios frontales de $\varnothing 19.8\,\text{mm}$. Los PCBs quedan encajados entre los rieles guía laterales.
3. **Instalación del Sensor DHT11:**
   * Colocar el DHT11 en la cámara fría derecha con su rejilla apuntando hacia las ranuras frontales y laterales.
4. **Instalación del MPU-6050:**
   * Atornillar la placa GY-521 a los dos postes rígidos usando dos tornillos M3x6mm. Apretar con firmeza para asegurar que no exista flexión relativa.
5. **Instalación del ESP32 DevKit V1:**
   * Deslizar el ESP32 en los rieles de la tapa trasera alineando el puerto Micro-USB/USB-C con la ventana avellanada inferior.
6. **Conexión de Mazos de Cables:**
   * Conectar los cables siguiendo la tabla de `GUIA_CONEXIONES_HARDWARE.md`. Comprobar que los cables pasen holgadamente sin tocar los elementos calientes de los MQ.
7. **Cierre de Carcasa:**
   * Acoplar la tapa trasera con el frontal guiándose por el labio machihembrado perimetral.
   * Atornillar los 4 tornillos M3x20mm en los orificios avellanados traseros.
   * Pegar las 4 almohadillas de silicona en las patas de escritorio.

¡Listo para navegar y monitorear la atmósfera como una nave de guerra acorazada! 🦾✨
