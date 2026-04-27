# Implementation Plan: Fix Sketch Intersections

**Branch**: `[005-fix-sketch-intersections]` | **Date**: 2026-04-26 | **Spec**: `specs/005-fix-sketch-intersections/spec.md`
**Input**: Feature specification from `/specs/005-fix-sketch-intersections/spec.md`

## Summary

Fix completed-sketch region derivation so every bounded planar region formed by supported sketch entities becomes independently selectable and extrudable as a closed profile. The design keeps state ownership in `document/Document` while localizing geometry work to `SketchPicker`, `SketchProfiles`, `SketchToWire`, and `ExtrudeOperation`, with rendering/UI updates and dedicated regression coverage for overlap selection, extrusion, and refresh behavior.

## Technical Context

**Language/Version**: C++17  
**Primary Dependencies**: Qt6, OpenCASCADE 7.9, OpenGL 3.3, spdlog  
**Storage**: In-memory `Document` / `Sketch` model; OCCT shapes for extrusion results where available  
**Testing**: `sketch_intersection_regression` target + `ctest --test-dir build --output-on-failure -R sketch-intersection-regression`, full Release build, and manual desktop validation from `specs/005-fix-sketch-intersections/quickstart.md`  
**Target Platform**: Linux desktop  
**Project Type**: Native desktop CAD application  
**Performance Goals**: Keep completed-sketch picking, highlighting, and extrusion setup interactive for common sketches; preserve existing single-shape selection/extrusion behavior  
**Constraints**: Millimetre units; `document/Document` remains the mutation owner; UI updates flow through Qt signals/slots; OCCT-specific behavior stays behind `#ifdef ELCAD_HAVE_OCCT`; no autonomous commits, pushes, or tags; in-scope geometry is any supported sketch entity combination that forms a bounded planar region, including open/partial curves when they participate in a closed boundary  
**Scale/Scope**: `src/sketch/`, `src/document/`, `src/ui/`, `src/viewport/`, `tests/sketch/`, `CMakeLists.txt`, and feature documentation for bounded-region selection, extrusion validity, and sketch-refresh regressions

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Pre-Research Gate

- **PASS — Native Linux CAD Product Integrity**: Scope is limited to sketch-region selection/extrusion behavior in the Linux desktop CAD workflow; units remain millimetres and no unrelated UI/platform work is introduced.
- **PASS — Document-Centered Architecture**: Document ownership stays with `document/Document`; region derivation/profile resolution remains in sketch-domain code; rendering/highlight work stays under `viewport/` and sketch rendering.
- **PASS — Buildable Native Stack Discipline**: No new dependencies are planned; work remains in C++17/CMake/Qt6/OpenGL/OCCT patterns with `ELCAD_HAVE_OCCT` guards retained.
- **PASS — Verification Evidence Required**: Automated regression coverage already exists for selection, extrusion, and refresh flows, and `quickstart.md` defines manual validation/evidence capture for overlap, edit-refresh, tangent, open, and duplicate-boundary cases.
- **PASS — Spec Kit Alignment & Human-Controlled Delivery**: Planning uses the repository's custom-agent guidance, keeps git auto-commit disabled, and does not write commits, pushes, or tags autonomously.

### Post-Design Re-Check

- **PASS — Scope**: `research.md`, `data-model.md`, and `quickstart.md` stay focused on bounded sketch-region detection, selection, extrusion, and refresh behavior.
- **PASS — Architecture**: The design resolves bounded regions into selection/profile records without moving persistent geometry ownership into UI code.
- **PASS — Native Stack**: Design artifacts require no new libraries, services, or non-native runtime components.
- **PASS — Verification**: Design artifacts pair automated regression commands with explicit reviewer-run desktop validation steps.
- **PASS — Delivery Control**: `copilot-instructions.md` continues to point at `specs/005-fix-sketch-intersections/plan.md` and keeps Spec Kit git auto-commit disabled.

## Project Structure

### Documentation (this feature)

```text
specs/005-fix-sketch-intersections/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── spec.md
├── tasks.md
└── checklists/
```

### Source Code (repository root)

```text
CMakeLists.txt
README.md

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
│   ├── SketchRenderer.cpp
│   ├── SketchToWire.h
│   └── SketchToWire.cpp
├── ui/
│   └── MainWindow.cpp
└── viewport/
    └── Renderer.cpp

tests/
├── CMakeLists.txt
└── sketch/
    ├── SketchIntersectionFixtures.h
    ├── SketchIntersectionFixtures.cpp
    ├── SketchIntersectionSelectionTest.cpp
    ├── SketchIntersectionExtrudeTest.cpp
    ├── SketchIntersectionRefreshTest.cpp
    └── SketchIntersectionRegressionMain.cpp
```

**Structure Decision**: Keep geometry derivation and profile construction under `src/sketch/`, keep selection ownership in `src/document/`, and limit UI/render changes to consuming resolved bounded-region indices. `contracts/` is intentionally omitted because this feature changes internal desktop CAD behavior and does not expose a standalone external API, CLI, or protocol worth documenting as a contract.

## Complexity Tracking

No constitution violations or extra complexity exceptions were required for this plan.
