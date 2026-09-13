# District Fury

Videojuego 2D beat'em up / brawler original desarrollado en C++17 y raylib.

## Estado actual
**DF-004 — Street Foundation**

El proyecto ya no se limita a una arena fija: la base actual incluye un mundo horizontal de 6000 unidades, cámara con seguimiento suave, escena urbana/industrial con capas de profundidad y parallax, recorrido por la calle, encuentros progresivos y combate con hitboxes/hurtboxes explícitas.

La fundación visual de sprites también se estabilizó: las hojas generadas se procesan por celda al cargar, se eliminan componentes ajenos y se usa filtrado POINT para evitar bleeding entre frames.

## Jugabilidad actual
- Rayden Cruz con idle/walk/punch/kick/energy/dash/rage.
- Movimiento WASD en horizontal y profundidad.
- Cámara 2D desplazable con límites del escenario.
- Calle nocturna industrial con edificios, aceras, farolas, autos, basura, cables y elementos de primer plano.
- Encuentros progresivos a medida que Rayden recorre el distrito.
- Arquetipos de enemigo: Punk, Brute, Charger y Enforcer.
- Hitboxes/hurtboxes independientes de la posición central.
- Hitstop, screenshake, partículas, números de daño y combo.
- Victoria al limpiar el distrito y alcanzar la salida; game over y reinicio.
- Build reproducible con CMake + raylib 5.5 y pruebas automatizadas.

## Controles de PC
- WASD: movimiento
- J: punch
- K: kick
- L: energy
- Shift: dash
- Space: Rage
- ESC: pausa
- R: reinicio tras victoria/derrota

## Arquitectura
```text
src/core/       estado global de la aplicación
src/game/       jugador, enemigos, escena y game flow
src/rendering/  animator y carga/procesamiento de assets
assets/         recursos visuales
 data/          datos configurables
 tests/         pruebas
```

GitHub es la fuente central de verdad. Los cambios grandes se desarrollan en ramas y se integran mediante Pull Requests después de CI.
