# District Fury — Auditoría DF-013

## Hallazgo principal

El proyecto ya contiene tres stages jugables, VS, Player, StreetEnemy, bosses, proyectiles, combos, hitstop, dificultad, Stage Clear y recursos de progresión. El siguiente riesgo de crecimiento no es la falta de contenido, sino la duplicación de lógica entre modos y stages.

## Puntos observados

### Gameplay
`Stage1StoryGame`, `Stage2Game` y `Stage3Game` resuelven por separado impactos del jugador, impactos de enemigos, proyectiles y golpes de boss. Esto dificulta mantener exactamente el mismo comportamiento en campaña y VS.

### Datos
Las oleadas y estadísticas de enemigos están parcialmente hardcodeadas en los controladores de stage. El proyecto ya define como objetivo separar stats, ataques, stages, rewards y dificultad en datos configurables.

### IA
`StreetEnemy` posee tipos diferenciados y estados de combate, pero la arquitectura actual todavía puede crecer hacia una FSM común con estados de posicionamiento, bloqueo, stun, retirada y especiales.

### Flujo
`main.cpp` mantiene el acceso de desarrollo a Stage 1/2/3 mediante F1/F2/F3 y el VS como flujo aparte. El siguiente paso es añadir una campaña unificada y mantener esos accesos como herramientas de debug.

### Presentación
El menú, HUD, Stage Clear y laboratorio ya existen. Falta convertirlos en un flujo de producto con Stage Select, tutorial, upgrades, checkpoints, eventos narrativos, recompensas persistentes y UI conectada al progreso.

## Primera implementación DF-013

La rama crea una base de datos compartida de ataques en `src/game/combat/` y pruebas automáticas asociadas. Esta capa define propiedades de combate que permitirán migrar progresivamente la lógica existente sin reescribir los tres stages de una sola vez.

Ataques iniciales definidos:

- PUNCH
- KICK
- ENERGY WAVE
- DASH ATTACK
- RAGE ATTACK
- FINISHER

## Estrategia de migración

1. Attack Data
2. Combat Core
3. Enemy AI Core
4. Stage Event System
5. Campaign Manager
6. Progression y SaveData
7. UI 2.0
8. Narrativa y transiciones
9. Hazards, destructibles y pickups
10. QA final

Cada fase debe mantener compilación, tests y compatibilidad con Stage 1, Stage 2, Stage 3 y VS.
