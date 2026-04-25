# Implementation Plan: Fix Multi-Shape Extrude

**Branch**: `001-fix-extrude-preview-normals` | **Date**: 2026-04-25 | **Spec**: `specs/002-fix-multi-shape-extrude/spec.md`
**Input**: Feature specification from `/specs/002-fix-multi-shape-extrude/spec.md`

## Summary

Support multi-face sketch extrusion by resolving the selected sketch areas into individual closed profiles, building one OCCT face/solid per selected profile, and committing the operation atomically so disconnected faces extrude successfully without pulling in unselected sketch shapes.

## Technical Context

**Language/Version**: C++17  
**Primary Dependencies**: Qt6, OpenCASCADE 7.9, OpenGL 3.3, spdlog  
**Storage**: In-memory `document/Document`, `Body`, and `Sketch` state  
**Testing**: Manual validation in elcad, CMake build verification, and documented regression-harness gap because the repository currently has no automated test target  
**Target Platform**: Linux desktop  
**Project Type**: Native desktop CAD application  
**Performance Goals**: Keep preview and commit responsive for sketches with 2-5 selected disconnected faces while avoiding unnecessary full-scene recomputation outside the active extrude flow  
**Constraints**: Millimetre units; `document/Document` remains the mutation owner; OCCT logic stays under `sketch/` helpers; `#ifdef ELCAD_HAVE_OCCT` guards remain intact; no autonomous commits, tags, pushes, or branch changes; use the repository's current checked-out state as source of truth  
**Scale/Scope**: Changes are limited to sketch area selection resolution, sketch-to-face conversion, extrude preview/commit handling, and validation/documentation for multi-face sketch extrusion

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- **Linux-first CAD scope**: PASS — fixes a core sketch-to-solid workflow and preserves millimetre modeling semantics.
- **Document-centered architecture**: PASS — `Document` remains the owner of committed body mutations, while UI code only resolves selection and delegates geometry work to sketch/operation helpers.
- **Native stack discipline**: PASS — stays within C++17/Qt6/OCCT/OpenGL and adds no new dependencies.
- **Verification evidence**: PASS — plan requires build verification plus explicit manual validation for multi-face and partial-selection flows; the automated-test gap is documented rather than hidden.
- **Spec Kit / git policy**: PASS — planning does not create/switch branches or create commits; Copilot guidance remains custom-agent based.

## Project Structure

### Documentation (this feature)

```text
specs/002-fix-multi-shape-extrude/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
└── tasks.md              # Created later by speckit.tasks
```

`contracts/` is intentionally omitted because this feature changes internal desktop modeling behavior and exposes no new external API, CLI schema, or service contract.

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
│   ├── SketchToWire.h
│   └── SketchToWire.cpp
├── ui/
│   ├── MainWindow.cpp
│   └── ViewportWidget.cpp
└── viewport/
    └── Renderer.cpp
```

**Structure Decision**: Implement the fix in the existing sketch/extrude path. Selection interpretation stays anchored to `Document` + `SketchPicker`, profile-to-OCCT conversion stays under `sketch/`, and `MainWindow` remains responsible only for orchestrating preview/commit and surfacing operation failures.

## Phase 0: Research Summary

- Resolve multi-face sketch extrusion by using selected `SketchArea` loop indices instead of converting the entire sketch into one wire.
- Treat each selected profile as an independent extrusion input so disconnected loops do not fail a single-wire build.
- Keep the operation atomic: validate and build all requested outputs first, then mutate `Document` only after every requested result succeeds.
- Preserve existing boolean-target behavior by applying all generated solids to a working target shape before committing one final document mutation.

## Phase 1: Design Summary

- Add selection-scoped sketch profile extraction that maps `Document::SelectedItem::SketchArea` entries to bounded sketch loops.
- Extend sketch-to-wire / extrude helpers to return multiple profile faces/solids instead of one whole-sketch result.
- Update `MainWindow` extrude preview and commit flows to handle multiple sketch-profile results, creating one body per selected face in New Body mode and all-or-nothing target-body booleans in Add/Cut modes.
- Keep unselected sketch areas untouched and preserve the existing `success` / `errorMsg` error-reporting pattern for invalid selections.

## Phase 2: Task Planning Preview

Expected implementation work for `speckit.tasks`:
1. Add selected-profile extraction and validation helpers in `sketch/`.
2. Refactor extrusion helpers to build one face/solid per selected profile.
3. Update `MainWindow` preview, commit, and undo handling for multi-result extrude.
4. Add verification updates (build/manual validation notes and any feasible regression harness work).

## Post-Design Constitution Check

- **Linux-first CAD scope**: PASS — design remains limited to sketch extrusion behavior.
- **Document-centered architecture**: PASS — no direct UI-owned geometry state is introduced; final body mutations still flow through `Document` / `Body`.
- **Native stack discipline**: PASS — OCCT helpers stay guarded and localized.
- **Verification evidence**: PASS — quickstart captures reproducible validation steps and expected review evidence.
- **Spec Kit / git policy**: PASS — no branch/commit automation added; disabled git hooks remain unexecuted.

## Complexity Tracking

No constitution violations or justified exceptions.
