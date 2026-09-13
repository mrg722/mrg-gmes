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

## DF-006 — Combat 2.0
- Combos encadenables reales.
- Hitbox/hurtbox por ataque y frame.
- Knockdown, launch, wall hit e invulnerabilidad.
- Proyectiles y eventos de impacto.
- VFX y audio de impacto.

## DF-007 — Stage 1: Slum District
- Calle completa con tramos, arenas y transiciones.
- Props, parallax y composición visual consistente.
- Brakk "The Chain" como boss original.
- Stage Clear, score y rango D→SSS.

## DF-008 — Stage 2: Old Steel Yard
- Completar identidad industrial/química.
- Grinder y variantes de enemigos.
- Boss y arenas específicas.

## DF-009 — Stage 3: Astra Tower
- Entorno tecnológico/corporativo.
- Enemigos avanzados.
- Titan-X como jefe final.

## DF-010 — Sistemas y release PC
- Menús completos.
- Guardado/carga.
- Dificultad.
- XP, coins, gems, level y mejoras.
- Desbloqueo de stages.
- Audio y VFX finales.
- Optimización y QA.
- Builds reproducibles Windows/Linux.

## Posterior
- WebAssembly.
- Android con joystick y botones táctiles.
