# Research: Fix Sketch Intersections

## Decision 1: Classify selectable material from derived bounded-region topology

**Decision**: Derive explicit bounded-region topology for completed sketches and classify each region as selectable material or excluded void based on containment and boundary ownership, instead of relying on loop-depth rules alone.

**Rationale**: The reported rectangle-circle overlap bug proves that simple parent/child loop interpretation can omit valid material regions, especially when intersections split shapes into multiple bounded areas. A bounded-region topology layer preserves the current architecture while supporting the accepted scope of any supported sketch entity combination that forms a bounded planar region.

**Alternatives considered**:
- Keep classifying loops only by odd/even nesting depth: rejected because overlap-generated regions can still be hidden or misclassified.
- Move region classification into `Document` or UI code: rejected because it breaks the document/sketch/render separation required by the constitution.

## Decision 2: Preserve the existing `SketchArea` selection contract, but make indices refer to resolved bounded regions

**Decision**: Continue using `Document::SelectedItem::Type::SketchArea` plus `sketchId` and integer indices, but define the index as a resolved bounded-region identifier for the current completed-sketch topology.

**Rationale**: `Document`, `Renderer`, and `MainWindow` already exchange sketch-area selections through this contract, so keeping it minimizes churn. Rebinding the meaning of the index to a bounded material region fixes missing selections without introducing a new cross-layer selection payload.

**Alternatives considered**:
- Introduce a new selection object carrying full polygons or entity sets: rejected because it expands the document/UI surface area unnecessarily.
- Special-case the missing three-quarter-circle region in the picker: rejected because it would not generalize to other supported overlap arrangements.

## Decision 3: Resolve each selected region to explicit outer/hole boundaries before extrusion

**Decision**: Convert a selected bounded region into a `SelectedSketchProfile` containing explicit outer boundary segments and only the true inner hole boundaries needed for face creation and extrusion.

**Rationale**: `SketchToWire::buildFaceForProfile` and `ExtrudeOperation::extrudeProfiles` are already the correct seam for region-accurate OCCT conversion. Feeding them explicit region data prevents ambiguous whole-sketch conversion and allows open or partial source entities to participate safely when the combined boundary is closed.

**Alternatives considered**:
- Re-convert the whole sketch during extrusion and let OCCT infer the intended face: rejected because it ignores the actual user-selected region.
- Extrude broader geometry first and boolean-trim later: rejected because it adds avoidable complexity and risks mismatch between selection and resulting solid.

## Decision 4: Use the repository regression target plus manual desktop evidence, and omit contracts

**Decision**: Validation for this feature will rely on the existing `sketch_intersection_regression` target, `ctest` execution, full Release build verification, and explicit manual desktop validation steps in `quickstart.md`. No `contracts/` artifact will be created.

**Rationale**: This feature changes internal desktop modeling behavior rather than a reusable external interface, so a contract document would be artificial. The constitution instead requires strong verification evidence, which is better served by dedicated regression coverage and reviewer-run CAD interaction scenarios.

**Alternatives considered**:
- Create a UI interaction contract anyway: rejected because it would duplicate the spec without defining a real external protocol.
- Depend on manual validation only: rejected because the repository already has a focused regression target that should remain part of the required evidence.
