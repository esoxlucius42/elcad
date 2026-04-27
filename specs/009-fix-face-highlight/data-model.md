# Data Model: Fix Face Highlight

## 1. Selected Face Region

**Purpose**: Represents the bounded face region the user intends to extrude.

**Fields**:
- `bodyId: quint64` — owning body
- `seedTriangleIndex: int` — initial picked tessellation triangle
- `faceOrdinal: int` — resolved OCCT face index when available
- `selectedTriangleIndices: unique list<int>` — triangles currently treated as part of the selected face region

**Validation rules**:
- The region must remain associated with exactly one body.
- The highlighted region must not include triangles outside the intended face boundary.
- `selectedTriangleIndices` must stay consistent with the face used for preview/final extrusion.

**Relationships**:
- One `Selected Face Region` is derived from one picked body and one seed triangle.
- It informs both viewport highlighting and face-based extrusion.

## 2. Face Selection Highlight

**Purpose**: The viewport overlay that communicates the currently selected face region.

**Fields**:
- `triangleVertices: list<QVector3D>` — vertices uploaded for highlight drawing
- `bodyId: quint64`
- `visible: bool`

**Validation rules**:
- The overlay may only draw triangles belonging to the selected face region.
- The overlay must update when the selected face region changes.
- The overlay must remain stable while preview parameters change.

**Relationships**:
- One `Face Selection Highlight` is rendered from one `Selected Face Region`.
- `Renderer` owns and draws this derived state.

## 3. Body Highlight State

**Purpose**: Tracks whether a body should receive whole-body selected styling such as brightened fill and selected edge color.

**Fields**:
- `bodyId: quint64`
- `explicitBodySelection: bool`
- `wholeBodyHighlightVisible: bool`

**Validation rules**:
- Whole-body highlight may only appear when the body itself is explicitly selected.
- Face, edge, and vertex selection must not imply whole-body highlight styling.

**Relationships**:
- One `Body Highlight State` belongs to one body.
- `Document` derives it from the canonical selection set and `Renderer` consumes it.

## 4. Extrusion Preview State

**Purpose**: The transient preview of the pending extrusion for the selected face.

**Fields**:
- `sourceBodyId: quint64`
- `sourceFaceOrdinal: int`
- `previewShapeValid: bool`
- `matchesSelectedFace: bool`

**Validation rules**:
- The preview must originate from the same selected face region the user sees highlighted.
- Preview updates must not broaden the underlying selected face region.

**Relationships**:
- One `Extrusion Preview State` is generated from one `Selected Face Region`.
- `MainWindow` and `Renderer` consume it during the extrude workflow.

## State Transitions

- `No selected face -> click face -> Selected Face Region resolved from seed triangle`
- `Selected Face Region -> renderer draws Face Selection Highlight`
- `Selected Face Region -> Body Highlight State remains off unless the body itself is selected`
- `Selected Face Region -> extrude preview starts -> Extrusion Preview State generated from same face`
- `Preview parameter change -> preview redraw -> Face Selection Highlight remains aligned`
- `Extrude commit -> final solid created -> selected face, preview, and result remain consistent`
