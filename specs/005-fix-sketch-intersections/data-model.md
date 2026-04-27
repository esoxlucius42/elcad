# Data Model: Fix Sketch Intersections

## 1. Sketch Entity Contribution

**Purpose**: Existing sketch entity data that may contribute part of a bounded planar region.

**Fields**:
- `entityId: quint64`
- `type: SketchEntity::Type`
- `construction: bool`
- Geometric parameters already stored on the sketch entity

**Validation rules**:
- Construction geometry never contributes selectable sketch regions.
- Any supported entity type may contribute if the combined boundary closes a bounded planar region.
- Open or partial entities are valid contributors only when the full resolved boundary is closed.
- Editing or deleting a contributing entity invalidates previously derived region mappings.

**Relationships**:
- Many `Sketch Entity Contribution` records belong to one completed `Sketch`.
- One entity may contribute boundary segments to multiple `Derived Sketch Region` records after intersections are split.

## 2. Derived Sketch Region

**Purpose**: Internal bounded-region candidate produced from completed-sketch topology analysis.

**Fields**:
- `regionIndex: int` — analysis-local identifier used for completed-sketch selection/highlighting
- `polygon: std::vector<QVector2D>` — representative polygon for hit testing and highlighting
- `boundarySegments: std::vector<SketchBoundarySegment>` — explicit resolved outer boundary
- `sourceEntityIds: std::vector<quint64>` — contributing sketch entities
- `parentRegionIndex: std::optional<int>` — containing region when containment exists
- `childRegionIndices: std::vector<int>` — immediately enclosed regions
- `isSelectableMaterial: bool` — whether the region represents selectable material rather than a void
- `samplePoint: QVector2D` or equivalent interior probe used for classification
- `signedArea: float` or equivalent derived area metric

**Validation rules**:
- Region must be bounded and non-degenerate.
- Boundary segments must connect into a closed outer loop.
- Classification must distinguish positive material from excluded holes/voids.
- Region indices are valid only for the current sketch geometry and must refresh after edits.

**Relationships**:
- One completed `Sketch` yields zero or more `Derived Sketch Region` records.
- One `Derived Sketch Region` may resolve to one `Selected Sketch Profile`.

## 3. Sketch Face Selection

**Purpose**: Document-level selection request for one or more completed-sketch faces.

**Fields**:
- `sketchId: quint64`
- `loopIndices: std::vector<int>` — interpreted here as selected bounded-region indices

**Validation rules**:
- At least one selected region must exist.
- All region indices in a request must belong to the same sketch.
- Duplicate indices are ignored before resolution.
- Non-material or unbounded regions must not enter the selection set.

**Relationships**:
- One `Sketch Face Selection` resolves to one or more `Selected Sketch Profile` records.

## 4. Selected Sketch Profile

**Purpose**: Region-accurate extrusion input representing the face the user selected.

**Fields**:
- `sketchId: quint64`
- `loopIndex: int`
- `sourceEntityIds: std::vector<quint64>`
- `boundarySegments: std::vector<SketchBoundarySegment>` — closed outer boundary
- `polygon: std::vector<QVector2D>`
- `holes: std::vector<HoleBoundary>`
- `plane: SketchPlane`
- `isClosed: bool`

**Validation rules**:
- The selected region must resolve against current sketch topology.
- The outer boundary must be closed and non-degenerate.
- Hole boundaries are attached only for true enclosed voids.
- Missing or construction-only source geometry rejects the whole profile before extrusion.

**Relationships**:
- Many `Selected Sketch Profile` records may belong to one `Sketch Face Selection`.
- One `Selected Sketch Profile` owns zero or more `HoleBoundary` records.

## 5. Hole Boundary

**Purpose**: Inner excluded loop attached to a selected sketch face when the selected material region contains a true void.

**Fields**:
- `holeLoopIndex: int`
- `entityIds: std::vector<quint64>`
- `boundarySegments: std::vector<SketchBoundarySegment>`
- `polygon: std::vector<QVector2D>`

**Validation rules**:
- Hole must be closed, non-degenerate, and contained by the selected outer boundary.
- Overlap-created positive-material regions must not be downgraded into holes.
- Multiple holes for one selected face must be preserved through face construction.

**Relationships**:
- Many `HoleBoundary` records may belong to one `Selected Sketch Profile`.

## 6. Extrude Request / Result

**Purpose**: Existing extrusion operation enriched with resolved bounded-region profiles.

**Fields**:
- `params.distance: double`
- `params.mode: int` (`0 = New Body`, `1 = Add`, `2 = Remove`)
- `params.symmetric: bool`
- `profiles: std::vector<SelectedSketchProfile>`
- `result.success: bool`
- `result.errorMsg: QString`
- `result.solids / result.finalTargetShape`: OCCT output where available

**Validation rules**:
- `profiles` must not be empty.
- Every selected profile must resolve before document mutation occurs.
- OCCT face construction must preserve the selected bounded region and any true holes.
- Failure must abort without partial body creation or partial boolean edits.

**State transitions**:
- `Sketch edited -> Regions derived -> Region selected -> Profile resolved -> Previewed`
- `Sketch edited -> Regions derived -> Region selected -> Profile resolved -> Committed`
- `Sketch edited -> Previous region indices invalidated -> Regions re-derived`
- `Sketch edited -> Region resolution failed -> Extrusion blocked with error`

## 7. Resulting 3D Object

**Purpose**: The solid contribution created from a selected bounded sketch region.

**Fields**:
- `bodyId: quint64` or updated target-body identity
- `sourceLoopIndex: int`
- `shape: TopoDS_Shape`
- `holeCount: int`

**Validation rules**:
- Material must exist only in the selected bounded region.
- Every selectable bounded region must extrude without being treated as an open profile.
- Open, tangent-only, or otherwise unbounded areas must not create committed solids.

**Relationships**:
- One `Selected Sketch Profile` maps to one resulting solid contribution.
- Add/Cut modes may fold multiple selected profiles into one updated target body.
