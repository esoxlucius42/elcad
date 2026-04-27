# Implementation Plan: Double-Sided Face Rendering

**Branch**: `[005-fix-sketch-intersections]` | **Date**: 2026-04-27 | **Spec**: `specs/006-double-sided-face-rendering/spec.md`
**Input**: Feature specification from `/specs/006-double-sided-face-rendering/spec.md`

## Summary

Restore reliable viewport visibility for extrude-generated faces by treating body-surface shading as two-sided in the shared OpenGL Phong pass, while keeping OCCT extrusion geometry, mesh caching, and depth-tested occlusion intact. The implementation should stay inside the existing `viewport/` and shader seams, with only minimal `MeshBuffer` follow-up if shader-only verification exposes a remaining orientation-specific normal issue.

## Technical Context

**Language/Version**: C++17  
**Primary Dependencies**: Qt6, OpenCASCADE 7.9, OpenGL 3.3, spdlog  
**Storage**: In-memory `Document` / `Body` model plus cached OCCT triangulation in `viewport/MeshBuffer`  
**Testing**: Full Release build (`cmake -B build -DCMAKE_BUILD_TYPE=Release`, `cmake --build build --parallel $(nproc)`) plus manual viewport validation from `specs/006-double-sided-face-rendering/quickstart.md`; no dedicated automated viewport regression currently exists, so any focused automated coverage is optional only if practical  
**Target Platform**: Linux desktop  
**Project Type**: Native desktop CAD application  
**Performance Goals**: Preserve interactive body rendering and orbiting; avoid geometry duplication or multi-pass surface rendering unless single-pass two-sided shading proves insufficient  
**Constraints**: Millimetre units; rendering changes stay under `src/viewport/` and `src/shaders/`; `document/Document` remains the model owner; OCCT-specific code stays behind `#ifdef ELCAD_HAVE_OCCT`; preserve depth-based occlusion; do not create/switch branches or write git history; repository guidance prefers `main`, but the repository is already checked out on `005-fix-sketch-intersections` and the user explicitly prohibited branch switching, so this plan proceeds in place without any git branch operation  
**Scale/Scope**: `src/shaders/phong.vert`, `src/shaders/phong.frag`, `src/viewport/Renderer.cpp`, `src/viewport/Renderer.h`, `src/viewport/MeshBuffer.cpp` for any narrowly scoped follow-up, and feature docs in `specs/006-double-sided-face-rendering/`

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Pre-Research Gate

- **PASS — Native Linux CAD Product Integrity**: Scope is limited to viewport visibility of extruded CAD surfaces; no unrelated product area or unit-system change is introduced.
- **PASS — Document-Centered Architecture**: Persistent model ownership remains in `document/Document`; the planned fix stays in shader/render code and does not move geometry ownership into UI code.
- **PASS — Buildable Native Stack Discipline**: Work remains inside the existing C++17/CMake/Qt6/OpenGL/OCCT stack with current include and guard patterns; no new dependencies are planned.
- **PASS — Verification Evidence Required**: The plan defines concrete build evidence and reviewer-run viewport scenarios for the known repro, orbit inspection, and hidden-surface regression checks.
- **PASS — Spec Kit Alignment & Human-Controlled Delivery**: Git auto-commit remains disabled, no branch creation/switching is performed, and `.github/copilot-instructions.md` is updated to the new plan path. The repository prefers `main`, but because the checkout was already on `005-fix-sketch-intersections` and the user forbade switching, remaining in place is the least-conflicting option.

### Post-Design Re-Check

- **PASS — Scope**: `research.md`, `data-model.md`, and `quickstart.md` stay focused on double-sided rendering of extruded faces and the evidence required to review it.
- **PASS — Architecture**: The design keeps the change in the shared viewport shading path, with `MeshBuffer` only as a constrained fallback seam if shading alone does not resolve the bug.
- **PASS — Native Stack**: Design artifacts require no new libraries, services, build tools, or non-native runtime components.
- **PASS — Verification**: Design artifacts pair explicit Release build steps with manual validation for front-side, back-side, mixed-orientation, and occlusion scenarios.
- **PASS — Delivery Control**: `copilot-instructions.md` now references `specs/006-double-sided-face-rendering/plan.md`, and no autonomous commit, tag, push, branch creation, or branch switching action was taken.

## Project Structure

### Documentation (this feature)

```text
specs/006-double-sided-face-rendering/
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
├── shaders/
│   ├── phong.vert
│   └── phong.frag
├── ui/
│   └── MainWindow.cpp
└── viewport/
    ├── MeshBuffer.cpp
    ├── MeshBuffer.h
    ├── Renderer.cpp
    └── Renderer.h
```

**Structure Decision**: Implement the feature in the existing shared surface-rendering path so committed bodies and preview meshes benefit from the same two-sided visibility behavior. `contracts/` is intentionally omitted because this feature changes internal desktop rendering behavior and does not add a reusable external API, CLI, or protocol surface worth documenting as a contract.

## Complexity Tracking

No constitution violations or extra complexity exceptions are required for this plan.
