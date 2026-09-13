/* ==========================================================================
 *  AIR GUARDIAN IOT - REPOSITORIO DE HARDWARE 3D
 *  DISEÑO PARAMÉTRICO DE CARCASA DE ALTA INGENIERÍA
 *  Arquitecto Naval: Franky (Master Shipwright)
 *  Tecnología: OpenSCAD v2021.01+ / FDM 3D Printing Ready
 * ========================================================================== */

// Selección de renderizado (Frontal, Trasera, Ensamble o Vista Explosionada)
part = "assembly"; // [ "front", "back", "assembly", "exploded" ]

// Calidad de facetas de polígonos
$fn = 64;
$fn_fast = 32;
$eps = 0.05; // Pequeño offset para evitar caras coplanares en booleanos (CSG limpio)

/* ==========================================================================
 *  1. PARÁMETROS MECÁNICOS Y TOLERANCIAS FDM (+0.3mm Clearance)
 * ========================================================================== */
clearance             = 0.30; // Holgura diametral / perimetral de ajuste FDM
wall_t                = 2.80; // Espesor de pared principal
box_w                 = 114.0; // Ancho exterior chasis (eje X)
box_h                 = 124.0; // Alto exterior chasis (eje Y)
corner_r              = 8.00; // Radio de redondeo en esquinas
front_depth           = 22.0; // Profundidad de la pieza frontal
back_depth            = 18.0; // Profundidad de la pieza trasera (Total: 40mm)

// Pantalla Circular GC9A01 (1.28" TFT SPI 240x240)
gc9a01_body_dia       = 35.50; // Diámetro nominal del módulo
gc9a01_bezel_dia      = 33.00; // Diámetro visible de retención frontal
gc9a01_recess_dia     = 36.40; // Rebaje interior de asiento del PCB (+0.9mm)
gc9a01_recess_depth   = 2.60;  // Profundidad del rebaje para el PCB y cristal
gc9a01_cable_w        = 18.00; // Ancho canal de paso para cables SPI
gc9a01_cable_h        = 6.00;  // Alto canal de paso cables SPI
gc9a01_pos_y          = 28.00; // Posición Y del centro de la pantalla

// Sensores de Gas MQ-135 y MQ-7 (Resistencias calefactoras a 5V)
mq_cupola_dia         = 19.80; // Diámetro de paso de cúpula de malla de acero
mq_pcb_w              = 32.50; // Ancho nominal de PCB MQ
mq_pcb_h              = 20.50; // Alto nominal de PCB MQ
mq_pcb_t              = 1.60;  // Espesor estándar de PCB
mq_x                  = -27.0; // Posición X de los sensores MQ
mq135_y               = -14.0; // Posición Y del MQ-135 (eCO2 / Calidad Aire)
mq7_y                 = -42.0; // Posición Y del MQ-7 (Monóxido de Carbono CO)

// Sensor DHT11 / KY-015 (Temperatura y Humedad Aislado)
dht11_w               = 16.20; // Ancho cuerpo DHT11
dht11_h               = 27.50; // Alto cuerpo DHT11
dht11_d               = 12.00; // Profundidad cuerpo DHT11
dht11_x               = 28.00; // Posición X (Cámara aislada derecha)
dht11_y               = -30.0; // Posición Y del sensor ambiental

// Sensor Inercial MPU-6050 (GY-521 - Acelerómetro / Giróscopo I2C)
mpu_hole_pitch        = 16.50; // Distancia exacta entre centros de orificios M3
mpu_hole_dia          = 2.90;  // Núcleo para autorroscado M3 o inserto térmico
mpu_post_dia          = 6.20;  // Diámetro exterior del poste de fijación
mpu_post_h            = 7.00;  // Altura del poste rígido
mpu_x                 = 28.00; // Posición X MPU6050
mpu_y                 = -4.00; // Posición Y MPU6050

// ESP32 DevKit V1 (Microcontrolador Principal)
esp32_w               = 28.60; // Ancho PCB ESP32
esp32_h               = 52.00; // Largo PCB ESP32
esp32_usb_w           = 12.50; // Ancho ventana USB con holgura
esp32_usb_h           = 8.00;  // Alto ventana USB
esp32_usb_chamfer     = 2.20;  // Chaflán exterior para conectores anchos
esp32_x               = 0.00;  // Centrado en X
esp32_y               = -6.00; // Posición en Y en tapa trasera

// Postes de Esquinas y Tornillería M3
screw_core_dia        = 2.90;  // Orificio para roscar directo en plástico M3
screw_clear_dia       = 3.40;  // Orificio pasante libre para M3
screw_head_dia        = 6.20;  // Alojamiento de cabeza de tornillo cilíndrica M3
screw_head_depth      = 3.50;  // Profundidad de alojamiento de cabeza M3
post_outer_dia        = 7.80;  // Diámetro exterior de los postes
post_x                = (box_w/2) - 10.0; // Posición X simétrica de tornillos
post_y                = (box_h/2) - 10.0; // Posición Y simétrica de tornillos

// Labio Machihembrado de Sellado Perimetral (Lip & Groove)
lip_h                 = 1.50;  // Altura del resalte de unión
lip_t                 = 1.20;  // Grosor del resalte
lip_clearance         = 0.25;  // Holgura perimetral para encaje perfecto

/* ==========================================================================
 *  2. MÓDULOS GEOMÉTRICOS AUXILIARES
 * ========================================================================== */

// Perfil 2D redondeado
module rounded_rect_2d(w, h, r) {
    hull() {
        translate([-w/2 + r, -h/2 + r]) circle(r=r, $fn=$fn_fast);
        translate([ w/2 - r, -h/2 + r]) circle(r=r, $fn=$fn_fast);
        translate([ w/2 - r,  h/2 - r]) circle(r=r, $fn=$fn_fast);
        translate([-w/2 + r,  h/2 - r]) circle(r=r, $fn=$fn_fast);
    }
}

// Prisma 3D con esquinas redondeadas
module rounded_box(w, h, d, r) {
    linear_extrude(height=d)
        rounded_rect_2d(w, h, r);
}

// Poste de tornillo hembra (Frontal: orificio 2.9mm)
module corner_post_front(x, y, h) {
    translate([x, y, 0]) {
        difference() {
            cylinder(d=post_outer_dia, h=h, $fn=$fn_fast);
            translate([0, 0, wall_t])
                cylinder(d=screw_core_dia, h=h - wall_t + $eps, $fn=$fn_fast);
        }
    }
}

// Poste de tornillo pasante (Trasera: pasante 3.4mm + cajeado para cabeza)
module corner_post_back(x, y, h) {
    translate([x, y, 0]) {
        difference() {
            cylinder(d=post_outer_dia, h=h, $fn=$fn_fast);
            // Orificio pasante
            translate([0, 0, -$eps])
                cylinder(d=screw_clear_dia, h=h + 2*$eps, $fn=$fn_fast);
            // Cajeado avellanado para cabeza en la base
            translate([0, 0, -$eps])
                cylinder(d=screw_head_dia, h=screw_head_depth + $eps, $fn=$fn_fast);
        }
    }
}

// Ranura de ventilación / branquia con chaflán imprimible a 45°
module vent_slot_x(len, w, h) {
    hull() {
        translate([-len/2 + w/2, 0, 0]) cylinder(d=w, h=h, center=true, $fn=16);
        translate([ len/2 - w/2, 0, 0]) cylinder(d=w, h=h, center=true, $fn=16);
    }
}

/* ==========================================================================
 *  3. PIEZA 1: FRONTAL (piece_front)
 *     - Bisel de retención GC9A01 visible 33.0mm, pestaña 36.4mm
 *     - Cúpulas de sensores MQ-135 y MQ-7 de 19.8mm
 *     - Branquias de tiro convectivo térmico
 *     - Baffle de aislamiento térmico para DHT11
 *     - Rejilla ambiental dedicada para DHT11
 *     - Postes rígidos con separación exacta de 16.5mm para MPU-6050
 *     - Canalizaciones libres de cables (>15mm profundidad)
 * ========================================================================== */
module piece_front() {
    difference() {
        union() {
            // Chasis exterior sólido
            rounded_box(box_w, box_h, front_depth, corner_r);

            // Labio exterior macho para encaje perimetral con la tapa trasera
            translate([0, 0, front_depth])
                linear_extrude(height=lip_h)
                    difference() {
                        rounded_rect_2d(box_w - 2*wall_t, box_h - 2*wall_t, corner_r - wall_t);
                        rounded_rect_2d(box_w - 2*wall_t - 2*lip_t, box_h - 2*wall_t - 2*lip_t, max(1, corner_r - wall_t - lip_t));
                    }

            // 4 Postes de esquina para tornillos M3
            corner_post_front(-post_x, -post_y, front_depth);
            corner_post_front( post_x, -post_y, front_depth);
            corner_post_front( post_x,  post_y, front_depth);
            corner_post_front(-post_x,  post_y, front_depth);

            // Tabique térmico deflector (Baffle Wall): aísla la cámara caliente MQ del DHT11
            translate([-1.0, -box_h/2 + wall_t, wall_t])
                cube([2.0, 60.0, front_depth - wall_t - 4.0]); // Altura completa salvo pase de cables superior

            // Postes rígidos para MPU-6050 (GY-521) - Separación 16.5mm exacta
            translate([mpu_x - mpu_hole_pitch/2, mpu_y, wall_t])
                difference() {
                    cylinder(d=mpu_post_dia, h=mpu_post_h, $fn=$fn_fast);
                    translate([0, 0, -$eps])
                        cylinder(d=mpu_hole_dia, h=mpu_post_h + 2*$eps, $fn=$fn_fast);
                }
            translate([mpu_x + mpu_hole_pitch/2, mpu_y, wall_t])
                difference() {
                    cylinder(d=mpu_post_dia, h=mpu_post_h, $fn=$fn_fast);
                    translate([0, 0, -$eps])
                        cylinder(d=mpu_hole_dia, h=mpu_post_h + 2*$eps, $fn=$fn_fast);
                }

            // Nervios de retención y guías de PCB para sensores MQ
            translate([mq_x - mq_pcb_w/2 - 1.2, mq135_y - mq_pcb_h/2, wall_t])
                cube([1.2, mq_pcb_h, 8.0]);
            translate([mq_x + mq_pcb_w/2, mq135_y - mq_pcb_h/2, wall_t])
                cube([1.2, mq_pcb_h, 8.0]);

            translate([mq_x - mq_pcb_w/2 - 1.2, mq7_y - mq_pcb_h/2, wall_t])
                cube([1.2, mq_pcb_h, 8.0]);
            translate([mq_x + mq_pcb_w/2, mq7_y - mq_pcb_h/2, wall_t])
                cube([1.2, mq_pcb_h, 8.0]);

            // Cuna guía para sensor DHT11 en cámara fría
            translate([dht11_x - dht11_w/2 - 1.2, dht11_y - dht11_h/2, wall_t])
                cube([1.2, dht11_h, 8.0]);
            translate([dht11_x + dht11_w/2, dht11_y - dht11_h/2, wall_t])
                cube([1.2, dht11_h, 8.0]);
            translate([dht11_x - dht11_w/2, dht11_y - dht11_h/2 - 1.2, wall_t])
                cube([dht11_w, 1.2, 8.0]);

            // Anillo de retención trasero de la pantalla GC9A01
            translate([0, gc9a01_pos_y, wall_t])
                difference() {
                    cylinder(d=gc9a01_recess_dia + 4.0, h=4.0, $fn=$fn);
                    translate([0, 0, -$eps])
                        cylinder(d=gc9a01_recess_dia, h=4.0 + 2*$eps, $fn=$fn);
                }
        }

        // ================= SUSTRACCIONES Y MECANIZADOS =================
        // Cavidad interior principal (garantiza >15mm de profundidad libre para mazos de cables)
        translate([0, 0, wall_t])
            linear_extrude(height=front_depth + lip_h + 2*$eps)
                rounded_rect_2d(box_w - 2*wall_t, box_h - 2*wall_t, corner_r - wall_t);

        // A. Pantalla GC9A01: Bisel visible frontal (33.0mm)
        translate([0, gc9a01_pos_y, -$eps])
            cylinder(d=gc9a01_bezel_dia, h=wall_t + 2*$eps, $fn=$fn);

        // A2. Pantalla GC9A01: Rebaje de asiento para PCB y cristal (36.4mm x 2.6mm)
        translate([0, gc9a01_pos_y, wall_t - 1.4])
            cylinder(d=gc9a01_recess_dia, h=gc9a01_recess_depth + $eps, $fn=$fn);

        // A3. Pantalla GC9A01: Canal superior para cinta / cables SPI (18mm x 6mm)
        translate([-gc9a01_cable_w/2, gc9a01_pos_y + (gc9a01_recess_dia/2) - 3.0, wall_t - 1.4])
            cube([gc9a01_cable_w, gc9a01_cable_h + 4.0, 10.0]);

        // B. Cúpulas cilíndricas para MQ-135 y MQ-7 (19.8mm de paso libre de malla metálica)
        translate([mq_x, mq135_y, -$eps])
            cylinder(d=mq_cupola_dia, h=wall_t + 2*$eps, $fn=$fn);
        translate([mq_x, mq7_y, -$eps])
            cylinder(d=mq_cupola_dia, h=wall_t + 2*$eps, $fn=$fn);

        // C. Branquias de escape de calor convectivo (Cámara MQ - Izquierda)
        // Ventilación frontal para flujo de aire hacia los calefactores
        for (i = [0:3]) {
            translate([mq_x + 14.5, -45 + i*8.0, -$eps])
                vent_slot_x(14.0, 2.2, wall_t + 2*$eps);
        }
        // Branquias laterales (X = -box_w/2) para entrada de aire fresco a los sensores de gas
        for (i = [0:4]) {
            translate([-box_w/2, mq7_y + i*7.0, front_depth/2])
                rotate([0, 90, 0])
                    cylinder(d=2.4, h=wall_t*2 + 2*$eps, center=true, $fn=16);
        }

        // D. Rejilla ambiental frontal dedicada para DHT11 (Cámara Fría Aislada)
        for (i = [0:4]) {
            translate([dht11_x, dht11_y - 12.0 + i*6.0, -$eps])
                vent_slot_x(18.0, 2.2, wall_t + 2*$eps);
        }
        // Branquias laterales derechas (X = box_w/2) exclusivas del DHT11
        for (i = [0:3]) {
            translate([box_w/2, dht11_y - 8.0 + i*6.0, front_depth/2])
                rotate([0, 90, 0])
                    cylinder(d=2.4, h=wall_t*2 + 2*$eps, center=true, $fn=16);
        }

        // E. Ranuras de escape superior convectivo (efecto chimenea para los MQ)
        for (i = [0:3]) {
            translate([mq_x - 6.0 + i*6.0, box_h/2, front_depth/2])
                rotate([90, 0, 0])
                    cylinder(d=2.4, h=wall_t*2 + 2*$eps, center=true, $fn=16);
        }
    }
}

/* ==========================================================================
 *  4. PIEZA 2: TAPA TRASERA (piece_back)
 *     - Cuna para ESP32 DevKit V1 con rieles guía y holguras
 *     - Ventana avellanada para conector USB (12.5 x 8 mm con chaflán 45°)
 *     - Orificios de esquina para tornillos M3 con avellanado
 *     - Branquias traseras de disipación
 *     - Patas de apoyo para escritorio (Desktop Stand angulado a 12°)
 *     - Alojamiento perimetral hembra para encaje con front
 * ========================================================================== */
module piece_back() {
    difference() {
        union() {
            // Chasis base de la tapa trasera
            rounded_box(box_w, box_h, back_depth, corner_r);

            // 4 Postes con orificio pasante M3 y cajeado para cabeza
            corner_post_back(-post_x, -post_y, back_depth);
            corner_post_back( post_x, -post_y, back_depth);
            corner_post_back( post_x,  post_y, back_depth);
            corner_post_back(-post_x,  post_y, back_depth);

            // Cuna y rieles guía para ESP32 DevKit V1 (28.6 x 52.0 mm)
            // Riel izquierdo
            translate([esp32_x - (esp32_w/2 + clearance) - 2.0, esp32_y - esp32_h/2, wall_t])
                cube([2.0, esp32_h, 8.0]);
            // Riel derecho
            translate([esp32_x + (esp32_w/2 + clearance), esp32_y - esp32_h/2, wall_t])
                cube([2.0, esp32_h, 8.0]);
            // Tope superior ESP32
            translate([esp32_x - (esp32_w/2 + clearance) - 2.0, esp32_y + esp32_h/2, wall_t])
                cube([esp32_w + 2*clearance + 4.0, 2.0, 8.0]);
            // Postes de apoyo de PCB del ESP32
            translate([esp32_x - (esp32_w/2 - 2.0), esp32_y - esp32_h/2 + 4.0, wall_t])
                cylinder(d=4.0, h=3.0, $fn=$fn_fast);
            translate([esp32_x + (esp32_w/2 - 2.0), esp32_y - esp32_h/2 + 4.0, wall_t])
                cylinder(d=4.0, h=3.0, $fn=$fn_fast);
            translate([esp32_x - (esp32_w/2 - 2.0), esp32_y + esp32_h/2 - 4.0, wall_t])
                cylinder(d=4.0, h=3.0, $fn=$fn_fast);
            translate([esp32_x + (esp32_w/2 - 2.0), esp32_y + esp32_h/2 - 4.0, wall_t])
                cylinder(d=4.0, h=3.0, $fn=$fn_fast);

            // Patas de apoyo para escritorio (Desktop Stand inclinable a 12° en la base)
            // Pie izquierdo
            translate([-box_w/2 + 10.0, -box_h/2 + 8.0, -5.0])
                hull() {
                    translate([0, 0, 5.0]) cylinder(r=5.0, h=0.1, $fn=$fn_fast);
                    translate([0, -4.0, 0]) cylinder(r=4.0, h=1.0, $fn=$fn_fast);
                }
            // Pie derecho
            translate([box_w/2 - 10.0, -box_h/2 + 8.0, -5.0])
                hull() {
                    translate([0, 0, 5.0]) cylinder(r=5.0, h=0.1, $fn=$fn_fast);
                    translate([0, -4.0, 0]) cylinder(r=4.0, h=1.0, $fn=$fn_fast);
                }
            // Cuña trasera de apoyo en ángulo de escritorio
            translate([-25.0, box_h/2 - 20.0, -10.0])
                hull() {
                    translate([0, 0, 10.0]) cube([50.0, 10.0, 0.1]);
                    translate([5.0, 2.0, 0]) cube([40.0, 6.0, 1.0]);
                }
        }

        // ================= SUSTRACCIONES Y MECANIZADOS =================
        // Cavidad interior de la tapa trasera
        translate([0, 0, wall_t])
            linear_extrude(height=back_depth - wall_t + $eps)
                rounded_rect_2d(box_w - 2*wall_t, box_h - 2*wall_t, corner_r - wall_t);

        // Ranura hembra perimetral para recibir el labio de la pieza frontal
        translate([0, 0, back_depth - lip_h])
            linear_extrude(height=lip_h + $eps)
                difference() {
                    rounded_rect_2d(box_w - 2*wall_t + lip_clearance, box_h - 2*wall_t + lip_clearance, corner_r - wall_t);
                    rounded_rect_2d(box_w - 2*wall_t - 2*lip_t - lip_clearance, box_h - 2*wall_t - 2*lip_t - lip_clearance, max(1, corner_r - wall_t - lip_t));
                }

        // Ventana para conector Micro-USB / USB-C del ESP32 (12.5 x 8 mm)
        translate([esp32_x - esp32_usb_w/2, -box_h/2 - $eps, wall_t + 2.0])
            cube([esp32_usb_w, wall_t + 2*$eps, esp32_usb_h]);

        // Chaflán exterior avellanado para conector USB (evita tropezar con fundas gruesas de cables)
        translate([esp32_x, -box_h/2, wall_t + 2.0 + esp32_usb_h/2])
            rotate([90, 0, 0])
                hull() {
                    cube([esp32_usb_w, esp32_usb_h, 0.1], center=true);
                    cube([esp32_usb_w + 2*esp32_usb_chamfer, esp32_usb_h + 2*esp32_usb_chamfer, 3.0], center=true);
                }

        // Branquias traseras de ventilación para el ESP32 (disipación térmica)
        for (i = [-3:3]) {
            translate([esp32_x + i*7.0, esp32_y + 12.0, -$eps])
                vent_slot_x(2.2, 22.0, wall_t + 2*$eps);
        }

        // 4 Cunas circulares para almohadillas de goma antideslizantes (Ø 8.5mm x 1.0mm profundidad)
        translate([-post_x, -post_y, -5.0 - $eps])
            cylinder(d=8.5, h=1.2, $fn=$fn_fast);
        translate([ post_x, -post_y, -5.0 - $eps])
            cylinder(d=8.5, h=1.2, $fn=$fn_fast);
        translate([-20.0, box_h/2 - 15.0, -10.0 - $eps])
            cylinder(d=8.5, h=1.2, $fn=$fn_fast);
        translate([ 20.0, box_h/2 - 15.0, -10.0 - $eps])
            cylinder(d=8.5, h=1.2, $fn=$fn_fast);
    }
}

/* ==========================================================================
 *  5. MÓDULOS DE ENSAMBLE Y VISUALIZACIÓN
 * ========================================================================== */
module piece_assembly() {
    // Pieza frontal en posición de trabajo
    color([0.15, 0.20, 0.25, 0.95])
        piece_front();

    // Pieza trasera orientada y acoplada a la pieza frontal
    translate([0, 0, front_depth + back_depth])
        rotate([0, 180, 0])
            color([0.85, 0.50, 0.10, 0.90])
                piece_back();
}

module piece_exploded() {
    // Frontal
    color([0.15, 0.20, 0.25, 0.95])
        piece_front();

    // Trasera separada 40mm para inspeccionar interiores
    translate([0, 0, front_depth + back_depth + 40.0])
        rotate([0, 180, 0])
            color([0.85, 0.50, 0.10, 0.85])
                piece_back();
}

/* ==========================================================================
 *  6. CONTROL DE EJECUCIÓN
 * ========================================================================== */
if (part == "front") {
    piece_front();
} else if (part == "back") {
    piece_back();
} else if (part == "exploded") {
    piece_exploded();
} else {
    piece_assembly();
}
