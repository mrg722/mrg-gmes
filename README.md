# District Fury

Videojuego 2D beat'em up / brawler original desarrollado en C++17 y raylib.

## Estado actual
La campaña jugable ya contiene tres etapas conectadas a través del ejecutable principal:

- **Stage 1 — Slum District:** cuatro escenarios narrativos, gatekeepers y Brakk "The Chain".
- **Stage 2 — Old Steel Yard:** Deep Line, roster químico/industrial y Grinder.
- **Stage 3 — Astra Tower:** Public Atrium, Research Floor, Executive Core y Titan-X.

La base técnica incluye un mundo horizontal, cámara 2D, combate con hitboxes/hurtboxes, combos, proyectiles, hitstop, partículas, dificultad y flujos de victoria/derrota/pausa.

## Jugabilidad actual
- Rayden Cruz con idle/walk/punch/kick/energy/dash/rage.
- Movimiento WASD en horizontal y profundidad.
- Cámara 2D desplazable con límites del escenario.
- Stage 1: Punk, Brute, Charger, Enforcer y Brakk.
- Stage 2/3: Chemical Soldier, Urban Ninja, Mutant, Armored Guard y bosses Grinder/Titan-X.
- Hitboxes/hurtboxes independientes de la posición central.
- Hitstop, screenshake, partículas, proyectiles, combo, score y rangos D→SSS.
- Bosses con fases y defensa/guard para evitar que sean simples esponjas de HP.
- Dificultad Easy/Normal/Hard en los story controllers.
- Build reproducible con CMake + raylib y pruebas automatizadas.

## Controles de PC
- WASD: movimiento
- J: punch
- K: kick
- L: energy
- Shift: dash
- Space: Rage
- ESC/P: pausa
- F1: Stage 1
- F2: Stage 2
- F3: Stage 3

## Arquitectura
```text
src/core/       estado global de la aplicación
src/game/       jugador, enemigos, stages y game flow
src/rendering/  animator y carga/procesamiento de assets
src/audio/      eventos y audio procedural
assets/         recursos visuales
data/           datos configurables
tests/          pruebas
```

GitHub es la fuente central de verdad. Los cambios grandes se desarrollan en ramas y se integran mediante Pull Requests después de CI.
