# elcad

A small but functional 3D CAD tool for Linux — sketch-based parametric modelling with STEP import/export for 3D printing.

Built with **C++17**, **Qt6**, **OpenCASCADE 7.9**, and **OpenGL 3.3**.

---

## Features (POC)

- 3D viewport with Phong-shaded bodies and edge overlay
- Infinite analytical grid
- Multiple independent bodies per scene
- **Navigation:** RMB drag = orbit · MMB drag = pan · Scroll = zoom
- **View presets:** `Num 1` Front · `Num 3` Right · `Num 7` Top · `Num 0` Isometric
- **Keyboard:** `G` toggle grid · `F` fit all · `P` perspective↔ortho · `Esc` cancel tool
- Dark Qt6 theme
- STEP import / STEP export (Phase 8)
- 2D sketcher with constraints (Phase 4)
- Extrude, boolean ops, transforms (Phases 5–7)

---

## Dependencies

| Library | Version | Notes |
|---|---|---|
| Qt6 | 6.x | Core, Gui, Widgets, OpenGL, OpenGLWidgets |
| OpenCASCADE (OCCT) | 7.9.0 | Auto-built from source if not found |
| CMake | ≥ 3.20 | Build system |
| GCC / Clang | C++17 | GCC 12+ recommended |

> **Note (Fedora Silverblue / rpm-ostree systems):** `opencascade-devel` cannot install due to a `mesa-libGL-devel` epoch conflict. elcad automatically builds OCCT 7.9.0 from source on first `cmake --build` — takes ~5–15 minutes with 16+ cores.

---

## Build

```bash
# 1. Clone
git clone <repo-url> elcad
cd elcad

# 2. Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Build (first run builds OCCT from source if not installed)
cmake --build build --parallel $(nproc)

# 4. Run
./build/bin/elcad
```

### With system OCCT (if installed)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
```

### With a custom OCCT install

```bash
cmake -B build -DOCCT_DIR=/path/to/occt-install -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
```

### After the first OCCT build

OCCT is installed to `build/occt-install/` and automatically reused on all subsequent `cmake -B build` calls — it will **not** rebuild OCCT again.

---

## Project Structure

```
elcad/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── app/            # Application class, dark theme
│   ├── ui/             # MainWindow, ViewportWidget, panels
│   ├── viewport/       # Camera, Renderer, ShaderProgram, Grid, MeshBuffer
│   ├── document/       # Document, Body, UndoStack
│   ├── sketch/         # Sketcher (Phase 4)
│   ├── tools/          # Tool classes (Phases 5–7)
│   ├── io/             # STEP import/export (Phase 8)
│   └── shaders/        # GLSL shaders (embedded via Qt resources)
└── third_party/        # Vendored dependencies (if any)
```

---

## Units

All values are in **millimetres (mm)**.
