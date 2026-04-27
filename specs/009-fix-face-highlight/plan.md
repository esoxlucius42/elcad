# Implementation Plan: Fix Face Highlight

**Branch**: `009-fix-face-highlight` | **Date**: 2026-04-27 | **Spec**: `specs/009-fix-face-highlight/spec.md`
**Input**: Feature specification from `/specs/009-fix-face-highlight/spec.md`

## Summary

Correct the rendered face-selection highlight used during extrude-face workflows so it stays within the intended face boundary and remains visually consistent with the already-correct extrusion preview/result. The design now treats this as a combined renderer-and-selection-state bug: the selected face region must resolve to the intended bounded face, and face-only picks must not also trigger whole-body highlight styling.

## Technical Context

**Language/Version**: C++17  
**Primary Dependencies**: Qt6, OpenCASCADE 7.9, OpenGL 3.3, spdlog  
**Storage**: In-memory `Document` selection set plus renderer-owned mesh/picking buffers  
**Testing**: Full Release build, focused `selection_regression` and `sketch_intersection_regression`, and manual viewport validation with screenshots for the visual mismatch  
**Target Platform**: Linux desktop  
**Project Type**: Native desktop CAD application  
**Performance Goals**: Preserve interactive face picking and preview responsiveness with no noticeable slowdown in viewport highlighting or extrude preview updates  
**Constraints**: Millimetre units; `Document` remains the persistent selection owner; OCCT-dependent code stays behind `#ifdef ELCAD_HAVE_OCCT`; highlight rendering stays under `viewport/`; no autonomous commits, pushes, or tags  
**Scale/Scope**: `src/document/Document.cpp`, `src/ui/ViewportWidget.cpp`, `src/ui/MainWindow.cpp`, `src/viewport/Renderer.*`, `tests/selection/`, `tests/sketch/`, feature docs in `specs/009-fix-face-highlight/`, and `.github/copilot-instructions.md`

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Pre-Research Gate

- **PASS — Native Linux CAD Product Integrity**: Scope is limited to correcting a core extrusion-selection viewport behavior in the Linux desktop modeling workflow.
- **PASS — Document-Centered Architecture**: Persistent selection ownership stays in `Document`; the planned change targets renderer-side face resolution/drawing and the document-to-renderer highlight bridge.
- **PASS — Buildable Native Stack Discipline**: The work stays inside the established C++17/CMake/Qt6/OpenGL/OCCT stack with no new dependencies.
- **PASS — Verification Evidence Required**: The plan includes full build verification, focused regression targets, and explicit manual screenshot evidence because the defect is visual.
- **PASS — Spec Kit Alignment & Human-Controlled Delivery**: Planning stays aligned with repository custom-agent guidance and preserves the no-autonomous-commit rule.

### Post-Design Re-Check

- **PASS — Scope**: Design artifacts stay focused on the mismatch between selected-face rendering and the extrusion workflow, not unrelated modeling behavior.
- **PASS — Architecture**: The design preserves `Document` as the selection source of truth while treating `Renderer`, `ViewportWidget`, and `MainWindow` as the correction points for bounded-face resolution and highlight styling.
- **PASS — Native Stack**: No new libraries, services, or runtime processes are introduced.
- **PASS — Verification**: Artifacts define concrete manual reproduction/evidence steps plus automated Release build and focused regression commands.
- **PASS — Delivery Control**: `.github/copilot-instructions.md` points to this plan and no autonomous history-changing git action is introduced.

## Project Structure

### Documentation (this feature)

```text
specs/009-fix-face-highlight/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── spec.md
└── tasks.md
```

### Source Code (repository root)

```text
src/
├── document/
│   └── Document.cpp
├── ui/
│   ├── MainWindow.cpp
│   └── ViewportWidget.cpp
└── viewport/
    ├── Renderer.h
    └── Renderer.cpp

tests/
├── selection/
│   └── AdditiveSelectionTest.cpp
└── sketch/
    ├── SketchIntersectionFixtures.h
    ├── SketchIntersectionFixtures.cpp
    ├── SketchIntersectionSelectionTest.cpp
    └── SketchIntersectionExtrudeTest.cpp
```

**Structure Decision**: Keep the fix centered on `Renderer` face resolution and face-highlight drawing, with `ViewportWidget` and `MainWindow` aligned to the same selected-face definition and `Document` updated so explicit body highlight styling is not implied by face-only selection. Reuse existing regression targets in `tests/selection/` and `tests/sketch/` rather than adding a new interface or contract surface.

## Complexity Tracking

No constitution violations or additional complexity exceptions are required for this plan.
