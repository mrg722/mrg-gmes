# District Fury — Arquitectura

## Tecnologías principales
- C++17+
- raylib
- CMake

## Estructura objetivo

```text
src/
├── core/
├── game/
│   ├── player/
│   ├── enemies/
│   ├── bosses/
│   ├── combat/
│   ├── stages/
│   └── progression/
├── ui/
├── audio/
├── rendering/
├── input/
├── data/
└── main.cpp
```

## Principios
- Mantener el gameplay modular.
- Separar entrada, simulación, render y UI cuando sea práctico.
- Mantener hitbox/hurtbox y resolución del combate independientes del render.
- Diseñar enemigos y bosses con sistemas reutilizables.
- Mantener los parámetros de balance configurables.
- Aislar código específico de plataforma para facilitar Web y Android posteriormente.
- Evitar dependencias innecesarias.
