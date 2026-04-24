# Research: Fix Extrude Preview Normals

## Context

The feature targets a viewport-only defect: during live extrusion preview, users see hidden back-facing sides through the ghosted volume instead of the camera-facing top and side surfaces they need to judge the operation.

## Decision 1: Treat the defect as a preview render-state problem, not an extrusion-solid generation problem

**Decision**: Keep the existing `MainWindow` → `ExtrudeOperation` → `Renderer::setPreviewShape()` shape-generation flow and focus implementation work on how the preview mesh is rendered.

**Rationale**: The preview path already uses the same OCCT-generated shape that would be committed if the user accepts the tool. In `Renderer::render()`, the preview pass explicitly renders a translucent mesh with depth writes disabled and back-face culling disabled, which is consistent with the reported symptom of hidden sides showing through the volume.

**Alternatives considered**:
- Rework extrusion direction or face-normal generation in `ExtrudeOperation`: rejected because the committed solid pipeline is not described as broken and the issue is specifically limited to the preview appearance.
- Recompute mesh normals differently in `MeshBuffer`: rejected as the first-line fix because incorrect two-sided translucent state is a simpler and more direct explanation of the visible defect.

## Decision 2: Preserve translucent previewing, but restore face visibility rules that match the camera view

**Decision**: Keep the preview ghost translucent, but design the renderer update so preview drawing respects front/back visibility (for example, by culling hidden back faces while continuing to depth-test against the scene).

**Rationale**: Users still need a light-weight ghost preview instead of a fully opaque committed-looking solid, but they must not see interior or occluded back faces blend through the preview. Matching the body-rendering visibility expectations is the least surprising behavior and directly supports FR-001 through FR-004.

**Alternatives considered**:
- Make the preview fully opaque: rejected because it changes the interaction style and may obscure surrounding geometry more than the current workflow expects.
- Increase alpha or tweak lighting only: rejected because lighting changes alone do not stop back faces from appearing through the translucent volume.
- Add a second special-purpose preview shader: rejected initially because existing shader and mesh paths should be sufficient once render state is corrected.

## Decision 3: Use manual validation evidence for this feature and document automated-test limits explicitly

**Decision**: Plan for release-build verification plus repeatable manual viewport inspection steps, screenshots/recordings, and reviewer notes instead of promising a new automated rendering regression suite.

**Rationale**: The repository currently documents manual validation as the normal path, and the active defect is a camera/view-dependent rendering issue without an existing snapshot or headless viewport harness. The planning artifacts should therefore make manual evidence concrete rather than leave validation vague.

**Alternatives considered**:
- Add screenshot-based rendering tests now: rejected because no current test harness or precedent is documented for reliable automated viewport image comparison.
- Rely on code review without visual evidence: rejected because the constitution requires reviewable verification evidence, especially for rendering behavior.

## Resolved Unknowns

- **Unknown**: Is the defect more likely in geometry generation or preview rendering?  
  **Resolution**: Preview rendering state is the primary suspect.
- **Unknown**: Should the fix introduce a new rendering path?  
  **Resolution**: No; refine the existing preview pass first.
- **Unknown**: What validation approach is practical in this repository?  
  **Resolution**: Manual viewport validation with build evidence and captured artifacts.
