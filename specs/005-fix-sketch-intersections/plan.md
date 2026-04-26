# Implementation Plan: Fix Sketch Intersections

**Branch**: `005-fix-sketch-intersections` | **Date**: 2026-04-26 | **Spec**: `specs/005-fix-sketch-intersections/spec.md`
**Input**: Feature specification from `/specs/005-fix-sketch-intersections/spec.md`

**Planning note**: The branch identifier above comes from setup-plan metadata. No branch creation, branch switching, commit, or other git history changes were performed during planning.

## Summary

Generalize completed-sketch region resolution so overlapping closed shapes produce every bounded selectable face, preserve the existing hole-aware selection behavior for nested loops, and build OCCT extrusion faces from the resolved region boundaries so each selected overlap region extrudes as a valid closed profile.

## Technical Context

**Language/Version**: C++17  
**Primary Dependencies**: Qt6, OpenCASCADE 7.9, OpenGL 3.3, spdlog  
**Storage**: In-memory `Document`, `Sketch`, selection, and `Body` state with OCCT shapes created on demand  
**Testing**: CMake build verification plus manual desktop validation for overlap selection, overlap extrusion, and sketch-edit refresh flows; no dedicated automated regression harness currently covers this workflow  
**Target Platform**: Linux desktop  
**Project Type**: Native desktop CAD application  
**Performance Goals**: Preserve interactive completed-sketch hover/pick responsiveness for common overlapping two-shape workflows and edited sketches; avoid document-wide recomputation outside the existing sketch-region analysis path  
**Constraints**: Millimetre units; `Document` remains the committed state owner; topology derivation stays in `src/sketch/`; UI continues to consume `Document::SelectedItem::SketchArea` style region references; OCCT-dependent face construction stays guarded with `#ifdef ELCAD_HAVE_OCCT`; no autonomous commits, pushes, tags, branch creation, or branch switching  
**Scale/Scope**: Completed-sketch region derivation, area picking, selected-profile resolution, extrusion face construction, and validation/documentation for overlapping closed-shape sketches

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- **Linux-first CAD scope**: PASS — this fixes a core sketch-to-solid workflow in the desktop modeling product.
- **Document-centered architecture**: PASS — committed state stays under `Document`; region/topology derivation remains in sketch helpers rather than UI-owned state.
- **Buildable native stack discipline**: PASS — the plan stays within the existing C++17/Qt6/OCCT/OpenGL stack and adds no new dependencies.
- **Verification evidence required**: PASS — the plan defines build verification plus explicit manual evidence for selection, extrusion, and sketch-edit refresh scenarios.
- **Spec Kit alignment & human-controlled delivery**: PASS — planning uses repository custom-agent guidance, performs no git history changes, and keeps repository instructions aligned with the no-autonomous-commit/no-autonomous-branch rules.

## Project Structure

### Documentation (this feature)

```text
specs/005-fix-sketch-intersections/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
└── tasks.md              # Created later by speckit.tasks
```

`contracts/` is intentionally omitted because this feature changes internal desktop CAD interaction/extrusion behavior and does not introduce a new external API, CLI surface, or service interface.

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
├── ui/
│   └── MainWindow.cpp
└── viewport/
    └── Renderer.cpp
```

**Structure Decision**: Keep derived-region analysis, selectable-profile resolution, and OCCT face construction inside the existing `src/sketch/` layer. `MainWindow` should remain an orchestration surface that consumes resolved profiles and surfaces errors, while `Renderer` continues to consume region indices for highlighting without owning new topology rules.

## Phase 0: Research Summary

- The current picker/profile path is optimized for standalone loops and nested hole containment, but it does not fully model every bounded material region created by intersecting closed shapes.
- The overlap bug should be fixed by deriving bounded sketch regions from the planar arrangement and classifying each candidate region by actual material coverage, rather than relying only on loop-depth / parent-child hole rules.
- Selection should keep using the existing sketch-area indexing flow where practical, but region indices must now map to the resolved bounded regions that remain valid after sketch edits.
- Extrusion must consume the resolved region boundary segments (plus hole boundaries where applicable) so the selected face is treated as a true closed profile in OCCT.
- `contracts/` stays omitted because the feature has no external interface; review evidence should focus on build output plus manual validation artifacts.

## Phase 1: Design Summary

- Add a derived region-topology layer in `SketchPicker` that can distinguish overlap-generated bounded regions from true void/hole exclusions.
- Update completed-sketch picking and profile resolution so all valid bounded overlap regions become selectable while preserving the prior nested-hole behavior for annular or multi-hole faces.
- Resolve each selected region to explicit boundary segments, outer boundary metadata, and optional hole boundaries in `SelectedSketchProfile`.
- Keep `SketchToWire` / `ExtrudeOperation` responsible for turning the resolved profile into one OCCT face per selected region, using hole wires only when the selected face genuinely contains excluded inner loops.
- Confirmed implementation touchpoints: `src/sketch/SketchPicker.h`, `src/sketch/SketchPicker.cpp`, `src/sketch/SketchProfiles.h`, `src/sketch/SketchToWire.cpp`, `src/sketch/ExtrudeOperation.cpp`, `src/ui/MainWindow.cpp`, and `src/viewport/Renderer.cpp` if highlight/index assumptions need alignment with the new region mapping.

## Phase 2: Task Planning Preview

Expected implementation work for `speckit.tasks`:
1. Extend sketch-region derivation so overlapping closed shapes produce stable bounded-region records with correct material/void classification.
2. Update picking and selected-profile resolution so each bounded overlap region is independently selectable and sketch edits invalidate/rebuild stale region mappings before the next selection.
3. Build OCCT faces from resolved region boundaries and verify each selected overlap region extrudes as a closed profile.
4. Validate rectangle-circle overlap, edit/remove refresh, tangent-touch negative, and duplicate/coincident-boundary regression scenarios; capture build/manual evidence.

## Post-Design Constitution Check

- **Linux-first CAD scope**: PASS — the design remains focused on sketch selection and extrusion behavior in the core CAD workflow.
- **Document-centered architecture**: PASS — no new persistent UI-owned model is introduced; `Document` remains the source of truth for committed state and selection.
- **Buildable native stack discipline**: PASS — changes stay within existing `src/sketch/`, `src/ui/`, and renderer codepaths with the current native dependency stack.
- **Verification evidence required**: PASS — quickstart captures explicit build/manual evidence requirements for overlap selection, extrusion, and refresh behavior.
- **Spec Kit alignment & human-controlled delivery**: PASS — planning stayed aligned with repository custom-agent guidance and skipped disabled git hooks plus all autonomous git history changes.

## Complexity Tracking

No constitution violations or justified exceptions.
