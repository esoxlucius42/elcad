# Data Model: Fix Extrude Preview Normals

## Overview

This feature does not add persisted document data or file-format changes. The relevant model is the temporary runtime state that connects extrude input, derived preview geometry, and camera-dependent visibility in the viewport.

## Entities

### 1. ExtrudePreviewSession

**Purpose**: Represents one active live-preview cycle while the extrude tool is open.

**Fields**:
- `sourceKind`: `SketchFace` or `BodyFace`
- `sourceReference`: sketch identifier or `(bodyId, triangleIndex/face selection)` used to rebuild the source face
- `distanceMm`: current preview depth in millimetres
- `mode`: `NewBody`, `BooleanAdd`, or `BooleanCut`
- `symmetric`: whether the preview is mirrored around the source plane
- `targetBodyId` (optional): selected body receiving boolean preview operations
- `previewShape`: derived OCCT shape passed to `Renderer::setPreviewShape()`
- `status`: `Inactive`, `Active`, `Cancelled`, or `Accepted`

**Validation rules**:
- `sourceReference` must resolve to a valid sketch face or body face before preview generation.
- `distanceMm` changes must trigger preview recomputation without mutating persistent document state.
- `previewShape` must be cleared when status becomes `Cancelled` or the tool exits.

### 2. PreviewMesh

**Purpose**: GPU-ready representation of the temporary preview shape.

**Fields**:
- `shape`: OCCT `TopoDS_Shape` used as the mesh source
- `triangleWinding`: face orientation inherited from OCCT triangulation / `MeshBuffer` build logic
- `vertexNormals`: averaged per-node normals generated during mesh build
- `alpha`: ghost opacity used for preview rendering
- `visibilityRule`: render only the faces that should remain visible from the active camera viewpoint

**Validation rules**:
- Mesh generation must preserve triangle winding consistent with the source face orientation.
- Preview visibility must not reveal hidden back faces through the volume.
- Mesh rebuilds occur only when preview input changes.

### 3. ViewportPerspective

**Purpose**: Describes the camera state that determines which preview surfaces are visible.

**Fields**:
- `cameraPosition`
- `viewMatrix`
- `projectionMatrix`
- `navigationMode`: orbit, pan, zoom, preset view, or idle

**Validation rules**:
- Camera movement must not require regenerating the extrude solid unless preview parameters changed.
- Camera-facing surfaces should remain visible and hidden surfaces should remain occluded during normal navigation.

### 4. PreviewRenderState

**Purpose**: Captures the renderer-owned rules for drawing the ghost preview.

**Fields**:
- `depthTestEnabled`
- `depthWriteEnabled`
- `blendingEnabled`
- `faceCullingMode`
- `shaderProgram`: existing Phong preview shading path

**Validation rules**:
- The state must preserve a readable ghost preview without showing hidden back faces.
- Any render-state change for previews must not leak into later body, edge, or gizmo passes.

## Relationships

- `ExtrudePreviewSession` **derives** one `PreviewMesh` from the current preview shape.
- `ViewportPerspective` **controls** which surfaces of `PreviewMesh` should be visible to the user.
- `PreviewRenderState` **governs** how `PreviewMesh` is drawn each frame.
- `ExtrudePreviewSession` **references** an optional target body when boolean previewing against an existing solid.

## State Transitions

```text
Inactive
  → Active (user starts extrude tool)
Active
  → Active (user changes distance/mode or moves camera)
Active
  → Accepted (user confirms extrusion and persistent model updates)
Active
  → Cancelled (user exits tool or presses Esc)
Cancelled / Accepted
  → Inactive (preview shape and mesh cleared)
```

## Notes

- No new persistent schema, file storage, or external contracts are required.
- The primary correctness concern is view-dependent visibility, not long-term data ownership.
