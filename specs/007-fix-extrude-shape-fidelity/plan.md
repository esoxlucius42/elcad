# Implementation Plan: Fix Extrude Shape Fidelity

**Branch**: `main` | **Date**: 2026-04-27 | **Spec**: `specs/007-fix-extrude-shape-fidelity/spec.md`
**Input**: Feature specification from `/specs/007-fix-extrude-shape-fidelity/spec.md`

## Summary

Restore one-to-one shape fidelity between each selected sketch face and the solid created by the extrude workflow. The design keeps persistent model ownership in `document/Document` while focusing the fix on the sketch-domain path that resolves selected bounded regions into `SelectedSketchProfile` records, converts those profiles into OCCT faces with preserved arc direction and hole boundaries, and extrudes them without cross-profile geometry corruption. Verification should extend the existing `sketch_intersection_regression` harness with curved-boundary and hole-preservation cases, plus manual screenshot-driven validation of the known distorted-result repro.

## Technical Context

**Language/Version**: C++17  
**Primary Dependencies**: Qt6, OpenCASCADE 7.9, OpenGL 3.3, spdlog  
**Storage**: In-memory `Document` / `Sketch` model; transient `SelectedSketchProfile` extrusion inputs; OCCT faces and solids for face-building/extrude results  
**Testing**: Full Release build (`cmake -B build -DCMAKE_BUILD_TYPE=Release`, `cmake --build build --parallel $(nproc)`), focused `sketch_intersection_regression` coverage (`cmake --build build --target sketch_intersection_regression --parallel $(nproc)`, `ctest --test-dir build --output-on-failure -R sketch-intersection-regression`), and manual desktop validation from `specs/007-fix-extrude-shape-fidelity/quickstart.md`  
**Target Platform**: Linux desktop  
**Project Type**: Native desktop CAD application  
**Performance Goals**: Keep sketch-face selection, extrusion setup, and result generation interactive for typical sketches; preserve existing single-face and multi-face extrusion responsiveness while adding no extra render passes or heavyweight topology scans beyond the existing sketch-region pipeline  
**Constraints**: Millimetre units; `document/Document` remains the model owner; selection/profile fidelity stays in `src/sketch/`; OCCT-specific work remains behind `#ifdef ELCAD_HAVE_OCCT`; preserve internal holes/voids when present; avoid autonomous commits, pushes, or tags
**Scale/Scope**: `src/sketch/SketchPicker.cpp`, `src/sketch/SketchProfiles.h`, `src/sketch/SketchToWire.cpp`, `src/sketch/ExtrudeOperation.cpp`, any narrowly scoped UI orchestration in `src/ui/MainWindow.cpp` / `src/ui/ExtrudeDialog.cpp` if preview or commit wiring needs alignment, `tests/sketch/`, `tests/CMakeLists.txt`, feature docs in `specs/007-fix-extrude-shape-fidelity/`, and `.github/copilot-instructions.md`

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Pre-Research Gate

- **PASS — Native Linux CAD Product Integrity**: Scope is limited to the desktop CAD extrusion workflow and fixes a modeling correctness bug where created solids diverge from the selected sketch face.
- **PASS — Document-Centered Architecture**: Persistent geometry ownership remains with `document/Document`; the planned changes stay in sketch profile resolution, face construction, extrusion utilities, and narrow UI orchestration rather than moving modeling logic into widgets or rendering code.
- **PASS — Buildable Native Stack Discipline**: Work remains in the established C++17/CMake/Qt6/OpenGL/OCCT stack with existing include and `ELCAD_HAVE_OCCT` patterns; no new dependencies are planned.
- **PASS — Verification Evidence Required**: The plan defines concrete build, regression-test, and manual screenshot-driven validation for curved boundaries, internal voids, multi-face selections, and the known repro scene.


### Post-Design Re-Check

- **PASS — Scope**: `research.md`, `data-model.md`, and `quickstart.md` stay focused on sketch-face boundary fidelity, hole preservation, and extrusion correctness rather than unrelated rendering or import/export work.
- **PASS — Architecture**: The design preserves the `SketchPicker` → `SelectedSketchProfile` → `SketchToWire` → `ExtrudeOperation` seam and keeps UI code as an orchestrator instead of a geometry owner.
- **PASS — Native Stack**: Design artifacts require no new libraries, services, or external runtimes.
- **PASS — Verification**: Design artifacts pair the existing regression harness with explicit reviewer-run desktop scenarios for the screenshot repro, curved faces, holes, and multi-face extrusions.
- **PASS — Delivery Control**: `copilot-instructions.md` will point to `specs/007-fix-extrude-shape-fidelity/plan.md`, and no autonomous commit, push, or tag action is introduced.

## Project Structure

### Documentation (this feature)

```text
specs/007-fix-extrude-shape-fidelity/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── spec.md
└── checklists/
```

### Source Code (repository root)

```text
src/
├── document/
│   ├── Document.h
│   └── Document.cpp
├── sketch/
│   ├── ExtrudeOperation.h
│   ├── ExtrudeOperation.cpp
│   ├── SketchPicker.h
│   ├── SketchPicker.cpp
│   ├── SketchProfiles.h
│   ├── SketchToWire.h
│   └── SketchToWire.cpp
├── ui/
│   ├── ExtrudeDialog.cpp
│   └── MainWindow.cpp
└── viewport/
    └── Renderer.cpp

tests/
├── CMakeLists.txt
└── sketch/
    ├── SketchIntersectionFixtures.h
    ├── SketchIntersectionFixtures.cpp
    ├── SketchIntersectionExtrudeTest.cpp
    ├── SketchIntersectionRegressionMain.cpp
    └── ...
```

**Structure Decision**: Keep selection fidelity, loop interpretation, and profile assembly under `src/sketch/`, keep extrusion result creation in `ExtrudeOperation`, and only touch UI orchestration if the current preview/commit path bypasses the resolved profile data. Extend the existing `tests/sketch/` regression harness rather than introducing a separate test framework. `contracts/` is intentionally omitted because this feature changes internal desktop CAD behavior and does not expose a standalone API, CLI schema, or protocol worth documenting as a contract.

## Complexity Tracking

No constitution violations or extra complexity exceptions are required for this plan.
