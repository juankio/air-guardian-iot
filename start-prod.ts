#!/usr/bin/env bun
import { spawn, $ } from "bun";

// Liberación proactiva de puertos 3000 y 3002
try {
  await $`fuser -k 3000/tcp 3002/tcp 2>/dev/null || true`.quiet();
  await new Promise((r) => setTimeout(r, 400));
} catch {}

console.log("\x1b[36m%s\x1b[0m", "🚀 Iniciando Air Guardian IoT en Modo Ultrarrápido (Producción Compilada)...");

// 1. Iniciar Backend NestJS en puerto 3002
const backend = spawn(["bun", "run", "src/main.ts"], {
  cwd: "./backend",
  stdout: "inherit",
  stderr: "inherit",
});

// 2. Iniciar Frontend Nuxt Compilado en puerto 3000 (Carga instantánea)
const frontend = spawn(["node", ".output/server/index.mjs"], {
  cwd: "./frontend",
  stdout: "inherit",
  stderr: "inherit",
  env: { ...process.env, PORT: "3000" }
});

// Manejo de salida limpia (Ctrl+C)
function cleanup() {
  console.log("\n\x1b[33m%s\x1b[0m", "🛑 Deteniendo servidores...");
  backend.kill();
  frontend.kill();
  process.exit(0);
}

process.on("SIGINT", cleanup);
process.on("SIGTERM", cleanup);
