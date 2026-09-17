# District Fury — Reglas para agentes

## Proyecto
District Fury es un videojuego 2D beat'em up/brawler original, desarrollado principalmente con C++17+, raylib y CMake.

El objetivo de desarrollo es llevar el proyecto desde el prototipo actual hasta un juego completo y mantenible, con campaña, combate profundo, personajes, enemigos, bosses, escenarios, narrativa, UI, audio, VFX, progresión, guardado, Windows `.exe` y Android `.apk`.

## Fuente de verdad
- El repositorio `mrg722/mrg-gmes` es la fuente de verdad del código.
- La rama principal de trabajo de DF-013 es `feature/DF-013-core-experience` mientras esta fase siga en desarrollo.
- No asumir que README, ROADMAP o documentos representan el estado real: comprobar siempre el código, recursos, consumidores e integración.
- Diferenciar explícitamente entre IMPLEMENTADO, PARCIAL, IMPLEMENTADO PERO NO INTEGRADO, LEGACY, PLACEHOLDER y NO IMPLEMENTADO.

## Reglas obligatorias
- District Fury es una IP original. No copiar personajes, nombres, logos, sprites, música, sonidos, escenarios, textos ni otros assets protegidos de terceros.
- Antes de cambios importantes, leer README.md, ROADMAP.md, ARCHITECTURE.md y GAME_DESIGN.md.
- Mantener una arquitectura modular. No concentrar el juego completo en `main.cpp` ni en una sola clase monolítica.
- Preferir composición, interfaces claras, sistemas pequeños, reutilizables y configuración por datos.
- Usar herencia/polimorfismo cuando aporte una diferencia real de comportamiento; no crear jerarquías de clases solo para cambiar estadísticas.
- Evitar duplicar sistemas de gameplay. Si existe una implementación nueva y una legacy para la misma responsabilidad, migrar consumidores hacia una única fuente de verdad y retirar legacy solo después de comprobar consumidores.
- No rehacer desde cero un sistema que ya exista. Primero localizarlo y determinar qué falta para integrarlo.
- Evitar dependencias innecesarias.
- Compilar después de cambios relevantes.
- Crear o actualizar tests para sistemas críticos cuando corresponda.
- Nunca afirmar que una función funciona sin haberla comprobado en el contexto correspondiente.
- Los agentes deben trabajar mediante ramas y Pull Requests cuando sea posible.
- Evitar que dos agentes modifiquen simultáneamente los mismos archivos sin coordinación.
- No realizar refactors masivos que no sean necesarios para la tarea.

## Arquitectura objetivo
Mantener progresivamente una separación clara de responsabilidades:

```text
src/
├── core/
├── game/
│   ├── player/
│   ├── combat/
│   ├── enemies/
│   ├── bosses/
│   ├── stages/
│   ├── progression/
│   ├── events/
│   └── world/
├── ui/
├── rendering/
├── audio/
├── input/
└── main.cpp
```

No mover archivos solamente por estética. Refactorizar cuando mejore reutilización, localización de errores, testabilidad o integración.

## Regla de integración DF-013
DF-013 ya contiene sistemas creados durante la evolución del proyecto que deben integrarse en el runtime real antes de duplicarlos:

- `AttackData` / `AttackDatabase`
- `CombatWorld`
- `EnemyBrain`
- FSM de enemigos
- `RageState`
- knockdown
- Dash Attack
- Finisher
- pickups/buffs
- hazards
- destructibles
- projectile/particle systems

Un sistema no se considera terminado solo porque compila o tiene un archivo propio. Debe ser el sistema que usa realmente el juego donde corresponda.

## Gameplay
El núcleo compartido debe ser la fuente común para:

- movimiento y profundidad;
- ataques y combos;
- hitboxes/hurtboxes;
- daño;
- hitstun;
- knockback;
- knockdown;
- launch;
- wall hit;
- guard;
- Guard Break;
- dash invulnerability;
- armor;
- prioridades;
- interrupciones;
- hitstop;
- screenshake;
- proyectiles;
- VFX;
- IA;
- stage flow;
- progresión;
- save data;
- UI state.

Las modalidades Historia y VS deben reutilizar estos sistemas. No crear implementaciones paralelas salvo cuando exista una razón específica de selección/configuración.

## Ataques data-driven
Los ataques deben poder configurarse sin reescribir cada Stage. Como mínimo deben poder definir:

- nombre;
- daño;
- startup;
- active frames;
- recovery;
- duración;
- alcance;
- profundidad;
- hitbox;
- knockback;
- hitstun;
- coste SP;
- ganancia de Rage;
- prioridad;
- cooldown;
- animación;
- VFX;
- SFX;
- combo;
- cancel;
- guard break.

Ataques base: PUNCH, KICK, ENERGY WAVE, DASH ATTACK, RAGE ATTACK y FINISHER.

## Enemigos
Los enemigos deben ser diferenciados por configuración y comportamiento, no por duplicación masiva.

La arquitectura debe permitir separar razonablemente:

- Enemy
- EnemyDefinition / Stats
- EnemyAI
- EnemyCombat
- EnemyAnimator
- EnemyRenderer
- EnemyStateMachine

Crear clases específicas solo cuando exista comportamiento específico real.

## Animación y arte
Priorizar la sensación de movimiento además de que cada sprite cargue correctamente.

Revisar siempre:

- timing de frames;
- transiciones;
- looping;
- animaciones de ataque;
- hit/recovery/death;
- pivotes;
- escalas;
- profundidad;
- cámara;
- interpolación adecuada;
- filtros apropiados para pixel art;
- transparencias y halos/matte blancos.

No usar escalas o velocidades arbitrarias para ocultar problemas de integración.

## Presentación
El juego debe mantener una identidad pixel-art/retro-futurista coherente.

El jugador debe distinguir claramente:

- Rayden;
- enemigos;
- bosses;
- hitboxes cuando debug esté activado;
- proyectiles;
- hazards;
- HUD;
- objetivos/eventos.

No saturar la pantalla con debug, texto o efectos innecesarios.

## Historia y campaña
El objetivo no es tener stages aislados unidos por `main.cpp`, sino una campaña coherente.

Flujo objetivo:

```text
MAIN MENU
→ NEW GAME
→ CAMPAIGN / STAGE SELECT
→ INTRO
→ STAGE
→ EVENTOS
→ ENEMIGOS
→ BOSS
→ VICTORIA
→ RECOMPENSA / SERUM / UPGRADE
→ SIGUIENTE STAGE
```

La narrativa debe ser breve y visual, reforzando el beat'em up en lugar de sustituirlo.

## Bosses
Los bosses deben reutilizar una arquitectura común pero conservar identidad propia.

Deben soportar cuando corresponda:

- phases;
- telegraphs;
- attacks;
- cooldowns;
- invulnerability;
- guard;
- special attacks;
- projectile attacks;
- transitions;
- death sequence.

No tratar un enemigo genérico como boss definitivo.

## Progresión y guardado
El sistema objetivo debe contemplar:

- XP;
- coins;
- gems;
- level;
- score;
- rango;
- recompensas;
- desbloqueos;
- upgrades;
- stages desbloqueados/completados;
- dificultad;
- configuración;
- progreso de campaña;
- SaveData versionado.

El guardado debe ser robusto frente a cambios futuros de versión.

## Escenarios
Los stages deben describir contenido y no implementar por duplicado la resolución del gameplay.

Deben poder soportar progresivamente:

- spawn groups;
- gatekeepers;
- eventos;
- hazards;
- destructibles;
- pickups;
- checkpoints;
- boss arenas;
- transiciones.

## UI
La UI debe ser un sistema reutilizable para Historia y VS.

Como mínimo debe cubrir:

- main menu;
- stage select;
- HUD;
- pause;
- options;
- stage clear;
- upgrade;
- game over;
- controls;
- feedback de save/load.

El HUD debe poder mostrar cuando corresponda:

- retrato de Rayden;
- HP;
- SP/Energía;
- Rage;
- shield;
- combo;
- score;
- boss HP;
- objetivo.

## Windows y Android
El resultado final debe contemplar ambas plataformas:

- Windows: `.exe` x64.
- Android: `.apk` y, cuando corresponda, `.aab`.

No considerar la implementación completa si solamente funciona en una modalidad de desarrollo o en una sola plataforma.

## Debug y QA
Conservar herramientas de debug durante desarrollo, separadas del build final.

Cuando corresponda, permitir inspeccionar:

- hitboxes/hurtboxes;
- estado de IA;
- SP;
- Rage;
- invulnerabilidad;
- estado de stage;
- eventos activos.

Los cambios centrales deben comprobarse en más de un consumidor. Una modificación que arregle una modalidad pero rompa otra no se considera correcta.

## Proceso de desarrollo
Antes de modificar sistemas centrales:

1. Identificar todos sus consumidores.
2. Identificar implementaciones duplicadas/legacy.
3. Determinar la fuente única de verdad.
4. Aplicar cambios incrementalmente.
5. Compilar.
6. Ejecutar pruebas relevantes.
7. Revisar Historia.
8. Revisar VS.
9. Revisar assets/animaciones/HUD cuando hayan sido afectados.

## Definition of Done
Una funcionalidad está TERMINADA cuando:

- existe;
- está integrada;
- funciona en el juego real;
- tiene presentación coherente;
- no depende de un placeholder que pretenda ser la versión final;
- no rompe funcionalidades existentes;
- la compilación pertinente funciona;
- las pruebas relevantes pasan.

"Compila" por sí solo NO significa "terminado".

## Prioridad
Cuando exista conflicto entre agregar contenido y mejorar la experiencia existente:

```text
MOVIMIENTO
↓
COMBATE
↓
IA
↓
NIVELES
↓
PROGRESIÓN
↓
PRESENTACIÓN
↓
CONTENIDO ADICIONAL
```

Priorizar profundidad y coherencia sobre cantidad artificial de contenido.

## Regla contra sobreingeniería
No convertir District Fury en un framework gigantesco.

Cada abstracción debe justificar su existencia por al menos uno de estos motivos:

- reutilización;
- testabilidad;
- reducción de duplicación;
- claridad;
- integración multiplataforma;
- facilidad de mantenimiento.

El código debe ser fácil de localizar y modificar: evitar clases monolíticas de cientos de responsabilidades mezcladas cuando una separación razonable mejore el proyecto.
