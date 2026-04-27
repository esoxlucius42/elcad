# Data Model: Fix Extrude Shape Fidelity

## 1. Selected Sketch Profile

**Purpose**: Represents one bounded material region selected from a finished sketch and used as the source for one extruded result.

**Fields**:
- `sketchId: quint64`
- `loopIndex: int`
- `sourceEntityIds: list<quint64>`
- `boundarySegments: list<SketchBoundarySegment>`
- `polygon: list<QVector2D>`
- `holes: list<HoleBoundary>`
- `plane: SketchPlane`
- `isClosed: bool`

**Validation rules**:
- Must resolve only from a selectable material loop, not an odd-depth hole loop.
- Must contain at least one outer boundary segment and a valid sketch plane.
- `isClosed` must remain true before face-building or extrusion proceeds.

**Relationships**:
- One `Selected Sketch Profile` owns many `Sketch Boundary Segment` records.
- One `Selected Sketch Profile` may own zero or more `Hole Boundary` records.
- One `Selected Sketch Profile` yields one `Planar Extrude Face` and, on success, one `Extruded Result`.

## 2. Sketch Boundary Segment

**Purpose**: Carries the resolved source geometry for one contiguous portion of a selected profile boundary.

**Fields**:
- `type: enum` — line, arc, or full circle
- `sourceEntityId: quint64`
- `start: QVector2D`
- `end: QVector2D`
- `center: QVector2D`
- `radius: float`
- `counterClockwise: bool`

**Validation rules**:
- Adjacent segments from the same source entity may merge only when continuity and arc direction are preserved.
- Arc segments must retain the correct `counterClockwise` value from the originating sketch entity or derived loop orientation.
- Segment ordering must describe a continuous closed loop when aggregated for a profile or hole.

**Relationships**:
- Many `Sketch Boundary Segment` records belong to one `Selected Sketch Profile` or one `Hole Boundary`.
- `SketchToWire::buildWireFromSegments()` consumes ordered `Sketch Boundary Segment` records to reconstruct OCCT wires.

## 3. Hole Boundary

**Purpose**: Represents one enclosed void inside a selected material loop that must remain open after extrusion.

**Fields**:
- `holeLoopIndex: int`
- `entityIds: list<quint64>`
- `boundarySegments: list<SketchBoundarySegment>`
- `polygon: list<QVector2D>`

**Validation rules**:
- Must resolve from a child loop nested inside its parent material loop.
- Must contain a closed ordered boundary before it is added to the face builder.
- Must never be silently converted into filled material during face creation or extrusion.

**Relationships**:
- Many `Hole Boundary` records may belong to one `Selected Sketch Profile`.
- Each `Hole Boundary` contributes one subtractive wire to a `Planar Extrude Face`.

## 4. Planar Extrude Face

**Purpose**: The OCCT face reconstructed from one selected profile's outer boundary plus any hole boundaries immediately before prism creation.

**Fields**:
- `profileLoopIndex: int`
- `outerBoundaryCount: int`
- `holeCount: int`
- `plane: SketchPlane`
- `isTopologicallyValid: bool`

**Validation rules**:
- Must be built on the selected sketch plane.
- Must include every preserved hole wire before extrusion.
- Must pass OCCT validity checks before a prism is considered trustworthy.

**Relationships**:
- One `Planar Extrude Face` comes from one `Selected Sketch Profile`.
- One `Planar Extrude Face` produces one `Extruded Result`.

## 5. Extruded Result

**Purpose**: The solid created from one selected sketch profile and expected to remain faithful to the source face's outline, hole boundaries, placement, and orientation.

**Fields**:
- `sourceLoopIndex: int`
- `distance: double`
- `mode: int`
- `symmetric: bool`
- `solidValid: bool`
- `matchesSourceProfile: bool`

**Validation rules**:
- Must correspond to exactly one selected profile in normal extrusion mode.
- Must preserve curved edges and internal voids from the source profile.
- Must not borrow edges, holes, or orientation from any other selected profile.

**Relationships**:
- One `Extruded Result` belongs to one `Selected Sketch Profile`.
- Many `Extruded Result` records may be created by one multi-face extrusion command.

## State Transitions

- `Derived bounded loop -> selectable material loop -> Selected Sketch Profile resolved`
- `Selected Sketch Profile resolved -> boundary segments merged -> holes attached -> Planar Extrude Face built`
- `Planar Extrude Face valid -> prism created -> Extruded Result stored or combined per mode`
- `Any boundary/face validation failure -> user-facing error -> no distorted solid committed`
