# District Fury

Videojuego 2D beat'em up / brawler original.

## Tecnologías
- C++17+
- raylib
- CMake

## Plataformas
1. PC
2. Web/WASM
3. Android

## Objetivo actual
**V0.1 — Vertical Slice**

La primera meta es tener una pequeña versión completamente jugable con:
- Rayden Cruz
- movimiento
- ataques básicos
- enemigo
- daño e hitstun
- combo
- HUD
- victoria / game over
- compilación reproducible

## Estructura

```text
src/                    Código C++
assets/                 Recursos del juego
data/                   Datos configurables
tests/                  Pruebas
docs/                   Documentación adicional
tasks/                  Tareas de agentes
reports/                Informes de builds, QA y agentes
.github/workflows/      Automatización CI
```

## Flujo de desarrollo
GitHub es la fuente central de verdad. Los cambios de agentes deben realizarse mediante ramas y Pull Requests.
