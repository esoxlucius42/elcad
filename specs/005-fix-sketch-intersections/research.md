# Research: Fix Sketch Intersections

## Decision 1: Classify bounded sketch regions from derived region topology instead of using hole-depth rules alone

**Decision**: Extend the completed-sketch topology analysis so the planner/implementation can reason about bounded regions created by intersecting closed shapes, not just standalone loops plus nested holes.

**Rationale**: The current hole-aware approach works for a circle entirely inside a rectangle, but the reported overlap bug shows that loop-depth / parent-child containment alone is not enough to expose every bounded material region when closed shapes intersect. A region-topology layer lets the implementation distinguish true selectable material regions, excluded voids, and non-bounded space without pushing derived geometry into `Document` or UI state.

**Alternatives considered**:
- Keep treating every detected loop as either a standalone face or a hole based only on nesting depth: rejected because overlap-generated regions can be misclassified or omitted.
- Rebuild the entire picker around OCCT booleans: rejected because the sketch picker already has a planar arrangement path and the feature should stay localized to sketch-region derivation.

## Decision 2: Preserve the existing sketch-area selection flow, but make the area index resolve to a bounded region record

**Decision**: Keep using the `Document::SelectedItem::SketchArea` selection path and region-like indices for completed sketch picking, while redefining those indices to refer to the resolved bounded regions that remain valid after topology refresh.

**Rationale**: `Document`, `Renderer`, and `MainWindow` already understand sketch-area selections by sketch ID plus index, so preserving that contract minimizes UI/document churn. The important change is what the index represents: it must map to a stable bounded region candidate rather than to a loop interpretation that only works for simple holes.

**Alternatives considered**:
- Introduce a brand-new selection payload containing entity sets or full polygons: rejected because it creates wider UI/document surface-area changes than this fix requires.
- Keep the current loop index semantics and special-case the missing region in the picker: rejected because it would not generalize to future intersection layouts or edit-refresh behavior.

## Decision 3: Resolve each selected sketch face to explicit boundary segments and optional holes before extrusion

**Decision**: Continue the current resolved-profile approach, but make each selected profile represent one bounded region with explicit boundary segments, source entities, and hole boundaries only when the region truly contains excluded inner loops.

**Rationale**: `SketchToWire::buildFaceForProfile` and `ExtrudeOperation::extrudeProfiles` already form the correct seam for converting a selected sketch face into an OCCT face/solid. Feeding those layers a richer, region-accurate profile keeps the extrusion behavior aligned with user intent and avoids falling back to whole-sketch conversion, which is what causes ambiguous or invalid profile handling.

**Alternatives considered**:
- Convert the entire sketch again during extrusion and let OCCT infer the intended face: rejected because that ignores the selected bounded region and recreates the ambiguity.
- Extrude a larger region and cut/intersect later: rejected because it adds unnecessary boolean complexity and risks preview/commit divergence.

## Decision 4: Skip contracts and rely on build + manual validation evidence

**Decision**: Do not create a `contracts/` artifact for this feature. Define validation through the repository build plus explicit manual scenarios for overlap selection, extrusion, sketch edits, and negative edge cases.

**Rationale**: The feature changes internal desktop modeling behavior and does not expose a reusable external interface, API, or command schema. The constitution still requires reviewable evidence, so the correct planning output is a strong quickstart/manual-validation document rather than an artificial contract file.

**Alternatives considered**:
- Add a UI or interaction contract anyway: rejected because it would duplicate the feature specification without defining a real external protocol.
- Promise automated regression coverage in this plan despite the lack of a current harness: rejected because the plan should accurately reflect the repository's present verification capabilities.
