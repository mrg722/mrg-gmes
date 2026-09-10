# District Fury — Architecture

## Main technologies
- C++17+
- raylib
- CMake

## Target structure

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

## Principles
- Keep gameplay systems modular.
- Separate input, simulation, rendering and UI responsibilities where practical.
- Keep hitbox/hurtbox and combat resolution independent from rendering.
- Prefer reusable enemy/boss systems.
- Prefer data-driven values for balance.
- Keep platform-specific code isolated so Web/Android targets can be added later.
