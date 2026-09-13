#!/usr/bin/env bun
import { spawn, $ } from "bun";

// Liberación proactiva de puertos 3000 y 3002 para evitar errores EADDRINUSE
try {
  await $`fuser -k 3000/tcp 3002/tcp 2>/dev/null || true`.quiet();
  await new Promise((r) => setTimeout(r, 400));
} catch {}

const forceDev = process.argv.includes("--dev");
const hasProdBuild = await Bun.file("./frontend/.output/server/index.mjs").exists();
const useProd = hasProdBuild && !forceDev;

console.log("\x1b[36m%s\x1b[0m", `🚀 Iniciando Air Guardian IoT (${useProd ? "Modo Rápido Compilado" : "Modo Desarrollo HMR"})...`);

const bunPath = process.execPath || "bun";

// Iniciar Backend NestJS
const backend = spawn([bunPath, "run", "src/main.ts"], {
  cwd: "./backend",
  stdout: "inherit",
  stderr: "inherit",
  env: { ...process.env, PATH: `${process.env.PATH}:/home/juankio/.bun/bin` }
});

// Iniciar Frontend Nuxt
const frontend = spawn(
  useProd ? ["node", ".output/server/index.mjs"] : [bunPath, "run", "dev"],
  {
    cwd: "./frontend",
    stdout: "inherit",
    stderr: "inherit",
    env: { ...process.env, PORT: "3000", PATH: `${process.env.PATH}:/home/juankio/.bun/bin` }
  }
);

// Manejo de salida limpia (Ctrl+C)
function cleanup() {
  console.log("\n\x1b[33m%s\x1b[0m", "🛑 Deteniendo servidores...");
  backend.kill();
  frontend.kill();
  process.exit(0);
}

process.on("SIGINT", cleanup);
process.on("SIGTERM", cleanup);
