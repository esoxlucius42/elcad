# Data Model: Fix Multi-Shape Extrude

## 1. Sketch Face Selection

**Purpose**: Represents the user-selected sketch faces that should participate in one extrude command.

**Fields**:
- `sketchId: quint64` — owning completed sketch
- `loopIndices: std::vector<int>` — selected `SketchArea` loop indices from `Document::SelectedItem`
- `selectionCount: int` — number of requested sketch faces

**Validation rules**:
- At least one selected item must be a `SketchArea`
- All selected loop indices for one batch must belong to the same sketch
- Duplicate loop indices are ignored or rejected before extrusion work starts

**Relationships**:
- One `Sketch Face Selection` resolves to one or more `Selected Sketch Profile` records

## 2. Selected Sketch Profile

**Purpose**: Internal resolved profile ready for OCCT face creation.

**Fields**:
- `loopIndex: int` — source loop index from `SketchPicker::findClosedLoops`
- `sourceEntityIds: std::vector<quint64>` — boundary sketch entities participating in the selected profile
- `plane: SketchPlane` — source sketch plane for 3D conversion
- `isClosed: bool` — whether the profile can form a valid bounded face

**Validation rules**:
- The loop index must resolve against the current sketch geometry
- The profile must remain closed and non-degenerate after extraction
- Ambiguous/shared-entity loops outside the feature scope should fail with a clear user-facing error instead of producing partial geometry

**Relationships**:
- Many `Selected Sketch Profile` records belong to one `Sketch Face Selection`
- Each profile produces zero or one extrusion solid in a single request

## 3. Multi Extrude Request

**Purpose**: Captures one user-confirmed extrude operation over one or more selected profiles.

**Fields**:
- `params.distance: double`
- `params.mode: int` (`0 = New Body`, `1 = Add`, `2 = Remove`)
- `params.symmetric: bool`
- `profiles: std::vector<Selected Sketch Profile>`
- `targetBodyId: std::optional<quint64>` — required for Add/Cut flows

**Validation rules**:
- `distance` must stay within the existing UI/tool limits
- `profiles` must not be empty
- Add/Cut mode requires a resolvable target body
- Any invalid profile rejects the whole request before document mutation

**State transitions**:
- `Requested -> Validated -> Previewed`
- `Requested -> Validated -> Committed`
- `Requested/Validated -> Rejected`

## 4. Extrude Batch Result

**Purpose**: Aggregates the geometry results before the document is mutated.

**Fields**:
- `success: bool`
- `errorMsg: QString`
- `solids: std::vector<TopoDS_Shape>` — one solid per selected profile in New Body mode
- `finalTargetShape: std::optional<TopoDS_Shape>` — populated for Add/Cut after applying all booleans to a working copy

**Validation rules**:
- All requested solids must be valid before commit
- `finalTargetShape` is only committed if every boolean step succeeds
- On failure, no bodies or target-body edits are committed

**Relationships**:
- One `Multi Extrude Request` yields one `Extrude Batch Result`

## 5. Resulting 3D Object

**Purpose**: The committed document-level output corresponding to a selected sketch face.

**Fields**:
- `bodyId: quint64`
- `sourceLoopIndex: int`
- `shape: TopoDS_Shape`
- `placement: derived from source sketch plane/profile location`

**Validation rules**:
- New Body mode creates one committed body per selected profile
- Relative position must remain aligned with the source sketch face
- Unselected faces must not create bodies

**Relationships**:
- One `Selected Sketch Profile` maps to one `Resulting 3D Object` in New Body mode
- Add/Cut modes instead fold all successful profile solids into one updated target body
