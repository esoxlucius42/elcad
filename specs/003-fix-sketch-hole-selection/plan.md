# Implementation Plan: Fix Sketch Hole Selection

**Branch**: `main` | **Date**: 2026-04-25 | **Spec**: `specs/003-fix-sketch-hole-selection/spec.md`
**Input**: Feature specification from `/specs/003-fix-sketch-hole-selection/spec.md`

**Planning note**: `.specify/scripts/bash/setup-plan.sh --json` reported `BRANCH=001-fix-extrude-preview-normals`, which was used only as setup context for this plan.

## Summary

Fix sketch face picking and extrusion for profiles with inner loops by deriving loop-containment relationships in `SketchPicker`, treating a selected face as one outer boundary plus enclosed hole boundaries, and building the OCCT face from that composite profile so clicks in hole regions no longer select filled material.

## Technical Context

**Language/Version**: C++17  
**Primary Dependencies**: Qt6, OpenCASCADE 7.9, OpenGL 3.3, spdlog  
**Storage**: In-memory `Document`, `Sketch`, and `Body` state  
**Testing**: Manual validation in elcad, CMake build verification, and explicit documentation of the current automated-regression gap  
**Target Platform**: Linux desktop  
**Project Type**: Native desktop CAD application  
**Performance Goals**: Preserve interactive sketch picking and extrude preview responsiveness for closed profiles with 1-3 inner holes; avoid whole-scene recomputation outside the existing extrude workflow  
**Constraints**: Millimetre units; `Document` remains the committed state owner; OCCT work stays under `sketch/`; `#ifdef ELCAD_HAVE_OCCT` guards remain intact; existing edge-picking precedence must remain intact; no autonomous commits, tags, or pushes  
**Scale/Scope**: Limited to completed-sketch area picking, selected-profile resolution, sketch-to-face conversion, extrusion behavior, and validation/documentation for profiles with holes

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- **Linux-first CAD scope**: PASS — fixes a core sketch-to-solid workflow used in normal desktop modeling.
- **Document-centered architecture**: PASS — committed geometry still flows through `Document`/`Body`; topology interpretation stays in `sketch/` helpers instead of UI-only state.
- **Native stack discipline**: PASS — stays within C++17/Qt6/OCCT/OpenGL and adds no new dependencies.
- **Verification evidence**: PASS — plan defines build verification, targeted manual scenarios, and explicit evidence capture because no automated regression harness currently covers this flow.
- **Spec Kit / git policy**: PASS — no branch or commit automation is introduced or executed; `.github/copilot-instructions.md` remains aligned with repository custom-agent guidance.

## Project Structure

### Documentation (this feature)

```text
specs/003-fix-sketch-hole-selection/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
└── tasks.md              # Created later by speckit.tasks
```

`contracts/` is intentionally omitted because this feature changes internal desktop modeling behavior and does not define a new external API, CLI contract, or service interface.

### Source Code (repository root)

```text
src/
├── sketch/
│   ├── ExtrudeOperation.cpp
│   ├── SketchPicker.h
│   ├── SketchPicker.cpp
│   ├── SketchProfiles.h
│   ├── SketchToWire.h
│   └── SketchToWire.cpp
└── ui/
    └── MainWindow.cpp
```

**Structure Decision**: Keep loop analysis, profile enrichment, and OCCT face construction in the existing `sketch/` layer. `MainWindow` should stay an orchestration surface that consumes resolved profiles and reports failures, rather than owning new topology logic.

## Phase 0: Research Summary

- The current bug comes from treating closed loops as independent filled faces with no containment semantics.
- The minimal fix is to derive loop containment from `SketchPicker::findClosedLoops`, keep selection anchored to the outer loop, and treat enclosed child loops as holes for that selected face.
- Picking must reject area hits when the click falls inside an excluded hole region while preserving existing edge/point hit priority.
- OCCT face creation must add inner wires to the selected outer wire before extrusion so the resulting solid preserves voids.

## Phase 1: Design Summary

- Add derived loop-topology metadata (parent/child or equivalent nesting depth) in `SketchPicker` for closed-loop analysis.
- Extend `SelectedSketchProfile` so one resolved face can carry one outer boundary plus zero or more hole boundaries.
- Update `SketchPicker::pick` and `resolveSelectedProfiles` so annular/profile-with-hole regions resolve to one composite profile, while clicks inside holes do not select the surrounding face.
- Update `SketchToWire::buildFaceForProfile` to create one OCCT face from an outer wire plus any hole wires, leaving `ExtrudeOperation` to extrude that composite face through the existing result/error pattern.
- Confirmed implementation touchpoints: `src/sketch/SketchProfiles.h`, `src/sketch/SketchPicker.h`, `src/sketch/SketchPicker.cpp`, `src/sketch/SketchToWire.cpp`, `src/sketch/ExtrudeOperation.cpp`, and `src/ui/MainWindow.cpp`.

## Phase 2: Task Planning Preview

Expected implementation work for `speckit.tasks`:
1. Add loop-containment helpers and hole-aware area-hit selection in `SketchPicker`.
2. Extend sketch profile data structures and profile resolution to carry enclosed hole boundaries.
3. Build OCCT faces with inner wires and verify extrusion preserves holes for preview/commit flows.
4. Validate rectangle-with-circle, multiple-hole, and hole-click negative scenarios; update build/manual evidence notes.

## Post-Design Constitution Check

- **Linux-first CAD scope**: PASS — design remains tightly focused on sketch face selection and extrusion behavior.
- **Document-centered architecture**: PASS — no new persistent UI-only model is introduced; document mutation remains centralized.
- **Native stack discipline**: PASS — OCCT-specific work stays isolated under `sketch/` and guarded where required.
- **Verification evidence**: PASS — quickstart captures reproducible manual validation plus build-output evidence for review.
- **Spec Kit / git policy**: PASS — planning stayed on `main`, skipped disabled git hooks, and updated Copilot guidance without adding autonomous VCS behavior.

## Complexity Tracking

No constitution violations or justified exceptions.
