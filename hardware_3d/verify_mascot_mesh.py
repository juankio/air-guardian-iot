#!/usr/bin/env python3
"""
=============================================================================
 AIR GUARDIAN IOT - REPORTE DE CALIDAD Y AUDITORÍA DE MALLAS 3D (TRIMESH)
 Master Shipwright: Franky
=============================================================================
"""

import os
import trimesh
from collections import Counter

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
STL_FILES = [
    ("PIEZA FRONTAL", os.path.join(BASE_DIR, "air_guardian_mascot_front.stl")),
    ("TAPA TRASERA", os.path.join(BASE_DIR, "air_guardian_mascot_back.stl")),
    ("ENSAMBLE COMPLETO", os.path.join(BASE_DIR, "air_guardian_mascot_assembly.stl")),
]

def audit_mesh(label, file_path):
    print("=" * 70)
    print(f"🔩 AUDITORÍA DE MALLA: {label}")
    print(f"   Ruta: {file_path}")
    print("=" * 70)

    if not os.path.exists(file_path):
        print(f"❌ ERROR: El archivo no existe.")
        return False

    mesh = trimesh.load(file_path)
    
    # 1. Conteo de aristas y búsqueda de aristas no-manifold
    edge_counts = Counter([tuple(e) for e in mesh.edges_sorted])
    bad_edges = [e for e, count in edge_counts.items() if count != 2]
    
    # 2. Caras rotas o degradadas
    broken = trimesh.repair.broken_faces(mesh)
    
    # 3. Propiedades topológicas
    is_solid = mesh.is_watertight and mesh.is_winding_consistent and len(bad_edges) == 0 and len(broken) == 0

    print(f"   • Watertight (Cerrado hermético)  : {'✅ SÍ (True)' if mesh.is_watertight else '❌ NO'}")
    print(f"   • Winding Consistent (Normales)   : {'✅ SÍ (True)' if mesh.is_winding_consistent else '❌ NO'}")
    print(f"   • Sólido con Volumen Positivo     : {'✅ SÍ (True)' if mesh.is_volume else '❌ NO'}")
    print(f"   • Aristas No-Manifold             : {'✅ 0 (Cero)' if len(bad_edges) == 0 else f'❌ {len(bad_edges)}'}")
    print(f"   • Caras Rotas o Degeneradas       : {'✅ 0 (Cero)' if len(broken) == 0 else f'❌ {len(broken)}'}")
    print(f"   • Vértices                        : {len(mesh.vertices):,}")
    print(f"   • Caras Triangulares              : {len(mesh.faces):,}")
    print(f"   • Aristas Únicas                  : {len(mesh.edges_unique):,}")
    print(f"   • Característica de Euler         : {mesh.euler_number}")
    print(f"   • Volumen Neto de Material        : {mesh.volume:.2f} mm³")
    print(f"   • Dimensiones Bounding Box        : {mesh.extents[0]:.2f} x {mesh.extents[1]:.2f} x {mesh.extents[2]:.2f} mm")
    print(f"   • Centro de Gravedad              : [{mesh.center_mass[0]:.2f}, {mesh.center_mass[1]:.2f}, {mesh.center_mass[2]:.2f}]")
    print(f"   • Veredicto de Fabricación        : {'🚀 SUUUUPER LISTO PARA IMPRESIÓN FDM' if is_solid else '⚠️ REQUIERE REVISIÓN'}\n")
    return is_solid

if __name__ == "__main__":
    print("\n🦾 [FRANKY SHIPWRIGHT] INICIANDO AUDITORÍA FÍSICA DE CARCASA MASCOTA TRIANGULAR...\n")
    all_passed = True
    for label, path in STL_FILES:
        res = audit_mesh(label, path)
        all_passed = all_passed and res

    print("=" * 70)
    if all_passed:
        print("🏆 VEREDICTO GENERAL: ¡TODAS LAS PIEZAS SON 100% SÓLIDAS, WATERTIGHT Y MANIFOLD!")
    else:
        print("⚠️ VEREDICTO GENERAL: DETECTADAS ANOMALÍAS EN UNA O MÁS MALLAS.")
    print("=" * 70 + "\n")
