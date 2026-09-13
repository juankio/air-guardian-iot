/* ==========================================================================
 *  AIR GUARDIAN IOT - MASCOTA ROBOT TRIANGULAR (HARDWARE 3D)
 *  DISEÑO PARAMÉTRICO DE CARCASA MECHA CHIBI CON OJO CIRCULAR GC9A01
 *  Arquitecto Naval: Franky (Master Shipwright)
 *  Tecnología: OpenSCAD v2021.01+ / FDM 3D Printing Ready (No Supports)
 * ========================================================================== */

// Selección de renderizado (Frontal, Trasera, Ensamble o Vista Explosionada)
part = "assembly"; // [ "front", "back", "assembly", "exploded", "showcase" ]

// Calidad de resolución de curvas y cilindros
$fn = 48;
$fn_fast = 24;
$eps = 0.05; // Margen anti-z-fighting para booleanos CSG limpios

/* ==========================================================================
 *  1. PARÁMETROS GEOMÉTRICOS Y TOLERANCIAS MECÁNICAS (+0.25mm Clearance)
 * ========================================================================== */
clearance             = 0.25; // Holgura perimetral para acople FDM suave
wall_t                = 2.40; // Espesor uniforme de pared estructural (2.40 mm)
front_depth           = 22.0; // Profundidad de la pieza frontal
back_depth            = 18.0; // Profundidad de la tapa trasera (Total interior: 35.2 mm libre)

// Perfil de Mascota Robot Triangular
tri_base_x            = 44.0; // Semiancho entre centros de base inferior
tri_base_y            = -36.0; // Posición Y de centros de base
tri_base_r            = 15.0; // Radio de redondeo en esquinas inferiores

tri_top_y             = 48.0; // Posición Y del vértice superior de la cabeza
tri_top_r             = 14.0; // Radio de redondeo de la cresta superior

// Orejitas Cilíndricas Robot (Pods para Sensores de Gas MQ-135 y MQ-7)
ear_x                 = 38.0; // Distancia X del centro de cada orejita
ear_y                 = 22.0; // Posición Y del centro de cada orejita
ear_r                 = 16.0; // Radio exterior del cilindro de cada oreja

// Pantalla Circular GC9A01 (1.28" TFT SPI 240x240 - El Gran Ojo Central)
gc9a01_bezel_dia      = 33.00; // Diámetro visible de apertura frontal
gc9a01_recess_dia     = 36.40; // Rebaje interior de asiento del PCB (+0.9mm)
gc9a01_recess_depth   = 2.60;  // Profundidad del rebaje para cristal y PCB
gc9a01_cable_w        = 18.00; // Ancho canal flexible de paso para bus SPI
gc9a01_cable_h        = 6.00;  // Altura del canal flexible de paso
gc9a01_pos_x          = 0.00;  // Centrado en X
gc9a01_pos_y          = 3.00;  // Posición Y del centro del ojo en la cara

// Sensores de Gas MQ-135 y MQ-7 (Malla cilíndrica asomando por las orejas)
mq_cupola_dia         = 20.00; // Diámetro de paso para cúpula de malla de acero
mq_pcb_w              = 32.50; // Largo PCB de módulo MQ
mq_pcb_h              = 20.50; // Ancho PCB de módulo MQ

// Boca de Ventilación y Cámara de Sensor DHT11 (Sonrisa Mecha Inferior)
dht11_w               = 16.20; // Ancho del sensor DHT11
dht11_h               = 27.50; // Alto del sensor DHT11
dht11_d               = 12.00; // Profundidad del cuerpo DHT11
dht11_x               = 0.00;  // Centrado en X detrás de la sonrisa
dht11_y               = -22.0; // Posición Y detrás de la boca

// Sensor Inercial MPU-6050 (GY-521 - Corazón Inercial en el Piso de la Base)
mpu_hole_pitch        = 16.50; // Separación exacta entre taladros M3
mpu_hole_dia          = 2.90;  // Núcleo para roscar tornillos M3 autorroscantes
mpu_post_dia          = 6.20;  // Diámetro exterior de los postes
mpu_post_h            = 6.00;  // Altura de postes rígidos
mpu_pos_x             = 20.00; // En el piso derecho de la base triangular
mpu_pos_y             = -34.00;// Posición Y sobre la base inferior

// ESP32 DevKit V1 (Microcontrolador / Cerebro Trasero)
esp32_w               = 28.60; // Ancho PCB ESP32
esp32_h               = 52.00; // Largo PCB ESP32
esp32_usb_w           = 13.00; // Ancho de la ventana USB
esp32_usb_h           = 8.00;  // Alto de la ventana USB
esp32_usb_chamfer     = 2.20;  // Chaflán exterior para conectores anchos
esp32_x               = 0.00;  // Centrado en X
esp32_y               = -8.00; // Posición vertical en la tapa trasera

// Sistema de Postes de Cierre Triangular (3 Vértices M3)
post_outer_dia        = 7.60;  // Diámetro exterior de los postes de fijación
screw_core_dia        = 2.90;  // Núcleo hembra roscable M3 (Frontal)
screw_clear_dia       = 3.40;  // Orificio pasante libre M3 (Trasera)
screw_head_dia        = 6.20;  // Alojamiento de cabeza cilíndrica M3 (DIN 912)
screw_head_depth      = 3.50;  // Profundidad de cajeado de cabeza M3

// Posiciones de los 3 postes en los vértices del triángulo
post_v1_x             = -40.0; // Vértice inferior izquierdo
post_v1_y             = -32.0;
post_v2_x             =  40.0; // Vértice inferior derecho
post_v2_y             = -32.0;
post_v3_x             =   0.0; // Vértice superior central
post_v3_y             =  44.0;

// Labio Machihembrado Escalonado (Step Joint Lip & Groove)
lip_h                 = 1.50;  // Altura del labio de unión
step_t                = wall_t / 2; // Ancho del escalón (1.20 mm)

/* ==========================================================================
 *  2. MÓDULOS GEOMÉTRICOS DEL PERFIL Y ELEMENTOS AUXILIARES
 * ========================================================================== */

// Contorno 2D exterior orgánico de la Mascota Robot Triangular
module mascot_contour_2d() {
    union() {
        // Silueta triangular suave con esquinas redondeadas
        hull() {
            translate([-tri_base_x, tri_base_y]) circle(r=tri_base_r, $fn=$fn);
            translate([ tri_base_x, tri_base_y]) circle(r=tri_base_r, $fn=$fn);
            translate([ 0,          tri_top_y])  circle(r=tri_top_r,  $fn=$fn);
        }
        // Orejita izquierda (Robot Ear Pod - MQ-135)
        translate([-ear_x, ear_y]) circle(r=ear_r, $fn=$fn);
        // Orejita derecha (Robot Ear Pod - MQ-7)
        translate([ ear_x, ear_y]) circle(r=ear_r, $fn=$fn);
    }
}

// Prisma 3D del cuerpo exterior
module mascot_body_3d(height) {
    linear_extrude(height=height)
        mascot_contour_2d();
}

// Ranura de ventilación horizontal con extremos semicirculares
module vent_slot(len, w, h) {
    hull() {
        translate([-len/2 + w/2, 0, 0]) cylinder(d=w, h=h, center=true, $fn=16);
        translate([ len/2 - w/2, 0, 0]) cylinder(d=w, h=h, center=true, $fn=16);
    }
}

// Poste hembra M3 para la pieza frontal (roscado directo en plástico)
module post_front(x, y, h) {
    translate([x, y, 0]) {
        difference() {
            cylinder(d=post_outer_dia, h=h, $fn=$fn_fast);
            translate([0, 0, h - 14.0])
                cylinder(d=screw_core_dia, h=14.0 + $eps, $fn=$fn_fast);
        }
    }
}

// Poste pasante M3 para la tapa trasera (pasante 3.4mm + cajeado para cabeza)
module post_back(x, y, h) {
    translate([x, y, 0]) {
        difference() {
            cylinder(d=post_outer_dia, h=h, $fn=$fn_fast);
            // Paso pasante del vástago de tornillo M3
            translate([0, 0, -$eps])
                cylinder(d=screw_clear_dia, h=h + 2*$eps, $fn=$fn_fast);
            // Cajeado avellanado para la cabeza cilíndrica M3
            translate([0, 0, -$eps])
                cylinder(d=screw_head_dia, h=screw_head_depth + $eps, $fn=$fn_fast);
        }
    }
}

/* ==========================================================================
 *  3. PIEZA 1: PIEZA FRONTAL (piece_mascot_front)
 * ========================================================================== */
module piece_mascot_front() {
    difference() {
        union() {
            // A. Concha exterior con cavidad interna uniforme (wall_t = 2.40 mm)
            difference() {
                mascot_body_3d(front_depth);
                translate([0, 0, wall_t])
                    linear_extrude(height=front_depth + lip_h + 2*$eps)
                        offset(r = -wall_t)
                            mascot_contour_2d();
            }

            // B. Labio perimetral macho (Lip) escalonado en la mitad interior de la pared
            translate([0, 0, front_depth])
                linear_extrude(height=lip_h)
                    difference() {
                        offset(r = -step_t) mascot_contour_2d();
                        offset(r = -wall_t) mascot_contour_2d();
                    }

            // C. 3 Postes estructurales de fijación en los vértices del triángulo
            post_front(post_v1_x, post_v1_y, front_depth);
            post_front(post_v2_x, post_v2_y, front_depth);
            post_front(post_v3_x, post_v3_y, front_depth);

            // D. Anillo interno de retención de la pantalla circular GC9A01
            translate([gc9a01_pos_x, gc9a01_pos_y, wall_t - $eps])
                difference() {
                    cylinder(d=gc9a01_recess_dia + 3.6, h=4.2 + $eps, $fn=$fn);
                    translate([0, 0, -$eps])
                        cylinder(d=gc9a01_recess_dia, h=4.2 + 3*$eps, $fn=$fn);
                }

            // E. Anillo exterior decorativo mecha alrededor del ojo (Bisel de lente)
            translate([gc9a01_pos_x, gc9a01_pos_y, 0])
                difference() {
                    cylinder(d=gc9a01_bezel_dia + 4.8, h=0.6, $fn=$fn);
                    translate([0, 0, -$eps])
                        cylinder(d=gc9a01_bezel_dia - 0.2, h=0.6 + 2*$eps, $fn=$fn);
                }

            // F. Tabique térmico deflector (Baffle Wall): aísla la cámara fría del DHT11
            translate([-15.0, -32.0, wall_t - $eps])
                cube([2.0, 18.0, front_depth - wall_t - 4.0 + $eps]);
            translate([ 13.0, -32.0, wall_t - $eps])
                cube([2.0, 18.0, front_depth - wall_t - 4.0 + $eps]);

            // G. Cuna y nervios de sujeción para el sensor ambiental DHT11 (U continua limpia)
            translate([dht11_x - dht11_w/2 - 1.2, dht11_y - dht11_h/2, wall_t - $eps])
                cube([1.2, dht11_h, 7.5 + $eps]);
            translate([dht11_x + dht11_w/2, dht11_y - dht11_h/2, wall_t - $eps])
                cube([1.2, dht11_h, 7.5 + $eps]);
            translate([dht11_x - dht11_w/2 - 1.2, dht11_y - dht11_h/2 - 1.2, wall_t - $eps])
                cube([dht11_w + 2.4, 1.2, 7.5 + $eps]);

            // H. Guías de sujeción de PCB para sensores de gas MQ en las orejitas
            // Oreja izquierda (MQ-135)
            translate([-ear_x - mq_pcb_h/2 - 1.2, ear_y - mq_pcb_w/2, wall_t - $eps])
                cube([1.2, mq_pcb_w, 7.0 + $eps]);
            translate([-ear_x + mq_pcb_h/2, ear_y - mq_pcb_w/2, wall_t - $eps])
                cube([1.2, mq_pcb_w, 7.0 + $eps]);

            // Oreja derecha (MQ-7)
            translate([ ear_x - mq_pcb_h/2 - 1.2, ear_y - mq_pcb_w/2, wall_t - $eps])
                cube([1.2, mq_pcb_w, 7.0 + $eps]);
            translate([ ear_x + mq_pcb_h/2, ear_y - mq_pcb_w/2, wall_t - $eps])
                cube([1.2, mq_pcb_w, 7.0 + $eps]);

            // I. Postes rígidos para MPU-6050 (GY-521) - Separación exacta 16.5mm
            translate([mpu_pos_x - mpu_hole_pitch/2, mpu_pos_y, wall_t - $eps])
                difference() {
                    cylinder(d=mpu_post_dia, h=mpu_post_h + $eps, $fn=$fn_fast);
                    translate([0, 0, -$eps])
                        cylinder(d=mpu_hole_dia, h=mpu_post_h + 3*$eps, $fn=$fn_fast);
                }
            translate([mpu_pos_x + mpu_hole_pitch/2, mpu_pos_y, wall_t - $eps])
                difference() {
                    cylinder(d=mpu_post_dia, h=mpu_post_h + $eps, $fn=$fn_fast);
                    translate([0, 0, -$eps])
                        cylinder(d=mpu_hole_dia, h=mpu_post_h + 3*$eps, $fn=$fn_fast);
                }
        }

        // ================= SUSTRACCIONES Y MECANIZADOS =================
        // 1. Ojo Central: Bisel circular de retención visible (33.0mm)
        translate([gc9a01_pos_x, gc9a01_pos_y, -$eps])
            cylinder(d=gc9a01_bezel_dia, h=wall_t + 2*$eps, $fn=$fn);

        // 1b. Ojo Central: Rebaje de asiento para PCB y cristal (36.4mm x 2.6mm)
        translate([gc9a01_pos_x, gc9a01_pos_y, wall_t - 1.2])
            cylinder(d=gc9a01_recess_dia, h=gc9a01_recess_depth + $eps, $fn=$fn);

        // 1c. Ojo Central: Canal superior flexible para bus SPI (18.0mm x 6.0mm)
        translate([gc9a01_pos_x - gc9a01_cable_w/2, gc9a01_pos_y + (gc9a01_recess_dia/2) - 2.5, wall_t - 1.2])
            cube([gc9a01_cable_w, gc9a01_cable_h + 3.0, 8.0]);

        // 2. Orejas de Robot: Orificios circulares para mallas de sensores MQ (20.0mm)
        translate([-ear_x, ear_y, -$eps])
            cylinder(d=mq_cupola_dia, h=wall_t + 2*$eps, $fn=$fn);
        translate([ ear_x, ear_y, -$eps])
            cylinder(d=mq_cupola_dia, h=wall_t + 2*$eps, $fn=$fn);

        // 3. Chimeneas Convectivas de las Orejas (Escape térmico para los calefactores MQ)
        for (i = [-1:1]) {
            // Oreja izquierda
            translate([-ear_x + i*4.2, ear_y + ear_r - 2.0, front_depth/2])
                rotate([90, 0, 0])
                    cylinder(d=2.4, h=wall_t*3, center=true, $fn=16);
            // Oreja derecha
            translate([ ear_x + i*4.2, ear_y + ear_r - 2.0, front_depth/2])
                rotate([90, 0, 0])
                    cylinder(d=2.4, h=wall_t*3, center=true, $fn=16);
        }

        // 4. Boca de Ventilación (Sonrisa Mecha Inferior para DHT11)
        translate([0, -15.5, -$eps]) vent_slot(28.0, 2.2, wall_t + 2*$eps);
        translate([0, -19.5, -$eps]) vent_slot(24.0, 2.2, wall_t + 2*$eps);
        translate([0, -23.5, -$eps]) vent_slot(20.0, 2.2, wall_t + 2*$eps);
        translate([0, -27.5, -$eps]) vent_slot(15.0, 2.2, wall_t + 2*$eps);

        // 5. Ranuras inferiores de admisión en la base de la boca
        for (i = [-1:1]) {
            translate([i*6.0, -31.5, -$eps])
                vent_slot(4.0, 2.0, wall_t + 2*$eps);
        }
    }
}

/* ==========================================================================
 *  4. PIEZA 2: TAPA TRASERA (piece_mascot_back)
 * ========================================================================== */
module piece_mascot_back() {
    difference() {
        union() {
            // A. Concha exterior de la tapa trasera con cavidad interna
            difference() {
                mascot_body_3d(back_depth);
                translate([0, 0, wall_t])
                    linear_extrude(height=back_depth - wall_t + $eps)
                        offset(r = -wall_t)
                            mascot_contour_2d();
            }

            // B. 3 Postes estructurales pasantes para tornillos M3
            post_back(post_v1_x, post_v1_y, back_depth);
            post_back(post_v2_x, post_v2_y, back_depth);
            post_back(post_v3_x, post_v3_y, back_depth);

            // C. Cuna y rieles guía para ESP32 DevKit V1 (28.6 x 52.0 mm)
            // Riel izquierdo
            translate([esp32_x - (esp32_w/2 + clearance) - 2.0, esp32_y - esp32_h/2, wall_t - $eps])
                cube([2.0, esp32_h, 7.5 + $eps]);
            // Riel derecho
            translate([esp32_x + (esp32_w/2 + clearance), esp32_y - esp32_h/2, wall_t - $eps])
                cube([2.0, esp32_h, 7.5 + $eps]);
            // Tope superior del ESP32
            translate([esp32_x - (esp32_w/2 + clearance) - 2.0, esp32_y + esp32_h/2, wall_t - $eps])
                cube([esp32_w + 2*clearance + 4.0, 2.0, 7.5 + $eps]);

            // Postes de apoyo para las 4 esquinas del PCB del ESP32
            translate([esp32_x - (esp32_w/2 - 2.0), esp32_y - esp32_h/2 + 4.0, wall_t - $eps])
                cylinder(d=4.0, h=3.0 + $eps, $fn=$fn_fast);
            translate([esp32_x + (esp32_w/2 - 2.0), esp32_y - esp32_h/2 + 4.0, wall_t - $eps])
                cylinder(d=4.0, h=3.0 + $eps, $fn=$fn_fast);
            translate([esp32_x - (esp32_w/2 - 2.0), esp32_y + esp32_h/2 - 4.0, wall_t - $eps])
                cylinder(d=4.0, h=3.0 + $eps, $fn=$fn_fast);
            translate([esp32_x + (esp32_w/2 - 2.0), esp32_y + esp32_h/2 - 4.0, wall_t - $eps])
                cylinder(d=4.0, h=3.0 + $eps, $fn=$fn_fast);

            // D. Patas de apoyo para escritorio (Desktop Stand inclinable a 12° en la base)
            // Pie izquierdo
            translate([-tri_base_x + 6.0, tri_base_y - 6.0, -4.5])
                hull() {
                    translate([0, 0, 4.5]) cylinder(r=5.5, h=0.1, $fn=$fn_fast);
                    translate([0, -2.5, 0]) cylinder(r=4.5, h=1.0, $fn=$fn_fast);
                }
            // Pie derecho
            translate([ tri_base_x - 6.0, tri_base_y - 6.0, -4.5])
                hull() {
                    translate([0, 0, 4.5]) cylinder(r=5.5, h=0.1, $fn=$fn_fast);
                    translate([0, -2.5, 0]) cylinder(r=4.5, h=1.0, $fn=$fn_fast);
                }
            // Cuña trasera de apoyo en ángulo de escritorio (Puntal de inclinación)
            translate([-18.0, tri_top_y - 14.0, -8.5])
                hull() {
                    translate([0, 0, 8.5]) cube([36.0, 8.0, 0.1]);
                    translate([3.0, 2.0, 0]) cube([30.0, 5.0, 1.0]);
                }
        }

        // ================= SUSTRACCIONES Y MECANIZADOS =================
        // 1. Ranura hembra perimetral escalonada (Groove) para recibir el labio de la frontal
        translate([0, 0, back_depth - lip_h])
            linear_extrude(height=lip_h + $eps)
                difference() {
                    offset(r = -step_t + clearance) mascot_contour_2d();
                    offset(r = -wall_t - clearance) mascot_contour_2d();
                }

        // 2. Ventana para conector Micro-USB / USB-C del ESP32 en la base inferior
        translate([esp32_x - esp32_usb_w/2, -55.0, wall_t + 1.2])
            cube([esp32_usb_w, 22.0, esp32_usb_h]);

        // 2b. Chaflán exterior avellanado para conector USB
        translate([esp32_x, tri_base_y - tri_base_r, wall_t + 1.2 + esp32_usb_h/2])
            rotate([90, 0, 0])
                hull() {
                    cube([esp32_usb_w, esp32_usb_h, 0.1], center=true);
                    cube([esp32_usb_w + 2*esp32_usb_chamfer, esp32_usb_h + 2*esp32_usb_chamfer, 3.5], center=true);
                }

        // 3. Branquias traseras de ventilación para disipación térmica del ESP32
        for (i = [-2:2]) {
            translate([esp32_x + i*6.5, esp32_y + 8.0, -$eps])
                vent_slot(2.2, 18.0, wall_t + 2*$eps);
        }

        // 4. Cunas para almohadillas de goma antideslizantes (Ø 8.5mm x 1.2mm)
        translate([-tri_base_x + 6.0, tri_base_y - 6.0, -4.5 - $eps])
            cylinder(d=8.5, h=1.4, $fn=$fn_fast);
        translate([ tri_base_x - 6.0, tri_base_y - 6.0, -4.5 - $eps])
            cylinder(d=8.5, h=1.4, $fn=$fn_fast);
        translate([ 0.0, tri_top_y - 10.0, -8.5 - $eps])
            cylinder(d=8.5, h=1.4, $fn=$fn_fast);
    }
}

/* ==========================================================================
 *  5. MÓDULOS DE ENSAMBLE Y VISUALIZACIÓN
 * ========================================================================== */

// Ensamble 3D con la cara frontal orientada hacia arriba (+Z) para inspección directa
module piece_mascot_assembly() {
    // Pieza frontal con su cara hacia arriba en Z = front_depth + back_depth (40mm)
    translate([0, 0, front_depth + back_depth])
        rotate([0, 180, 0])
            color([0.20, 0.25, 0.32, 0.98]) // Gris Mecha Robot
                piece_mascot_front();

    // Pieza trasera descansando sobre su base en Z = 0
    color([0.92, 0.48, 0.10, 0.95]) // Naranja Thousand Sunny
        piece_mascot_back();
}

// Vista Showcase para renders con la pantalla y mallas de sensores
module piece_mascot_showcase() {
    piece_mascot_assembly();

    // Representación del Gran Ojo GC9A01 (Pantalla activa turquesa robot)
    translate([gc9a01_pos_x, gc9a01_pos_y, front_depth + back_depth + 0.2])
        color([0.05, 0.90, 0.95, 0.95])
            cylinder(d=gc9a01_bezel_dia - 0.4, h=0.2, $fn=$fn);

    // Representación de las mallas metálicas de los sensores de gas MQ
    translate([-ear_x, ear_y, front_depth + back_depth + 0.5])
        color([0.78, 0.80, 0.85, 0.90])
            cylinder(d=mq_cupola_dia - 0.4, h=3.5, $fn=$fn);
    translate([ ear_x, ear_y, front_depth + back_depth + 0.5])
        color([0.78, 0.80, 0.85, 0.90])
            cylinder(d=mq_cupola_dia - 0.4, h=3.5, $fn=$fn);
}

// Vista Explosionada para ver el interior
module piece_mascot_exploded() {
    // Tapa trasera abajo en Z = 0
    color([0.92, 0.48, 0.10, 0.90])
        piece_mascot_back();

    // Pieza frontal elevada 45mm en Z
    translate([0, 0, front_depth + back_depth + 45.0])
        rotate([0, 180, 0])
            color([0.20, 0.25, 0.32, 0.95])
                piece_mascot_front();
}

/* ==========================================================================
 *  6. CONTROL DE COMPILACIÓN / EJECUCIÓN
 * ========================================================================== */
if (part == "front") {
    piece_mascot_front();
} else if (part == "back") {
    piece_mascot_back();
} else if (part == "exploded") {
    piece_mascot_exploded();
} else if (part == "showcase") {
    piece_mascot_showcase();
} else {
    piece_mascot_assembly();
}
