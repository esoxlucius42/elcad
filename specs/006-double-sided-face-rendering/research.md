# Research: Double-Sided Face Rendering

## Decision 1: Treat the bug as a shared viewport-shading problem, not an extrusion-topology problem

**Decision**: Keep `ExtrudeOperation` and `SketchToWire` focused on producing valid OCCT geometry, and plan the fix in the shared body-surface rendering path used by extruded solids.

**Rationale**: `Renderer.cpp` already disables face culling for opaque bodies, and `MeshBuffer.cpp` already normalizes triangle winding for reversed OCCT faces before upload. That makes the reported symptom more consistent with a visibility/shading issue in the viewport pass than with a missing topological face in the extrusion result.

**Alternatives considered**:
- Rework extrusion face construction: rejected because the specification assumes geometry creation is already correct and only viewport visibility is broken.
- Add document-level metadata for "extruded" versus "non-extruded" faces: rejected because it expands model scope for a rendering-only bug.

## Decision 2: Use fragment-side two-sided lighting in the Phong shader

**Decision**: Update the Phong surface shader so each fragment uses a camera-facing shading normal (for example by flipping the interpolated normal toward the viewer with `faceforward` or an equivalent check) while leaving the existing single draw pass and depth test in place.

**Rationale**: A shader-side normal flip is the smallest change that makes the same surface readable from front and back without duplicating geometry, adding a second opaque pass, or weakening depth-based occlusion. It also applies consistently to committed body meshes and preview meshes because both already run through the same Phong shader.

**Alternatives considered**:
- Two opaque passes with front-face toggles: rejected as a higher-cost fallback that doubles draw work and state changes.
- Duplicate triangles with inverted normals: rejected because it increases mesh size and complicates picking/selection expectations for little benefit.

## Decision 3: Keep `MeshBuffer` winding, face ordinals, and picking stable unless verification proves a remaining normal-generation defect

**Decision**: Preserve the current `MeshBuffer` triangle upload and picking contracts as the default plan. Only revisit per-face normal generation if shader verification shows a remaining face that still renders incorrectly after the two-sided lighting change.

**Rationale**: `MeshBuffer` currently feeds both rendering and ray-picking/BVH data, so changing its winding or index semantics carries more regression risk than a shader-only change. Constraining any mesh-level follow-up keeps the initial implementation focused and easier to validate.

**Alternatives considered**:
- Immediately negate normals or remove winding swaps for reversed faces: rejected because the correct fix is not yet proven and would change a broader rendering/picking seam.
- Add per-triangle orientation flags throughout the mesh pipeline: rejected as unnecessary complexity for a narrowly scoped rendering fix.

## Decision 4: Require build evidence plus manual viewport evidence, and omit contracts

**Decision**: Validation for this feature will require a successful Release build and explicit manual viewport evidence captured from the repro extrusion, back-side orbit inspection, unaffected comparison scenes, and hidden-surface checks. No `contracts/` artifact will be created.

**Rationale**: The repository currently lacks a dedicated automated viewport regression path for this defect, so the constitution requires strong manual evidence instead. A contract document would be artificial because this feature does not expose a new external interface.

**Alternatives considered**:
- Depend on manual notes without build evidence: rejected because reviewers still need a reproducible build result.
- Create a UI contract document anyway: rejected because it would duplicate the spec without defining a real protocol or public interface.
