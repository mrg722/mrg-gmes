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

## Integridad de sprites enemigos — DF-011.4

La rama de integridad de sprites usa exclusivamente el atlas correspondiente a cada tipo de enemigo. Chemical Soldier, Urban Ninja, Mutant y Armored Guard no pueden sustituirse por el atlas de otro enemigo.

Todos los atlas enemigos deben ser PNG RGBA de **512x384**, organizados como **4 columnas x 3 filas de 128x128**. La aplicación rechaza un atlas con dimensiones inesperadas y utiliza un fallback procedural coherente si el arte no está disponible o no puede medirse.

Los bounds alfa de los 12 frames se miden desde la propia textura cuando se carga. El recorte se conserva junto con un pivote calculado en la zona inferior del personaje para mantener los pies anclados aunque durante un ataque sobresalgan armas o efectos. Al invertir horizontalmente el personaje, el pivote también se refleja correctamente.

La secuencia común de los atlas enemigos es:

- **0–3:** idle / locomoción
- **4–7:** ataque
- **8–9:** impacto
- **10–11:** derrota

El validador de assets exige los ocho atlas enemigos, verifica sus dimensiones, formato RGBA y que las 12 celdas contengan arte. El CI ejecuta además la compilación Release y las pruebas existentes.

> Los cuatro atlas nuevos deben ser los PNG originales transparentes del paquete de arte. Las imágenes de referencia compuestas no se consideran sustituto del asset original para una validación final de calidad.

## Modo VS / Prueba

Desde el menú principal se puede pulsar **V** para abrir un laboratorio de combate aislado. Permite seleccionar:

- Stage 1, Stage 2 o Stage 3.
- El escenario de prueba disponible dentro de cada Stage.
- Entre 1 y 4 enemigos simultáneos.
- El tipo independiente de cada enemigo entre los ocho `StreetEnemyType`.

El modo reutiliza `Player`, `StreetEnemy`, `AssetManager` y el sistema de entrada de raylib para que la comprobación de sprites y combate se haga sobre los mismos recursos de runtime. El objetivo es poder revisar rápidamente escala, pivote, animaciones, hitbox/hurtbox y carga de atlas sin recorrer toda la campaña.

### Controles del Modo VS
- **↑ / ↓:** cambiar campo.
- **← / →:** cambiar valor.
- **ENTER / J:** iniciar la prueba.
- **R:** reiniciar la prueba actual.
- **ESC:** volver a la configuración; desde la configuración, volver al menú principal.

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
- V: Modo VS / Prueba desde el menú principal

## Arquitectura
```text
src/core/       estado global de la aplicación
src/game/       jugador, enemigos, stages y game flow
src/rendering/  animator y carga/procesamiento de assets
src/audio/      eventos y audio procedural
assets/         recursos visuales
data/           datos configurables
tests/          pruebas
docs/           documentación
```

GitHub es la fuente central de verdad. Los cambios grandes se desarrollan en ramas y se integran mediante Pull Requests después de CI.
