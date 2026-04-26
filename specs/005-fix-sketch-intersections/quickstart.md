# Quickstart: Validate Sketch Intersection Planning Output

## Prerequisites

1. From the repository root, configure the project:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   ```
2. Build the application:
   ```bash
   cmake --build build --parallel $(nproc)
   ```
3. Run elcad:
   ```bash
   ./build/bin/elcad
   ```

## Automation Note

- This feature adds a dedicated `sketch_intersection_regression` target plus a `ctest` entry for completed-sketch overlap selection, resolved-profile extrusion, and sketch-refresh regressions.
- Automated validation commands:
  ```bash
  cmake --build build --target sketch_intersection_regression --parallel $(nproc)
  ctest --test-dir build --output-on-failure -R sketch-intersection-regression
  ```
- Keep the full Release build command above as the baseline integration verification alongside the manual scenarios below.

## Manual Validation Scenario A: All bounded regions from the rectangle-circle overlap are selectable

1. Create a sketch on a standard plane.
2. Draw one closed rectangle.
3. Draw one closed circle whose center lies on one corner of the rectangle so that one quarter of the circle lies inside the rectangle.
4. Finish the sketch.
5. Click the rectangle-only region outside the circle.
6. Click the quarter-circle overlap region.
7. Click the remaining three-quarter circle region outside the rectangle.

**Expected result**:
- Each of the three bounded regions becomes selectable independently.
- The three-quarter circle region outside the rectangle is selectable on the first attempt.
- No extra unbounded or phantom region becomes selectable.

**Evidence checklist**:
- [ ] Capture the source sketch showing the intended overlap geometry.
- [ ] Capture one screenshot or recording showing each of the three bounded regions highlighted independently.
- [ ] Add a reviewer note confirming the previously missing three-quarter circle region is now selectable.

## Manual Validation Scenario B: Every selectable overlap region extrudes as a closed profile

1. Reuse the same rectangle-circle overlap sketch.
2. Select the rectangle-only region and start the Extrude tool.
3. Repeat for the quarter-circle overlap region.
4. Repeat for the outer three-quarter circle region.
5. Use a visible distance such as `10 mm` for each attempt.

**Expected result**:
- Each selected region previews and commits as a valid closed profile.
- No attempt fails with an open/incomplete profile error for these bounded regions.
- Only the chosen selected region contributes material for each extrusion.

**Evidence checklist**:
- [ ] Save or paste the successful `cmake --build build --parallel $(nproc)` output used for validation.
- [ ] Capture preview/commit evidence for at least one overlap-generated region.
- [ ] Add reviewer notes confirming all three bounded regions extrude successfully.

## Manual Validation Scenario C: Region availability refreshes after sketch edits or deletion

1. Create the rectangle-circle overlap sketch and confirm all expected regions exist.
2. Re-open/edit the sketch.
3. Move the circle or rectangle so the overlap changes.
4. Finish the edit and attempt region selection again.
5. Delete or undo one of the intersecting shapes and check the available regions once more.

**Expected result**:
- The selectable region set updates to match the current visible bounded areas.
- Removed overlaps no longer expose stale selectable regions.
- Newly created overlap changes are reflected before the next selection attempt.

**Evidence checklist**:
- [ ] Capture the region set before the edit.
- [ ] Capture the region set after the edit.
- [ ] Capture the region set after deletion or undo.
- [ ] Add a reviewer note confirming no stale region indices remain selectable.

## Manual Validation Scenario D: Negative edge cases stay non-misleading

1. Create two closed shapes that only touch tangentially without forming a bounded overlap region.
2. Confirm that no extra overlap face becomes selectable.
3. Create an invalid/open intersection case by editing one boundary so a formerly closed region becomes open.
4. Attempt selection/extrusion again.
5. Optionally create duplicate/coincident partial boundaries and inspect the resulting selectable regions.

**Expected result**:
- Tangent-only contact does not create a phantom selectable face.
- Open/unbounded regions are not selectable or extrudable as sketch faces.
- Duplicate/coincident boundary handling does not hide valid bounded regions or invent false ones.

**Evidence checklist**:
- [ ] Capture the tangent-only case.
- [ ] Capture the invalid/open case outcome.
- [ ] Add reviewer notes describing duplicate/coincident-boundary behavior.

## Review Evidence To Capture

- Successful output from `cmake --build build --target sketch_intersection_regression --parallel $(nproc)` and `ctest --test-dir build --output-on-failure -R sketch-intersection-regression`
- Successful build output from `cmake --build build --parallel $(nproc)`
- Screenshot or recording showing all three bounded overlap regions highlighted independently
- Screenshot or recording showing at least one successful extrusion of an overlap-generated region
- Screenshot or recording proving the region set refreshes after sketch edits or deletion
- Reviewer notes confirming tangent/open/duplicate edge cases remain non-misleading

## Implementation Evidence Log

- **Regression target command**: `cmake --build build --target sketch_intersection_regression --parallel $(nproc)`
- **Regression test command**: `ctest --test-dir build --output-on-failure -R sketch-intersection-regression`
- **Regression verification status**: PASS — automated regression target build and `ctest` execution succeeded locally during implementation on 2026-04-26
- **Build verification command**: `cmake --build build --parallel $(nproc)`
- **Build verification status**: PASS — full project build succeeded locally during implementation on 2026-04-26
- **Manual validation status**: Pending reviewer execution in the desktop app for Scenarios A-D above (not runnable in the headless CLI environment)
- **Reviewer notes placeholder**:
  - Scenario A:
  - Scenario B:
  - Scenario C:
  - Scenario D:
