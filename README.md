# drude-sim

A real-time simulation of how DC circuits work — electrons bouncing through a copper lattice, guided by an electric field, visualized using [raylib](https://www.raylib.com/).

<img width="1872" height="1009" alt="image" src="https://github.com/user-attachments/assets/75868202-7298-47fe-aa81-298006198456" />

---

## Motivation

I haven't found an introduction to DC circuits course that answers the question: **why does current flow uniformly through a wire regardless of its shape?**

It shouldn't matter what the shape of the wire is right? What does the battery care, it just needs to pump out electrons.

This simulation is an attempt to visually demonstrate the Drude model: electrons in a conductor move under the influence of a sustained electric field, colliding with the fixed copper ion lattice and gradually drifting in a net direction.

---

## Note that

Simulation only works well if your display is 1920 x 1080 - this is due to dire yet interesting personal circumstances.

Despite being towered in length by `circuit.c`, the LaTeX writeup (`main.tex`) is the main file in this project, it actually explains what happens.

Forgive me for the code organisation.

## Dependencies

- [raylib](https://www.raylib.com/) — for rendering (circles, rectangles, window management)
- A C compiler (GCC recommended)

### Installing raylib

**Linux (Debian/Ubuntu):**
```bash
sudo apt install libraylib-dev
```

**macOS (Homebrew):**
```bash
brew install raylib
```

**Windows:** Download from [raylib releases](https://github.com/raysan5/raylib/releases) and link manually.

---

## Building

```bash
gcc circuit.c -o drude-sim -lraylib -lm
```

Then run:
```bash
./drude-sim
```

The simulation opens a **1920×1080** fullscreen-style window. Close it by pressing `Esc` or the window's close button.

---

## How it works (briefly)

A lattice of stationary copper atoms is drawn, electrons are spawned at random positions 'inside a battery' (which is a rectangle). They are all initialized with some velocity.

There is a constant electric field in each branch of the wire, collisions with copper atoms (which are determined not using squared distance but the alpha max beta min algorithm for speed) have energy loss whereas collisions with the wire boundary are elastic.

---

## Configuration (in `circuit.c`)

| Macro | Default | Meaning |
|---|---|---|
| `N_E` | 4000 | Number of electrons |
| `V` | 10 | Initial electron speed |
| `R_CU` | 6 | Radius of copper atoms (px) |
| `R_E` | 2 | Radius of electrons (px) |
| `GAP` | 8 | Spacing between copper atoms |
| `THICCNESS` | 120 | Wire thickness (px) |
| `E_field` | 50 | Electric field strength |
| `energy_loss_frac` | 0.5 | Really this is momentum loss but yeah |

---

## Files

```
drude-sim/
├── circuit.c      # Main simulation source
├── main.tex       # LaTeX write-up explaining the physics
├── 1.png          # Screenshot
├── 2.png          # Screenshot
└── README.md
```
