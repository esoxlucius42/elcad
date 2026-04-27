# Quickstart: Validate Fix Sketch Intersections

## Prerequisites

1. From the repository root, configure the project:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   ```
2. Build the application:
   ```bash
   cmake --build build --parallel $(nproc)
   ```
3. Run the focused regression target:
   ```bash
   cmake --build build --target sketch_intersection_regression --parallel $(nproc)
   ctest --test-dir build --output-on-failure -R sketch-intersection-regression
   ```
4. Launch the desktop app:
   ```bash
   ./build/bin/elcad
   ```

## Verification Scope

This feature must validate bounded-region selection and extrusion for any supported sketch entity combination that forms a bounded planar region. The mandatory regression evidence centers on the rectangle-circle overlap from the spec, plus edit-refresh, tangent-only, open-boundary, and duplicate-boundary checks.

## Implementation Notes

- Completed sketches resolve overlap-generated bounded regions by current region index, so each material face can be hovered, highlighted, and selected independently.
- Extrusion now consumes resolved sketch-face profiles with explicit outer boundaries and true hole boundaries instead of reusing the whole sketch wire blindly.
- Re-activating or hiding a sketch invalidates stale completed-sketch area selections before the next pick/extrude attempt.

## Manual Validation Scenario A: All bounded regions from the rectangle-circle overlap are selectable

1. Create a sketch on a standard plane.
2. Draw one rectangle.
3. Draw one circle whose center lies on one rectangle corner so one quarter of the circle lies inside the rectangle.
4. Finish the sketch.
5. Click the rectangle-only region outside the circle.
6. Click the quarter-circle overlap region.
7. Click the remaining three-quarter circle region outside the rectangle.

**Expected result**:
- Each of the three bounded regions becomes selectable independently.
- The outer three-quarter circle region is selectable on the first attempt.
- No extra unbounded or phantom region becomes selectable.

**Evidence checklist**:
- [ ] Capture the source sketch geometry.
- [ ] Capture each of the three bounded regions highlighted independently.
- [ ] Add reviewer notes confirming the previously missing outer circle region is now selectable.

## Manual Validation Scenario B: Every selectable overlap region extrudes as a closed profile

1. Reuse the same rectangle-circle overlap sketch.
2. Select the rectangle-only region and start the Extrude tool.
3. Repeat for the quarter-circle overlap region.
4. Repeat for the outer three-quarter circle region.
5. Use a visible distance such as `10 mm`.

**Expected result**:
- Each selected region previews and commits as a valid closed profile.
- No attempt fails with an open or incomplete profile error for these bounded regions.
- Only the selected region contributes material for each extrusion.

**Evidence checklist**:
- [ ] Save the successful Release build output.
- [ ] Save the successful regression-target and `ctest` output.
- [ ] Capture at least one successful extrusion of an overlap-generated region.
- [ ] Add reviewer notes confirming all three bounded regions extrude successfully.

## Manual Validation Scenario C: Region availability refreshes after sketch edits or deletion

1. Create the rectangle-circle overlap sketch and confirm the expected regions exist.
2. Re-open the sketch for editing.
3. Move or replace one intersecting shape so the overlap changes.
4. Finish the edit and attempt selection again.
5. Delete or undo one intersecting shape and check the available regions again.

**Expected result**:
- The selectable region set updates to match the current visible bounded areas.
- Removed overlaps no longer expose stale selectable regions.
- New overlap geometry is reflected before the next selection attempt.

**Evidence checklist**:
- [ ] Capture the region set before the edit.
- [ ] Capture the region set after the edit.
- [ ] Capture the region set after deletion or undo.
- [ ] Add reviewer notes confirming no stale region indices remain selectable.

## Manual Validation Scenario D: Negative edge cases stay non-misleading

1. Create two shapes that only touch tangentially without forming a bounded overlap region.
2. Confirm that no extra overlap face becomes selectable.
3. Edit one boundary so a formerly bounded region becomes open.
4. Attempt selection and extrusion again.
5. Create duplicate or coincident boundary segments and inspect the resulting selectable regions.
6. Optionally create a bounded region that depends on open or partial entities contributing to a closed boundary, and verify that only the truly bounded region is selectable/extrudable.

**Expected result**:
- Tangent-only contact does not create a phantom selectable face.
- Open or unbounded regions are not selectable or extrudable as sketch faces.
- Duplicate or coincident boundaries do not hide valid bounded regions or invent false ones.
- Open or partial entities only participate when the combined boundary is actually closed.

**Evidence checklist**:
- [ ] Capture the tangent-only case.
- [ ] Capture the open-boundary case outcome.
- [ ] Capture duplicate/coincident-boundary behavior.
- [ ] Add reviewer notes for any bounded-region case involving open or partial contributing entities.

## Evidence To Capture

- Successful output from `cmake --build build --parallel $(nproc)`
- Successful output from `cmake --build build --target sketch_intersection_regression --parallel $(nproc)`
- Successful output from `ctest --test-dir build --output-on-failure -R sketch-intersection-regression`
- Screenshot or recording showing all three bounded overlap regions highlighted independently
- Screenshot or recording showing at least one successful extrusion of an overlap-generated region
- Screenshot or recording showing region refresh after sketch edits or deletion
- Reviewer notes confirming tangent/open/duplicate edge cases remain correct

## Current Automated Validation

- [x] `cmake --build build --parallel $(nproc)`
- [x] `cmake --build build --target sketch_intersection_regression --parallel $(nproc)`
- [x] `ctest --test-dir build --output-on-failure -R sketch-intersection-regression`
- [ ] Manual desktop validation scenarios A-D completed and captured by a reviewer

## Contracts

No `contracts/` artifact is required for this feature because it changes internal desktop CAD behavior and does not add a reusable external API, CLI surface, or protocol.
