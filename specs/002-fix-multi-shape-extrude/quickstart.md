# Quickstart: Validate Multi-Shape Extrude Planning Output

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

## Manual Validation Scenario A: Multiple disconnected selected faces extrude together

1. Create a sketch on a standard plane.
2. Draw at least three closed, non-overlapping shapes (for example, three rectangles or two rectangles plus one circle).
3. Finish the sketch.
4. Select two or more sketch faces from that same sketch (use the existing multi-select gesture if needed).
5. Start the Extrude tool.
6. Keep **Operation = New Body** and set one shared distance (for example, `10 mm`).
7. Confirm the extrude.

**Expected result**:
- The command completes without `Wire builder failed: edges may not be connected`.
- One separate 3D body is created for each selected face.
- The new bodies stay positioned over their source sketch faces.

**Evidence checklist**:
- [ ] Save or paste the successful `cmake --build build --parallel $(nproc)` output used for validation.
- [ ] Capture a screenshot showing the source sketch, the multi-face selection, and the resulting separate bodies.
- [ ] Add a reviewer note confirming the connectivity error no longer appears.

## Manual Validation Scenario B: Unselected sketch faces remain unchanged

1. Repeat the same sketch setup with at least three disconnected closed faces.
2. Select only a subset of the sketch faces.
3. Run one extrude operation with a shared distance.

**Expected result**:
- Only the selected faces create new 3D bodies.
- Unselected sketch faces remain available in the sketch and do not produce solids.

**Evidence checklist**:
- [ ] Record the selected-face count and confirm the created body count matches it exactly.
- [ ] Capture a screenshot showing the unselected face still present in the sketch after the extrude.
- [ ] Add a reviewer note confirming no extra solids were created.

## Manual Validation Scenario C: Invalid selection fails atomically

1. Prepare a sketch state where one requested face cannot be resolved as a valid closed profile.
2. Attempt the same multi-face extrude workflow.

**Expected result**:
- The command stops with a clear user-facing error.
- No partial new bodies or partial target-body edits remain in the scene.

**Evidence checklist**:
- [ ] Capture the displayed error message for the invalid selection.
- [ ] Confirm body count and target-body state are unchanged after the failed command.
- [ ] Add a reviewer note confirming the failure left no partial geometry behind.

## Review Evidence To Capture

- Successful Release or Debug build output
- Screenshot or short recording of the source sketch, multi-face selection, and resulting separate bodies
- Reviewer notes confirming the previous connectivity error no longer appears, selection scoping stays correct, and invalid selections fail atomically
