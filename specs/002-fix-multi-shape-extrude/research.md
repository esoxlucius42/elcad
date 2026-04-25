# Research: Fix Multi-Shape Extrude

## Decision 1: Scope sketch extrusion to selected sketch areas instead of the entire sketch

**Decision**: Resolve the selected `Document::SelectedItem::SketchArea` entries, group them by `sketchId`, and extrude only those selected closed faces.

**Rationale**: The current failure comes from `SketchToWire::convert` trying to connect every non-construction entity in the sketch into one wire, which fails for disconnected loops and also ignores partial-selection intent. The repository already records sketch-area selections by loop index, so the fix should start from that selection model rather than from whole-sketch geometry.

**Alternatives considered**:
- Reuse whole-sketch conversion and try to auto-connect disconnected loops: rejected because it preserves the root cause and still cannot exclude unselected faces.
- Extrude the entire sketch and filter results afterward: rejected because it violates the requirement to affect only selected faces.

## Decision 2: Build one OCCT face and one solid per selected profile

**Decision**: Treat each selected profile as its own extrusion input, producing a per-profile OCCT face/solid result instead of one shared sketch-wide face.

**Rationale**: Separate selected loops need separate topology so disconnected faces no longer depend on a single `BRepBuilderAPI_MakeWire` result. This also matches the product expectation that New Body mode yields one resulting body per selected face.

**Alternatives considered**:
- Build a single compound wire/face from all selected loops: rejected because it recreates the same connectivity risk and obscures per-profile validation.
- Merge selected loops into one body immediately: rejected because the spec requires separate resulting 3D objects for disconnected selected faces.

## Decision 3: Keep execution atomic for both body creation and target-body booleans

**Decision**: Validate all selected profiles and generate all tool solids first, then commit document changes only after every requested extrusion succeeds. In New Body mode, create one body per resulting solid; in Add/Cut modes, apply all generated solids against a working copy of the target shape and commit only the final boolean result.

**Rationale**: The specification explicitly prefers no partial output when any selected face is invalid. Building everything first preserves the existing `success` / `errorMsg` operation pattern while avoiding stray bodies or half-applied target-body edits.

**Alternatives considered**:
- Commit one body/boolean at a time as each profile succeeds: rejected because failures midway would leave partial geometry behind.
- Restrict the fix to New Body mode only: rejected because the shared extrude workflow and parameters should stay consistent across supported modes.

## Decision 4: Contracts are not applicable; verification remains build + manual workflow evidence

**Decision**: Do not generate a `contracts/` artifact for this feature. Use build verification plus explicit manual validation steps and evidence capture in quickstart documentation.

**Rationale**: The change is an internal desktop CAD workflow fix, not a new external API, CLI, or service contract. The repository currently has no automated test target, so the plan must document that gap and still require concrete manual evidence.

**Alternatives considered**:
- Add a UI contract document anyway: rejected because it would duplicate the feature spec without defining a real external interface.
- Promise automated regression coverage in this plan without a harness: rejected because that would overstate current repository capabilities.
