# Data Model: Double-Sided Face Rendering

## 1. Extruded Face

**Purpose**: A visible surface produced by extruding a sketch profile into a 3D body.

**Fields**:
- `bodyId: quint64`
- `faceOrdinal: int` — mesh-face grouping already tracked by `MeshBuffer`
- `surfaceRole: enum` — expected categories such as cap, side wall, or subdivided face fragment
- `orientationState: enum` — normal-aligned or reversed relative to the expected exterior view
- `visibleRegionCount: int` — number of triangulated surface regions that can contribute pixels to the viewport

**Validation rules**:
- Face must come from a non-null OCCT body shape and a non-empty triangulation.
- Reversed orientation must not suppress rendering of externally visible pixels.
- The feature must apply consistently across caps, side walls, and subdivided faces described in the spec.

**Relationships**:
- One `Extruded Face` belongs to one rendered body.
- One `Extruded Face` expands to many `Surface Sample` records during rasterization.

## 2. Surface Sample

**Purpose**: A rasterized triangle fragment candidate evaluated by the body-surface shader.

**Fields**:
- `triangleIndex: int`
- `worldPosition: QVector3D`
- `interpolatedNormal: QVector3D`
- `viewDirection: QVector3D`
- `depthValue: float`
- `isOccluded: bool`

**Validation rules**:
- Every sample must originate from a triangle already uploaded by `MeshBuffer`.
- Depth testing remains enabled so farther samples cannot overwrite nearer visible surfaces.
- Occluded samples must stay hidden even when the visible side of the same face becomes two-sided.

**Relationships**:
- Many `Surface Sample` records belong to one `Extruded Face`.
- Each `Surface Sample` produces one `Two-Sided Lighting Evaluation`.

## 3. Two-Sided Lighting Evaluation

**Purpose**: The shader-time decision that determines how a surface sample is shaded when viewed from either side.

**Fields**:
- `cameraFacingNormal: QVector3D`
- `sourceNormal: QVector3D`
- `isBackView: bool`
- `diffuseContribution: float`
- `fillContribution: float`
- `specularContribution: float`
- `finalAlpha: float`

**Validation rules**:
- If the source normal points away from the viewer, the shading normal must be flipped toward the camera before the Phong lighting terms are evaluated.
- `finalAlpha` for opaque bodies remains `1.0f`; this feature does not change hidden-surface rules.
- Lighting changes must not alter selection/picking identity for the underlying triangle.

**Relationships**:
- One `Two-Sided Lighting Evaluation` belongs to one `Surface Sample`.
- The evaluation feeds the final visible color for the shared Phong body pass and preview pass.

## 4. Visibility Review Record

**Purpose**: Reviewer-captured evidence that the rendering change fixes the known bug without introducing see-through regressions.

**Fields**:
- `scenarioId: string`
- `buildVerified: bool`
- `frontViewVisible: bool`
- `backViewVisible: bool`
- `occlusionPreserved: bool`
- `notes: QString`
- `artifactRefs: list` — screenshots, screen recordings, or build logs retained by the reviewer

**Validation rules**:
- The known reproduction case must show the previously missing face from both front and back viewpoints.
- At least one previously correct extrusion must remain visually correct after the change.
- Hidden faces must remain hidden behind nearer geometry.

**Relationships**:
- One `Visibility Review Record` summarizes one manual validation scenario from `quickstart.md`.

## State Transitions

- `Extruded Face generated -> MeshBuffer triangulated -> Surface Sample rasterized -> Two-Sided Lighting Evaluation applied -> Visible if not occluded`
- `Extruded Face generated -> Surface Sample rasterized from opposite camera side -> camera-facing normal flipped -> Visible if not occluded`
- `Surface Sample behind nearer geometry -> depth test fails -> remains hidden`
- `Body shape changes -> mesh cache invalidated -> Extruded Face / Surface Sample data rebuilt on next render`
