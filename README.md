# Darts — rebuilt

## Files
- `Game.h` / `Game.cpp` — game mechanics: aiming physics, scoring (with labels like "Triple 20"), and the bust/win rule.
- `main.cpp` — everything else: the menu system, the full state machine, rendering, and animations.

## What's new

**Menus**
- Main Menu → Choose Players (2–4) → Choose Aim Mode → Play.
- Navigate with Up/Down, confirm with Enter, go back with Esc.

**Two aim modes**
- *Classic Free Aim* — the original wandering-dot mechanic, steered with the arrow keys, thrown with Space.
- *Two-Axis Precision* — a vertical line sweeps left↔right; Space locks your X position. Then a horizontal line sweeps up↔down; Space locks your Y position and throws the dart.

**Scoring / overflow fix**
The old code just silently refused to add the score if it would exceed 300 (so you'd hit a wall with no feedback and a stuck game). This version implements the real darts "bust" rule on a 301 game with 3 darts per visit:
- If a dart would push your total *over* 301 → **bust**: the whole visit is voided and your score reverts to what it was before the visit started.
- If a dart lands you *exactly* on 301 → you win.
- Otherwise your score updates normally and you keep throwing (up to 3 darts per visit).

**Better win / feedback messaging**
- Each dart shows a label (e.g. "Triple 20", "Bullseye", "Miss") and its score as a popup.
- End of each visit shows a clean summary panel (all three darts, visit total, new score, or a clear "BUST!" message).
- Game over screen shows a pulsing "<Player> WINS!" banner with simple animated sparkles and a final sorted scoreboard.

**Design/animation**
- Dark, styled menus with a pulsing selected option and title drop-shadow.
- A live scoreboard bar at the top during play, with the current player highlighted.
- A short impact-ring animation when a dart lands (color-coded: white for a normal hit, red for a bust, gold for a win).
- If `Dartboard.jpg` or `Arimo.ttf` aren't found next to the executable, the game falls back to a built-in vector dartboard / system font instead of just crashing, so it's always runnable.

## Building

You need SFML 2.x dev libraries.

**Option A — CMake (recommended, cross-platform)**
```
mkdir build && cd build
cmake ..
cmake --build .
```
This produces a `darts` (or `darts.exe`) binary and copies `Dartboard.jpg`/`Arimo.ttf` next to it automatically if they're present in the repo.

**Option B — direct g++ (Linux/macOS)**
```
g++ -std=c++17 Game.cpp main.cpp -o darts -lsfml-graphics -lsfml-window -lsfml-system
```

**Option C — Visual Studio (Windows)**
Create a project with `Game.h`, `Game.cpp`, `main.cpp` only (no leftover original main file), point it at an SFML 2.x install, and build.

Put `Dartboard.jpg` and `Arimo.ttf` in the same folder as the executable for the original look (optional — the game falls back to a built-in vector dartboard / system font if they're missing).

I compiled this exact code end-to-end (g++ 13 and CMake/SFML 2.6.1) with zero errors and only one harmless unused-parameter warning, so it's good to drop in as-is.
