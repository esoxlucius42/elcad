# Copilot Instructions for elcad

elcad is a sketch-based parametric 3D CAD tool for Linux, built with C++17, Qt6, OpenCASCADE (OCCT), and OpenGL 3.3.

## Build

```bash
# Configure (first build auto-downloads and builds OCCT ~5–15 min)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --parallel $(nproc)

# Run
./build/bin/elcad
```

There are no automated tests. `MainWindow::createTestScene()` creates a sample scene for manual visual inspection. The `sphere_tess_count` utility (built alongside the main target) measures tessellation triangle counts.

## Architecture

The application follows a loose MVC structure wired together with Qt signals:

```
main.cpp → Application → MainWindow
                          ├── Document          (model: bodies, sketches, undo)
                          └── ViewportWidget    (OpenGL canvas + input)
                                ├── Camera
                                ├── Renderer    (draw calls, mesh cache, picking)
                                │    ├── MeshBuffer  (GPU mesh + BVH ray picking)
                                │    └── Gizmo       (transform handles)
                                └── Sketch mode (SketchPlane, Sketch, SketchTool, SnapEngine)
```

**Document** (`document/Document`) is the central model. It owns `Body` objects (OCCT `TopoDS_Shape` + display props), the active `Sketch`, and the `UndoStack`. All mutations go through it and fire Qt signals (`bodyAdded`, `selectionChanged`, `activeSketchChanged`, etc.) that the UI observes.

**ViewportWidget** owns the `Renderer` and switches between 3D navigation mode and sketch-editing mode. In sketch mode it activates a `SketchTool` (LineTool, CircleTool, RectTool — all derive from the abstract `SketchTool`) and a `SnapEngine`.

**Renderer** tessellates OCCT shapes into `MeshBuffer` objects (keyed by body ID) on demand. It runs four shader passes per frame: body surfaces (Phong), edge overlay, infinite grid, and the transform gizmo.

**OCCT geometry** is conditionally compiled behind `#ifdef ELCAD_HAVE_OCCT`. Static utility classes `StepIO` and `TransformOps` wrap all OCCT calls. `SketchToWire` + `ExtrudeOperation` convert 2D sketch entities into OCCT solids.

**UndoStack** uses the Command pattern. Prefer `LambdaCommand` for simple undo/redo pairs rather than writing a full `Command` subclass.

## Conventions

### Naming
- Classes/structs: `PascalCase`
- Member variables: `m_camelCase` (instance), `s_camelCase` (static)
- Functions/methods: `camelCase`
- Enum names and values: `PascalCase`
- File names match the class name exactly (`MainWindow.h` / `MainWindow.cpp`)

### Includes
`src/` is a private include root, so use paths relative to it:
```cpp
#include "document/Document.h"   // ✓
#include "src/document/Document.h" // ✗
```

### Qt
- Use `Q_OBJECT` and signals/slots for Document ↔ UI communication; avoid direct coupling.
- All UI classes inherit from an appropriate Qt widget. Dark Fusion theme is applied globally in `Application`.
- Qt resources (shaders, icons) are embedded via `resources.qrc` and accessed with `":/..."` paths.

### OCCT
- Guard OCCT-dependent code with `#ifdef ELCAD_HAVE_OCCT`.
- All geometry lives on `Document`/`Body`; don't scatter OCCT types into UI code.
- OCCT shapes are value types (`TopoDS_Shape`) — copy them freely; they share underlying topology via reference counting.

### OpenGL
- All rendering lives under `viewport/`. VAO/VBO/EBO setup belongs in `MeshBuffer`; shader uniform setting belongs in `Renderer`.
- Shaders are GLSL 3.30 core. Add new shaders as `.vert`/`.frag` files in `src/shaders/` and register them in `resources.qrc`.

### Logging
Use the project macros (wrapping spdlog) rather than `std::cout` or `qDebug()`:
```cpp
LOG_INFO("message {}", value);
LOG_DEBUG("message");
LOG_WARN("...");
LOG_ERROR("...");
```

### Error handling
Operations that can fail return a result struct with a `bool success` and `QString errorMsg` field. Check this in callers and surface errors to the user via `QMessageBox`.

### CMake
- Sources are globbed with `file(GLOB_RECURSE ...)` — adding a new `.cpp` file is picked up automatically on re-configure.
- New external dependencies should use `FetchContent` (lightweight) or the existing OCCT ExternalProject pattern (heavy, cached).

<!-- SPECKIT START -->
Current implementation plan: `specs/009-fix-face-highlight/plan.md`

For additional context about technologies to be used, project structure,
shell commands, and other important information, read the current plan.

In GitHub Copilot CLI, this repository's Spec Kit integration is exposed as
repository-level custom agents under `.github/agents/`, not as repository-defined
slash commands. Do not tell users to run `/speckit.specify`, `/speckit.plan`,
or similar commands in Copilot CLI.

When working in Copilot CLI, direct users to one of these supported entrypoints instead:
- `/agent` and then choose a `speckit.*` agent
- Mention the agent name in a natural-language prompt, for example `Use the speckit.plan agent`
- Start the CLI with `copilot --agent=speckit.plan --prompt "..."` when appropriate

The `.github/prompts/` files are part of the Spec Kit installation, but Copilot CLI
support should be described in terms of custom agents and custom instructions.
<!-- SPECKIT END -->
