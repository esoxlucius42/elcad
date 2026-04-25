# Implementation Plan: Fix Extrude Preview Normals

**Branch**: `001-fix-extrude-preview-normals` | **Date**: 2026-04-24 | **Spec**: `specs/001-fix-extrude-preview-normals/spec.md`
**Input**: Feature specification from `/specs/001-fix-extrude-preview-normals/spec.md`

## Summary

Correct the live extrude preview so the ghosted volume shows camera-facing top and side surfaces instead of bleeding back faces through the shape. The implementation should stay inside the existing preview pipeline by refining viewport preview render state and preserving the current `MainWindow` → `ExtrudeOperation` → `Renderer::setPreviewShape()` flow.

## Technical Context

**Language/Version**: C++17  
**Primary Dependencies**: Qt6, OpenCASCADE 7.9, OpenGL 3.3, spdlog  
**Storage**: In-memory `document/Document` model; no new persisted data  
**Testing**: Manual viewport validation, release build verification, optional targeted regression coverage if a practical harness emerges  
**Target Platform**: Linux desktop  
**Project Type**: Native desktop CAD application  
**Performance Goals**: Preserve interactive preview updates during distance changes and camera navigation; avoid adding noticeable redraw latency to the extrude workflow  
**Constraints**: Millimetre units remain unchanged; `document/Document` stays the owner of persistent model state; OCCT-dependent preview generation remains behind existing guards; no new dependencies; no autonomous commits  
**Scale/Scope**: Localized to extrusion preview rendering for sketch-face and face-based extrude workflows, plus supporting planning/validation documents  
**Affected Components**: `src/ui/MainWindow.cpp`, `src/viewport/Renderer.{h,cpp}`, `src/viewport/MeshBuffer.cpp`, existing Phong preview shading path  
**Research Status**: All initial unknowns resolved in `specs/001-fix-extrude-preview-normals/research.md`

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Pre-Research Gate

- **Linux-first CAD scope**: PASS — change is limited to the extrusion preview in the desktop modeling workflow and does not alter units or non-CAD scope.
- **Document-centered architecture**: PASS — the defect is in preview display; planned work keeps persistent state and undo behavior under existing `Document`/operation flows and confines rendering changes to `viewport/`.
- **Native stack discipline**: PASS — work stays within C++17, Qt6, OpenGL 3.3, and OCCT-backed preview generation with no new dependencies.
- **Verification evidence**: PASS — plan defines manual validation across top/isometric/side/opposite views, plus build output and screenshot/recording evidence; automated viewport regression is documented as currently impractical.
- **Spec Kit / delivery rules**: PASS — artifacts reference repository custom agents, and this plan performs no commit, tag, or push operations.

### Post-Design Re-Check

- **Linux-first CAD scope**: PASS — `research.md`, `data-model.md`, and `quickstart.md` stay focused on extrusion preview correctness for Linux desktop CAD review.
- **Document-centered architecture**: PASS — design keeps preview generation in existing extrude flows and places visibility/culling behavior in renderer-owned preview state.
- **Native stack discipline**: PASS — no contract or dependency expansion is required; only existing renderer/UI paths are in scope.
- **Verification evidence**: PASS — `quickstart.md` provides repeatable reviewer steps and required artifacts for manual evidence.
- **Spec Kit / delivery rules**: PASS — `.github/copilot-instructions.md` now points at this plan and continues to direct Copilot CLI users to custom agents while preserving the no-autocommit rule.

## Project Structure

### Documentation (this feature)

```text
specs/001-fix-extrude-preview-normals/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/           # Not created; no external interface contract is introduced
└── tasks.md             # Phase 2 output, not created in this workflow
```

### Source Code (repository root)

```text
src/
├── sketch/
│   └── ExtrudeOperation.{h,cpp}
├── ui/
│   └── MainWindow.cpp
├── viewport/
│   ├── MeshBuffer.{h,cpp}
│   └── Renderer.{h,cpp}
└── shaders/
    └── phong.{vert,frag}

.github/
└── copilot-instructions.md
```

**Structure Decision**: Keep runtime changes centered in `src/viewport/` because the bug is a preview-visibility/render-state issue, with `src/ui/MainWindow.cpp` remaining the integration point that recomputes preview shapes. No new source directories or public interfaces are needed; planning artifacts live entirely under `specs/001-fix-extrude-preview-normals/`.

## Phase 0 Research Summary

Phase 0 resolves three planning questions:
1. The defect is most credibly caused by translucent preview rendering state rather than extrude solid generation.
2. The preferred fix is to preserve depth-tested ghost rendering while preventing back faces from blending through the preview volume.
3. Manual visual validation is the required evidence path because the repository does not currently provide a practical automated viewport-render regression harness.

## Phase 1 Design Summary

- Model the feature as a temporary `ExtrudePreviewSession` rendered from derived OCCT/mesh data with view-dependent visibility rules.
- Limit implementation changes to preview-session rendering and validation behavior; do not change persisted document entities.
- Skip `/contracts` because this feature does not expose a new API, CLI schema, file format, or other external interface.
- Provide reviewer quickstart steps for reproducing and validating the fix.

## Complexity Tracking

No constitution violations or compensating complexity are expected for this feature.
