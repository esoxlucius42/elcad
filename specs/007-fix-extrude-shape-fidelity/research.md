# Research: Fix Extrude Shape Fidelity

## Decision 1: Preserve original arc direction and loop orientation through profile resolution

**Decision**: Treat the highest-risk defect seam as the `SketchPicker` path that flattens sketch geometry into `SketchBoundarySegment` records, and preserve the true arc direction and merged loop orientation all the way into `SelectedSketchProfile`.

**Rationale**: `SketchBoundarySegment` already models `counterClockwise`, and `SketchToWire::buildWireFromSegments()` relies on it when reconstructing OCCT arcs. Current flattening code in `SketchPicker.cpp` hardcodes arc boundaries as counter-clockwise, which can reverse selected curved regions, break hole/material classification, and yield distorted or wedge-like extruded solids even when the selected face looked correct in the sketch.

**Alternatives considered**:
- Fix only `SketchToWire`: rejected because face building cannot recover the original curve intent if profile boundary metadata is already wrong.
- Treat the problem as viewport-only rendering distortion: rejected because the screenshots show wrong created solids, and the sketch-domain code already contains the metadata seam that controls profile fidelity.

## Decision 2: Keep hole preservation and planar face construction centered in `SketchToWire::buildFaceForProfile`

**Decision**: Preserve the existing architecture where `SketchPicker` resolves one material loop plus child hole loops and `SketchToWire::buildFaceForProfile()` constructs the planar OCCT face, including all hole wires.

**Rationale**: The feature scope now explicitly includes internal voids, and `buildFaceForProfile()` is already the narrow seam that turns resolved outer and hole boundaries into one face before extrusion. Keeping fidelity enforcement here avoids duplicating topology logic in `ExtrudeOperation` and keeps user-facing behavior aligned with selected sketch faces instead of the full sketch wire.

**Alternatives considered**:
- Rebuild holes during extrusion: rejected because it would duplicate profile semantics in `ExtrudeOperation` and increase the chance of divergence between preview and commit paths.
- Flatten all loops into a single wire and infer holes later: rejected because the existing profile model already distinguishes outer boundaries from holes more safely.

## Decision 3: Keep extrusion generation in `ExtrudeOperation`, but validate source-profile fidelity before and after prism creation

**Decision**: Leave `ExtrudeOperation::extrudeProfiles()` responsible for prism generation and boolean follow-up, while ensuring it only receives profile-derived faces that already preserve the selected source boundary and holes.

**Rationale**: `ExtrudeOperation` is already the correct seam for OCCT prism creation and per-profile result collection. The safer fix is to harden the selected-profile → face conversion path and then extend extrusion regression coverage so invalid or distorted solids are caught with `BRepCheck_Analyzer` and profile-focused assertions.

**Alternatives considered**:
- Move more topology repair into `ExtrudeOperation`: rejected because it broadens the extrusion seam and risks masking upstream profile corruption instead of fixing it.
- Add renderer-side corrections after bad solids are created: rejected because the feature requires the solid itself to match the selected sketch face.

## Decision 4: Extend the existing sketch regression harness and use manual screenshot validation; omit contracts

**Decision**: Add focused extrusion-fidelity cases to the existing `sketch_intersection_regression` harness and pair them with manual validation scenarios based on the provided screenshots. Do not create a `contracts/` artifact.

**Rationale**: The repository already has a lightweight OCCT-backed regression target in `tests/sketch/`, including extrusion and hole-preservation assertions, so extending that harness is lower-risk than inventing a new test framework. This feature does not expose a reusable external interface, so a contracts artifact would duplicate the spec and quickstart without defining a meaningful API surface.

**Alternatives considered**:
- Rely on manual validation only: rejected because there is already an automated regression seam that can catch profile/face/extrusion regressions.
- Create a UI contract document anyway: rejected because this is internal desktop behavior, not a protocol or public API.
