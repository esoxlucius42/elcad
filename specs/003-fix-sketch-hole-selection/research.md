# Research: Fix Sketch Hole Selection

## Decision 1: Derive sketch-face containment from closed-loop topology instead of treating every loop as a standalone filled face

**Decision**: Keep using `SketchPicker::findClosedLoops` as the base loop detector, then derive parent/child containment metadata so a selected bounded face can be interpreted as one outer loop plus its enclosed inner loops.

**Rationale**: The current picker/extrude path knows only about independent closed loops, which is why a rectangle loop is resolved as a filled face and an inner circle can still be treated as a separate selectable area. A containment layer is the smallest change that explains which loops are usable faces versus excluded hole boundaries without moving topology state into the UI or `Document`.

**Alternatives considered**:
- Rebuild the planar-face algorithm to emit composite annular regions directly: rejected because it is a larger rewrite of working loop-detection code.
- Store explicit hole membership in `Document` selection state: rejected because hole relationships are derived geometry and can stay local to `SketchPicker` profile resolution.

## Decision 2: Keep selection keyed by the chosen outer loop, but reject hits that land inside enclosed hole loops

**Decision**: Continue selecting sketch areas by outer loop index, and update area-hit evaluation so a click is valid only when it is inside the candidate outer loop and outside every enclosed hole loop.

**Rationale**: The existing selection pipeline already propagates sketch-area loop indices through `Document::SelectedItem` and `selectedSketchFaces()`. Preserving that contract minimizes UI/document churn while still fixing both broken behaviors: annular clicks resolve to the intended outer face, and clicks inside the inner void produce no surrounding-face selection.

**Alternatives considered**:
- Introduce a brand-new selection payload containing outer and hole loop indices: rejected because the same outcome can be achieved with less surface-area change by deriving holes during profile resolution.
- Allow clicks inside holes to select the inner loop as a positive face: rejected because it violates FR-003 and misleads users about the resulting solid.

## Decision 3: Represent one resolved sketch face as an outer boundary plus zero or more hole boundaries

**Decision**: Extend `SelectedSketchProfile` to hold the outer-loop geometry and a collection of enclosed hole boundaries, each carrying loop identity, polygon data, and source sketch entities.

**Rationale**: `SketchToWire::buildFaceForProfile` currently receives only one flat loop, so it cannot construct a face with holes even when the picker knows the intended area. A richer selected-profile model keeps the interpretation localized to `sketch/` code and provides exactly the data OCCT needs for correct face construction.

**Alternatives considered**:
- Subtract hole solids after extruding the outer loop: rejected because it adds unnecessary boolean complexity and risks preview/commit divergence.
- Convert the entire sketch again during extrusion and hope OCCT infers holes: rejected because whole-sketch conversion caused the current ambiguity and ignores selection intent.

## Decision 4: Build the selected face in OCCT from one outer wire plus inner wires, and keep contracts omitted

**Decision**: Update `SketchToWire::buildFaceForProfile` to create an OCCT face from the outer boundary wire and add one wire per hole boundary before extrusion. Do not generate a `contracts/` artifact for this feature.

**Rationale**: OCCT natively supports planar faces with inner wires, so this is the direct representation of the desired bounded face. The feature is an internal desktop interaction fix, not a new public interface, so a contract artifact would be artificial while the real review evidence is build output plus manual validation.

**Alternatives considered**:
- Keep `buildFaceForProfile` single-wire only and special-case hole removal later in `ExtrudeOperation`: rejected because the face itself would already be wrong.
- Add a UI/interaction contract document: rejected because the feature spec and quickstart already cover user-visible behavior without defining a reusable external protocol.
