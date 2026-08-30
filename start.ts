#!/usr/bin/env bun
import { spawn } from "bun";

console.log("\x1b[36m%s\x1b[0m", "🚀 Iniciando Air Guardian IoT (Backend NestJS + Frontend Nuxt 3)...");

// Iniciar Backend NestJS
const backend = spawn(["bun", "run", "src/main.ts"], {
  cwd: "./backend",
  stdout: "inherit",
  stderr: "inherit",
});

// Iniciar Frontend Nuxt
const frontend = spawn(["bun", "run", "dev"], {
  cwd: "./frontend",
  stdout: "inherit",
  stderr: "inherit",
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
