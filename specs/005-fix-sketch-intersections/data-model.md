# Data Model: Fix Sketch Intersections

## 1. Closed Sketch Shape

**Purpose**: Existing user-authored closed primitive that contributes area to the sketch arrangement.

**Fields**:
- `entityId: quint64`
- `type: SketchEntity::Type` (`Line` chains interpreted as closed loops, `Circle`, `Arc` where applicable)
- `construction: bool`
- Geometric parameters already stored in `SketchEntity`

**Validation rules**:
- Construction geometry never contributes selectable sketch regions.
- Only supported closed-shape geometry participates in bounded-region generation.
- Editing or deleting a contributing shape invalidates any previously derived bounded-region mapping.

**Relationships**:
- Many `Closed Sketch Shape` records belong to one completed `Sketch`.
- One shape may contribute boundary segments to many `Derived Sketch Region` records after intersections are split.

## 2. Derived Sketch Region

**Purpose**: Internal bounded-region candidate produced from completed-sketch topology analysis.

**Fields**:
- `regionIndex: int` — stable per-analysis identifier used by completed-sketch selection/highlighting
- `polygon: std::vector<QVector2D>` — representative ordered polygon for hit testing and highlighting
- `boundarySegments: std::vector<SketchBoundarySegment>` — explicit split line/arc boundaries for the region
- `sourceEntityIds: std::vector<quint64>` — contributing sketch entities
- `parentRegionIndex: std::optional<int>` — containing region when containment exists
- `childRegionIndices: std::vector<int>` — immediately enclosed regions, if any
- `isSelectableMaterial: bool` — whether the region represents user-selectable material rather than an excluded void
- `samplePoint: QVector2D` or equivalent derived interior probe used for classification

**Validation rules**:
- Region must be bounded and non-degenerate (`polygon.size() >= 3`).
- Boundary segments must connect into a closed outer loop for the resolved face.
- Classification must distinguish overlap-generated material regions from true hole/void regions.
- Region indices are only valid for the current sketch geometry and must refresh after sketch edits.

**Relationships**:
- One completed `Sketch` yields zero or more `Derived Sketch Region` records.
- One `Derived Sketch Region` may resolve to one `Selected Sketch Profile`.

## 3. Sketch Face Selection

**Purpose**: Existing document-level selection request for one or more completed-sketch faces.

**Fields**:
- `sketchId: quint64`
- `loopIndices: std::vector<int>` — interpreted here as selected bounded-region indices

**Validation rules**:
- At least one selected region must exist.
- All selected region indices in one request must belong to the same sketch.
- Duplicate indices are ignored before resolution.
- Regions classified as non-material / non-selectable must not enter the selection set.

**Relationships**:
- One `Sketch Face Selection` resolves to one or more `Selected Sketch Profile` records.

## 4. Selected Sketch Profile

**Purpose**: Region-accurate extrusion input representing the face the user actually selected.

**Fields**:
- `sketchId: quint64`
- `loopIndex: int` — selected bounded-region index
- `sourceEntityIds: std::vector<quint64>`
- `boundarySegments: std::vector<SketchBoundarySegment>` — outer boundary segments for the selected face
- `polygon: std::vector<QVector2D>`
- `holes: std::vector<HoleBoundary>` — enclosed excluded loops when the selected region contains true holes
- `plane: SketchPlane`
- `isClosed: bool`

**Validation rules**:
- The selected region must resolve against the current sketch topology.
- The outer boundary must be closed and non-degenerate.
- Hole boundaries are attached only when the selected face truly contains excluded inner loops.
- Missing or construction-only source geometry rejects the whole profile before extrusion.

**Relationships**:
- Many `Selected Sketch Profile` records may belong to one `Sketch Face Selection`.
- One `Selected Sketch Profile` owns zero or more `HoleBoundary` records.

## 5. HoleBoundary

**Purpose**: Inner excluded loop attached to a selected sketch face when the resolved region contains a true void.

**Fields**:
- `holeLoopIndex: int`
- `entityIds: std::vector<quint64>`
- `boundarySegments: std::vector<SketchBoundarySegment>`
- `polygon: std::vector<QVector2D>`

**Validation rules**:
- Hole must be closed, non-degenerate, and contained by the selected outer boundary.
- Hole boundaries must not be synthesized for overlap regions that represent positive material.
- Multiple enclosed holes for one selected face must all be preserved through face construction.

**Relationships**:
- Many `HoleBoundary` records may belong to one `Selected Sketch Profile`.

## 6. Extrude Request / Result

**Purpose**: Existing extrusion operation enriched with region-accurate selected profiles.

**Fields**:
- `params.distance: double`
- `params.mode: int` (`0 = New Body`, `1 = Add`, `2 = Remove`)
- `params.symmetric: bool`
- `profiles: std::vector<SelectedSketchProfile>`
- `result.success: bool`
- `result.errorMsg: QString`
- `result.solids / result.finalTargetShape: OCCT output`

**Validation rules**:
- `profiles` must not be empty.
- Every selected profile must resolve before document mutation occurs.
- OCCT face construction must preserve the selected region boundary and any true holes.
- Failure must abort without leaving partial new bodies or partial boolean edits behind.

**State transitions**:
- `Sketch changed -> Regions derived -> Region selected -> Profile resolved -> Previewed`
- `Sketch changed -> Regions derived -> Region selected -> Profile resolved -> Committed`
- `Sketch changed -> Regions derived -> Region selected -> Profile resolution failed`
- `Sketch changed -> Previous region indices invalidated -> Regions re-derived`

## 7. Resulting 3D Object

**Purpose**: The committed solid contribution created from a selected bounded sketch region.

**Fields**:
- `bodyId: quint64` or updated target-body identity
- `sourceLoopIndex: int`
- `shape: TopoDS_Shape`
- `holeCount: int`

**Validation rules**:
- Material must exist only in the selected bounded region.
- Every selectable overlap region must extrude without being treated as an open profile.
- Open, tangent-only, or otherwise unbounded areas must not create committed solids.

**Relationships**:
- One `Selected Sketch Profile` maps to one resulting solid contribution.
- Add/Cut modes may fold multiple selected profiles into one updated target body.
