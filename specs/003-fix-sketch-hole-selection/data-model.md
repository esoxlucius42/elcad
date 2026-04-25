# Data Model: Fix Sketch Hole Selection

## 1. Derived Loop Topology

**Purpose**: Internal sketch-analysis record used to understand which closed loops act as outer boundaries and which act as enclosed void boundaries.

**Fields**:
- `loopIndex: int` — index from `SketchPicker::findClosedLoops`
- `polygon: std::vector<QVector2D>` — flattened 2D loop polygon
- `entityIds: std::vector<quint64>` — source sketch entities participating in the loop
- `signedArea: float` — used for stable loop sizing/orientation checks
- `parentLoopIndex: std::optional<int>` — immediate containing loop, if any
- `childLoopIndices: std::vector<int>` — immediately enclosed loops
- `nestingDepth: int` — derived depth for odd/even containment reasoning

**Validation rules**:
- Loop must have at least 3 polygon points.
- Parent/child relationships must be computed from closed-loop containment, not persisted in `Document`.
- Degenerate or ambiguous loops fail profile resolution with a clear error instead of producing partial geometry.

**Relationships**:
- One completed sketch yields zero or more `Derived Loop Topology` records.
- One outer loop may own zero or more immediately enclosed child loops that act as holes.

## 2. Sketch Face Selection

**Purpose**: Existing selection request carried through `Document` for one extrude action.

**Fields**:
- `sketchId: quint64` — owning completed sketch
- `loopIndices: std::vector<int>` — requested outer-loop selections from `Document::SelectedItem::SketchArea`

**Validation rules**:
- At least one selected sketch-area loop must exist.
- All loop indices in one request must belong to the same sketch.
- Duplicate loop indices are ignored before profile resolution.
- A click inside a hole region must not create a `SketchArea` selection for the surrounding face.

**Relationships**:
- One `Sketch Face Selection` resolves to one or more `Selected Sketch Profile` records.

## 3. Selected Sketch Profile

**Purpose**: Hole-aware extrusion input representing the actual bounded face the user selected.

**Fields**:
- `sketchId: quint64`
- `loopIndex: int` — selected outer-loop index
- `sourceEntityIds: std::vector<quint64>` — outer-boundary entities
- `sourceEntities: std::vector<SketchEntity>` — outer-boundary entity copies for OCCT conversion
- `polygon: std::vector<QVector2D>` — outer boundary polygon
- `holes: std::vector<HoleBoundary>` — enclosed void boundaries attached to this selected face
- `plane: SketchPlane` — source sketch plane
- `isClosed: bool`

**Validation rules**:
- Outer boundary must resolve to valid non-construction geometry.
- Every hole boundary must be fully enclosed by the outer loop.
- Hole boundaries must not be treated as additive material for the same selected face.
- Invalid or missing source geometry rejects the whole selected profile.

**Relationships**:
- One `Sketch Face Selection` may produce many `Selected Sketch Profile` records.
- One `Selected Sketch Profile` owns zero or more `HoleBoundary` records.

## 4. HoleBoundary

**Purpose**: Inner closed loop that defines excluded material for a selected sketch face.

**Fields**:
- `holeLoopIndex: int`
- `entityIds: std::vector<quint64>`
- `entities: std::vector<SketchEntity>`
- `polygon: std::vector<QVector2D>`

**Validation rules**:
- Hole loop must be closed and non-degenerate.
- Hole loop must not intersect the selected outer boundary.
- Multiple holes inside the same selected face are all preserved.

**Relationships**:
- Many `HoleBoundary` records may belong to one `Selected Sketch Profile`.

## 5. Extrude Request / Result

**Purpose**: Existing extrude operation enriched with a hole-aware selected profile.

**Fields**:
- `params.distance: double`
- `params.mode: int` (`0 = New Body`, `1 = Add`, `2 = Remove`)
- `params.symmetric: bool`
- `profiles: std::vector<Selected Sketch Profile>`
- `result.success: bool`
- `result.errorMsg: QString`
- `result.shape or solids: OCCT output preserving inner wires`

**Validation rules**:
- `profiles` must not be empty.
- Every selected profile must resolve before any document mutation occurs.
- Face construction must preserve all hole wires before prism creation.
- Invalid geometry must stop the operation without leaving filled unintended solids behind.

**State transitions**:
- `Requested -> Resolved -> Previewed`
- `Requested -> Resolved -> Committed`
- `Requested/Resolved -> Rejected`

## 6. Resulting 3D Object

**Purpose**: The committed extruded solid corresponding to the selected bounded sketch face.

**Fields**:
- `bodyId: quint64` or updated target-body identity
- `sourceLoopIndex: int`
- `holeCount: int`
- `shape: TopoDS_Shape`

**Validation rules**:
- Material must exist only in the area inside the outer boundary and outside all hole boundaries.
- Clicking/extruding within a hole must not create solid material for the surrounding face.
- Multiple enclosed holes must remain open in both preview and final committed geometry.

**Relationships**:
- One `Selected Sketch Profile` maps to one resulting solid contribution.
- Add/Cut modes may fold multiple selected profiles into one updated target body.
