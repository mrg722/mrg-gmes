# District Fury — Hoja de ruta de producción

## DF-004 — Street Foundation ✅
- Estabilización de sprite sheets generados.
- Escala y pivote de personajes adecuados al escenario.
- Hitboxes/hurtboxes explícitos.
- Cámara 2D con seguimiento y límites.
- Escenario urbano-industrial desplazable.
- Recorrido horizontal y encuentros progresivos.
- Variantes de enemigos con estadísticas y comportamiento diferente.

## DF-005 — Character Presentation & Visual Composition 🔧
- Normalización de atlas por frame con baseline/pivote común.
- Escala visual coherente por tipo de actor.
- Escala progresiva según profundidad del lane.
- Animaciones idle/walk/attack/hit/defeat sin saltos de tamaño o posición.
- Separación estricta entre sprite bounds y hurtbox/attack box.
- Soporte para atlas RGBA limpios y fallback seguro a hojas legacy.
- Composición del escenario orientada a mostrar 2–4 enemigos sin saturar la pantalla.
- Preparación de dirección artística para fondos por capas y mayor riqueza visual.

## DF-006 — Combat 2.0 🔧
- Combos encadenables reales.
- Hitbox/hurtbox por ataque y frame.
- Knockdown, launch, wall hit e invulnerabilidad.
- Proyectiles y eventos de impacto.
- VFX y audio de impacto.

## DF-007 — Stage 1: Slum District ✅
- Cuatro escenarios narrativamente conectados: Slum District, Old Market/Canal, Steel Gate/Freight Route y Chain Yard.
- Arenas bloqueadas y desbloqueadas por limpieza de encuentros.
- Gatekeepers fuertes al final de los tres primeros escenarios.
- Brakk "The Chain" como boss final con tres fases, telegraphs, guard, ataques cuerpo a cuerpo, charge y Power Wave.
- Stage Clear, score y rango D→SSS.
- Menú, controles, pausa, Game Over y persistencia básica integrados.

## DF-008 — Stage 2: Old Steel Yard ✅
- Identidad industrial/química y Deep Line.
- Chemical Soldier, Urban Ninja, Mutant y Armored Guard.
- Grinder con tres fases y arenas específicas.
- Energy Wave, HUD, Stage Clear y flujo jugable.

## DF-009 — Stage 3: Astra Tower ✅
- Tres escenarios conectados: Public Atrium, Research Floor y Executive Core.
- Roster avanzado: Urban Ninja, Chemical Soldier, Mutant y Armored Guard.
- Gatekeepers en los dos primeros escenarios y progresión bloqueada por limpieza.
- Titan-X como boss final con tres fases, guard adaptativo, dash, proyectiles y ataques de proximidad.
- HUD de boss, telegraph visual, hitstop, partículas, score y rango D→SSS.
- Dificultad Easy/Normal/Hard con diferencias de vida y daño.
- Integración al ejecutable principal mediante F3.

## DF-010 — Sistemas y release PC 🔧
- Menús completos.
- Guardado/carga robusto.
- Dificultad con diferencias reales de presión/IA.
- XP, coins, gems, level y mejoras.
- Desbloqueo de stages.
- Audio y VFX finales.
- Optimización y QA.
- Builds reproducibles Windows/Linux.

## DF-013 — Core Experience / Campaign 1.0 🚧
- Gameplay Core compartido para Player, combate, proyectiles, VFX y estados.
- Attack Data centralizado para evitar parámetros duplicados por stage.
- IA diferenciada por arquetipo y FSM reutilizable.
- Sistema de eventos de escenario: oleadas, emboscadas, NPC, diálogo, hazards, destructibles, pickups, checkpoints y boss triggers.
- Flujo de campaña unificado y Stage Select.
- Progresión real con XP, coins, gems, level y upgrades funcionales.
- SaveData central con desbloqueos, mejoras, mejores resultados y configuración.
- Boss Core común conservando la identidad de Brakk, Grinder y Titan-X.
- UI 2.0, tutorial integrado, narrativa conectada y transiciones.
- Auditoría de AudioSystem y feedback audiovisual.
- Compatibilidad obligatoria con Stage 1, Stage 2, Stage 3 y VS.
- Tests ampliados y CI verde como criterio de integración.

## Posterior
- WebAssembly.
- Android con joystick y botones táctiles.
