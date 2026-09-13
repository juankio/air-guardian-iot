#!/usr/bin/env python3
"""
=============================================================================
 AIR GUARDIAN IOT - GENERADOR DE FICHA TÉCNICA VISUAL DE MASCOTA TRIANGULAR
 Master Shipwright: Franky
 Combina vistas ortográficas, axonométricas y explosionadas en un render pro.
=============================================================================
"""

import os
from PIL import Image, ImageDraw, ImageFont

BASE_DIR = "/home/juankio/air-guardian-iot/hardware_3d"
VIEW_FRONT = os.path.join(BASE_DIR, "view_front.png")
VIEW_ISO = os.path.join(BASE_DIR, "view_iso.png")
VIEW_EXPLODED = os.path.join(BASE_DIR, "view_exploded.png")
VIEW_BACK = os.path.join(BASE_DIR, "view_back.png")
OUT_PNG = os.path.join(BASE_DIR, "preview_mascot_triangle.png")

# Crear lienzo maestro 2400 x 1800 (Fondo oscuro Cyberpunk Mecha)
WIDTH = 2400
HEIGHT = 1800
canvas = Image.new("RGBA", (WIDTH, HEIGHT), (15, 23, 42, 255)) # Dark slate navy
draw = ImageDraw.Draw(canvas)

# Intentar cargar fuentes TrueType del sistema o usar default
try:
    font_title = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 44)
    font_subtitle = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24)
    font_panel = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 26)
    font_badge = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 20)
    font_data = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 20)
except Exception:
    try:
        font_title = ImageFont.truetype("DejaVuSans-Bold.ttf", 44)
        font_subtitle = ImageFont.truetype("DejaVuSans.ttf", 24)
        font_panel = ImageFont.truetype("DejaVuSans-Bold.ttf", 26)
        font_badge = ImageFont.truetype("DejaVuSans-Bold.ttf", 20)
        font_data = ImageFont.truetype("DejaVuSansMono.ttf", 20)
    except Exception:
        font_title = ImageFont.load_default()
        font_subtitle = font_title
        font_panel = font_title
        font_badge = font_title
        font_data = font_title

# Encabezado Principal
draw.rectangle([(40, 40), (WIDTH - 40, 140)], fill=(30, 41, 59, 255), outline=(56, 189, 248, 255), width=2)
draw.text((65, 55), "AIR GUARDIAN IOT — MASCOTA ROBOT TRIANGULAR", fill=(248, 250, 252, 255), font=font_title)
draw.text((65, 105), "DISEÑO HARDWARE 3D MECATRÓNICO • PANTALLA CIRCULAR GC9A01 • FRANKY SHIPWRIGHT ENGINE", fill=(148, 163, 184, 255), font=font_subtitle)

# Medidas de los 4 paneles
panel_w = 1130
panel_h = 720
gap_x = 60
gap_y = 50
top_y = 170

panels = [
    ("PANEL A: CARA ROBOT • OJO CENTRAL GC9A01 & OREJAS MQ", VIEW_FRONT, 40, top_y, (56, 189, 248, 255)),
    ("PANEL B: ISOMÉTRICA 3D • PERFIL TRIANGULAR ORGÁNICO", VIEW_ISO, 40 + panel_w + gap_x, top_y, (249, 115, 22, 255)),
    ("PANEL C: VISTA EXPLOSIONADA • LIPS & GROOVE + BAFFLES", VIEW_EXPLODED, 40, top_y + panel_h + gap_y, (168, 85, 247, 255)),
    ("PANEL D: TAPA TRASERA • CUNA ESP32 & PUERTO USB INFERIOR", VIEW_BACK, 40 + panel_w + gap_x, top_y + panel_h + gap_y, (34, 197, 94, 255)),
]

for title, img_path, px, py, accent_col in panels:
    # Marco de panel
    draw.rectangle([(px, py), (px + panel_w, py + panel_h)], fill=(24, 32, 47, 255), outline=(51, 65, 85, 255), width=2)
    # Cabecera de panel
    draw.rectangle([(px, py), (px + panel_w, py + 48)], fill=(30, 41, 59, 255), outline=accent_col, width=1)
    draw.text((px + 20, py + 10), title, fill=(241, 245, 249, 255), font=font_panel)

    if os.path.exists(img_path):
        img = Image.open(img_path)
        # Recortar márgenes transparentes o blancos si los tiene
        # Redimensionar para encajar dentro del área del panel (ancho 1090, alto 640)
        img_ratio = img.width / img.height
        target_w = panel_w - 40
        target_h = panel_h - 70
        target_ratio = target_w / target_h

        if img_ratio > target_ratio:
            new_w = target_w
            new_h = int(new_w / img_ratio)
        else:
            new_h = target_h
            new_w = int(new_h * img_ratio)

        resized = img.resize((new_w, new_h), Image.Resampling.LANCZOS)
        offset_x = px + (panel_w - new_w) // 2
        offset_y = py + 55 + (panel_h - 60 - new_h) // 2
        canvas.paste(resized, (offset_x, offset_y), resized if resized.mode == "RGBA" else None)

# Barra inferior de datos técnicos (Footer)
footer_y = HEIGHT - 90
draw.rectangle([(40, footer_y), (WIDTH - 40, HEIGHT - 30)], fill=(30, 41, 59, 255), outline=(56, 189, 248, 255), width=2)

specs = [
    "DIMENSIONES: 118 x 113 x 48.5 mm",
    "PARED: 2.40 mm Uniforme",
    "PANTALLA: GC9A01 1.28\" (33mm / 36.4mm)",
    "SENSORES: MQ-135, MQ-7, DHT11, MPU-6050",
    "CEREBRO: ESP32 DevKit V1",
    "CIERRE: 3x M3 en Vértices",
    "CALIDAD MALLA: 100% Watertight (Trimesh)",
]
spec_str = "  |  ".join(specs)
draw.text((65, footer_y + 18), spec_str, fill=(226, 232, 240, 255), font=font_data)

canvas.save(OUT_PNG, quality=95)
print(f"🦾 [Franky Shipwright] Lámina técnica maestra guardada con éxito en: {OUT_PNG}")
