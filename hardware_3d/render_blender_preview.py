#!/usr/bin/env python3
"""
=============================================================================
 AIR GUARDIAN IOT - 3D PREVIEW & BLENDER / TRIMESH RENDER ENGINE
 Arquitecto: Franky (Master Shipwright)
 
 Este script tiene modo dual inteligente:
 1. Modo Standalone / Headless (trimesh + matplotlib):
    Genera vistas ortográficas e isométricas de alta resolución en PNG.
 2. Modo Blender Automático (bpy):
    Si se invoca desde Blender ('blender -b -P render_blender_preview.py'):
    Configura escena de estudio de tres puntos, materiales PBR de plástico FDM
    (Gris carbón y Naranja Thousand Sunny) y renderiza la carcasa.
=============================================================================
"""

import os
import sys

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
STL_FRONT = os.path.join(BASE_DIR, "air_guardian_front.stl")
STL_BACK = os.path.join(BASE_DIR, "air_guardian_back.stl")
STL_ASSEMBLY = os.path.join(BASE_DIR, "air_guardian_assembly.stl")

def render_with_trimesh_matplotlib():
    """Genera renders 3D axonométricos y ortogonales usando trimesh y matplotlib."""
    import trimesh
    import numpy as np
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d import Axes3D
    from mpl_toolkits.mplot3d.art3d import Poly3DCollection

    print("🦾 [Franky 3D Engine] Iniciando renderizado técnico con Trimesh & Matplotlib...")

    parts = [
        ("Frontal (Chasis de Sensores)", STL_FRONT, "preview_front.png", "#1f2937", (25, -55)),
        ("Tapa Trasera (Cuna ESP32)", STL_BACK, "preview_back.png", "#c2410c", (25, -55)),
        ("Ensamble Completo Air Guardian", STL_ASSEMBLY, "preview_assembly.png", "#0284c7", (30, 45)),
    ]

    for title, stl_path, out_png, color, (elev, azim) in parts:
        if not os.path.exists(stl_path):
            print(f"⚠️ Archivo no encontrado: {stl_path}")
            continue

        mesh = trimesh.load(stl_path)
        print(f"-> Procesando {title} ({len(mesh.faces)} caras, {len(mesh.vertices)} vértices)...")

        fig = plt.figure(figsize=(10, 8), dpi=150)
        ax = fig.add_subplot(111, projection="3d")
        ax.set_facecolor("#0f172a")
        fig.patch.set_facecolor("#0f172a")

        # Submuestrear caras para visualización ultra-rápida si supera 20,000
        faces = mesh.faces
        vertices = mesh.vertices
        if len(faces) > 8000:
            step = len(faces) // 6000
            faces = faces[::step]

        triangles = vertices[faces]
        poly = Poly3DCollection(triangles, alpha=0.92, linewidths=0.15)
        poly.set_facecolor(color)
        poly.set_edgecolor("#334155")
        ax.add_collection3d(poly)

        # Configurar límites y aspecto
        scale = vertices.flatten()
        ax.auto_scale_xyz(scale, scale, scale)
        ax.view_init(elev=elev, azim=azim)

        # Ocultar paneles y cuadrículas para aspecto CAD limpio
        ax.xaxis.pane.fill = False
        ax.yaxis.pane.fill = False
        ax.zaxis.pane.fill = False
        ax.grid(False)
        ax.set_xticks([])
        ax.set_yticks([])
        ax.set_zticks([])

        plt.title(f"Air Guardian IoT - {title}\nDimensiones: {mesh.extents[0]:.1f} x {mesh.extents[1]:.1f} x {mesh.extents[2]:.1f} mm | Watertight: {mesh.is_watertight}",
                  color="#f8fafc", fontsize=11, fontweight="bold", pad=12)

        out_path = os.path.join(BASE_DIR, out_png)
        plt.tight_layout()
        plt.savefig(out_path, facecolor=fig.get_facecolor(), edgecolor="none")
        plt.close(fig)
        print(f"✅ Guardado render: {out_path}")

    # Generar hoja de contacto técnica combinada (Orthographic Tri-View)
    render_technical_contact_sheet()

def render_technical_contact_sheet():
    """Genera una hoja técnica combinada con vistas Front, Back y Assembly."""
    import trimesh
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d.art3d import Poly3DCollection

    out_sheet = os.path.join(BASE_DIR, "preview_technical_sheet.png")
    fig = plt.figure(figsize=(18, 6), dpi=160)
    fig.patch.set_facecolor("#0b0f19")

    configs = [
        (1, "FRONTAL: Bisel GC9A01 + Cámaras MQ/DHT11", STL_FRONT, "#38bdf8", 30, -50),
        (2, "TRASERA: Cuna ESP32 + Ventana USB + Stand", STL_BACK, "#fb923c", 30, -50),
        (3, "ENSAMBLE CERRADO: Tolerancia 0.3mm", STL_ASSEMBLY, "#4ade80", 25, 45)
    ]

    for idx, title, path, color, el, az in configs:
        ax = fig.add_subplot(1, 3, idx, projection="3d")
        ax.set_facecolor("#0b0f19")
        mesh = trimesh.load(path)
        faces = mesh.faces[::max(1, len(mesh.faces)//5000)]
        triangles = mesh.vertices[faces]
        poly = Poly3DCollection(triangles, alpha=0.9, linewidths=0.1)
        poly.set_facecolor(color)
        poly.set_edgecolor("#1e293b")
        ax.add_collection3d(poly)
        scale = mesh.vertices.flatten()
        ax.auto_scale_xyz(scale, scale, scale)
        ax.view_init(elev=el, azim=az)
        ax.xaxis.pane.fill = False
        ax.yaxis.pane.fill = False
        ax.zaxis.pane.fill = False
        ax.grid(False)
        ax.set_xticks([])
        ax.set_yticks([])
        ax.set_zticks([])
        ax.set_title(title, color="#e2e8f0", fontsize=10, fontweight="bold", pad=6)

    plt.suptitle("AIR GUARDIAN IOT - PLANO DE MONTAJE Y VERIFICACIÓN 3D (FRANKY SHIPWRIGHT ENGINE)",
                 color="#38bdf8", fontsize=13, fontweight="bold", y=0.98)
    plt.tight_layout()
    plt.savefig(out_sheet, facecolor=fig.get_facecolor(), edgecolor="none")
    plt.close(fig)
    print(f"🌟 [SUPER] Hoja técnica completa generada: {out_sheet}")

def render_with_blender():
    """Script para ser ejecutado con: blender -b -P render_blender_preview.py"""
    try:
        import bpy
    except ImportError:
        return False

    print("🤖 [Blender Headless Mode] Inicializando motor Cycles/EEVEE...")
    # Limpiar escena
    bpy.ops.wm.read_factory_settings(use_empty=True)
    
    # Importar STL Assembly
    bpy.ops.import_mesh.stl(filepath=STL_ASSEMBLY)
    obj = bpy.context.selected_objects[0]
    obj.name = "AirGuardianAssembly"
    bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')
    obj.location = (0, 0, 0)

    # Material de plástico técnico mate
    mat = bpy.data.materials.new(name="FDM_Dark_Plastics")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs['Base Color'].default_value = (0.05, 0.08, 0.12, 1.0) # Dark Navy Tech
        bsdf.inputs['Roughness'].default_value = 0.45
    obj.data.materials.append(mat)

    # Cámara
    cam_data = bpy.data.cameras.new(name="Camera")
    cam_obj = bpy.data.objects.new("Camera", cam_data)
    bpy.context.scene.collection.objects.link(cam_obj)
    bpy.context.scene.camera = cam_obj
    cam_obj.location = (180, -220, 160)
    cam_obj.rotation_euler = (1.0, 0.0, 0.785)

    # Luces de estudio (Key, Fill, Rim)
    def add_light(name, loc, energy, color):
        light_data = bpy.data.lights.new(name=name, type='POINT')
        light_data.energy = energy
        light_data.color = color
        light_obj = bpy.data.objects.new(name=name, object_data=light_data)
        bpy.context.scene.collection.objects.link(light_obj)
        light_obj.location = loc

    add_light("KeyLight", (150, -150, 200), 500.0, (1.0, 0.95, 0.9))
    add_light("FillLight", (-150, -100, 100), 250.0, (0.7, 0.85, 1.0))
    add_light("RimLight", (0, 200, 150), 400.0, (1.0, 0.6, 0.2))

    # Render settings
    scene = bpy.context.scene
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 1080
    scene.render.image_settings.file_format = 'PNG'
    scene.render.filepath = os.path.join(BASE_DIR, "preview_blender_render.png")

    bpy.ops.render.render(write_still=True)
    print(f"🌟 Render Blender completado: {scene.render.filepath}")
    return True

if __name__ == "__main__":
    # Si corre dentro de Blender
    if "bpy" in sys.modules:
        render_with_blender()
    else:
        render_with_trimesh_matplotlib()
