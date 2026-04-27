# Quickstart: Validate Extrude Shape Fidelity

## Prerequisites

1. From the repository root, configure the project:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   ```
2. Build the application:
   ```bash
   cmake --build build --parallel $(nproc)
   ```
3. Build and run the focused sketch regression target:
   ```bash
   cmake --build build --target sketch_intersection_regression --parallel $(nproc)
   ctest --test-dir build --output-on-failure -R sketch-intersection-regression
   ```
4. Launch the desktop app:
   ```bash
   ./build/bin/elcad
   ```

## Verification Scope

This feature validates that every extruded solid matches the selected sketch face that produced it, including curved boundaries, internal voids, and multi-face selections. Evidence must cover the screenshot-based repro where extruded solids became wedge-like or otherwise unrelated to the selected profiles, plus regression checks for hole preservation and profile-to-solid correspondence.

## Implementation Notes

- The planned fix stays in the sketch profile resolution and OCCT face-building path so both preview and committed extrusions consume the same corrected source geometry.
- Existing `SelectedSketchProfile` and hole-boundary data should remain the authoritative representation of what the user selected for extrusion.
- Extend the existing `sketch_intersection_regression` harness rather than adding a new test framework.
- No `contracts/` artifact is required because this feature changes internal desktop CAD behavior and does not introduce a public API, CLI surface, or protocol.

## Manual Validation Scenario A: The known distorted-result repro now extrudes matching solids

1. Recreate or load the sketch shown in `before-extrude.png`.
2. Select the same set of bounded faces that previously produced the distorted results shown in `after-extrude.png`.
3. Start the extrude workflow and confirm the operation with a visible distance such as `10 mm`.

**Expected result**:
- Each created solid matches the outline of its selected sketch face.
- Curved selected regions remain curved rather than collapsing into wedge-like substitutes.
- No result appears mirrored, rotated into an unrelated outline, or blended with another selected face.

**Evidence checklist**:
- [ ] Capture the selected sketch faces before extrusion.
- [ ] Capture the resulting solids from the same view used in the repro screenshot.
- [ ] Add reviewer notes confirming the original distortion no longer reproduces.

## Manual Validation Scenario B: Curved boundaries remain intact from sketch to solid

1. Create or load a sketch face that mixes straight segments with one or more arcs.
2. Select only that face.
3. Extrude it using the standard workflow.

**Expected result**:
- The resulting solid preserves the same curved and straight boundary transitions as the source face.
- No curved segment is replaced by an unintended straight shortcut or collapsed corner.

**Evidence checklist**:
- [ ] Capture the source face with the curved boundary called out.
- [ ] Capture the extruded solid showing the preserved curve.
- [ ] Add reviewer notes confirming FR-002 and SC-001 from the spec.

## Manual Validation Scenario C: Internal holes remain open after extrusion

1. Create or load a sketch face that contains one or more enclosed voids.
2. Select the material region around the void.
3. Extrude the face.

**Expected result**:
- The resulting solid preserves each internal opening instead of filling it in.
- The outer profile remains correct while the voids stay aligned with the selected face.

**Evidence checklist**:
- [ ] Capture the source face with its enclosed voids.
- [ ] Capture the resulting solid from a view that makes the preserved openings obvious.
- [ ] Add reviewer notes confirming the hole-preservation clarification is satisfied.

## Manual Validation Scenario D: Multi-face extrusion keeps one-to-one source mapping

1. Create or load a sketch with at least three separated bounded faces.
2. Select multiple faces in one extrude command.
3. Confirm the operation with one shared extrusion distance.

**Expected result**:
- One solid is created for each selected face.
- Each solid remains positioned over its own source face.
- No result borrows geometry, holes, or orientation from any other selected face.

**Evidence checklist**:
- [ ] Capture the multi-face selection before extrusion.
- [ ] Capture the completed solids with each source/result correspondence visible.
- [ ] Add reviewer notes confirming FR-004, FR-005, FR-007, and SC-002 from the spec.

## Manual Validation Scenario E: Invalid fidelity cases fail safely

1. Use a sketch case that still cannot be converted into a faithful face after the fix, if one is known during implementation.
2. Attempt extrusion.

**Expected result**:
- The operation stops without creating a misleading distorted solid.
- The user sees a clear failure message instead of a corrupted result.

**Evidence checklist**:
- [ ] Capture the failing source case if reproduced.
- [ ] Capture the user-facing error outcome.
- [ ] Add reviewer notes confirming no partial or distorted solid was committed.

## Evidence To Capture

- Successful output from `cmake -B build -DCMAKE_BUILD_TYPE=Release`
- Successful output from `cmake --build build --parallel $(nproc)`
- Successful output from `cmake --build build --target sketch_intersection_regression --parallel $(nproc)`
- Successful output from `ctest --test-dir build --output-on-failure -R sketch-intersection-regression`
- Before/after screenshots of the known distortion repro
- Screenshot or recording showing preserved curved boundaries
- Screenshot or recording showing preserved internal holes
- Screenshot or recording showing correct multi-face source/result correspondence
- Reviewer notes for any safe-failure case exercised during review

## Current Automated Validation

- [X] `cmake -B build -DCMAKE_BUILD_TYPE=Release`
- [X] `cmake --build build --parallel $(nproc)`
- [X] `cmake --build build --target sketch_intersection_regression --parallel $(nproc)`
- [X] `ctest --test-dir build --output-on-failure -R sketch-intersection-regression`
- [ ] Manual desktop validation scenarios A-E completed and captured by a reviewer

## Contracts

No `contracts/` artifact is required for this feature because it changes internal desktop CAD behavior and does not add a reusable external API, CLI surface, or protocol.
