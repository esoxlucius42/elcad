# Quickstart: Validate Sketch Hole Selection Planning Output

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

- No dedicated automated regression harness currently covers completed-sketch hole picking or extrude-with-hole preview behavior in elcad.
- Use the build command above plus the manual scenarios below as the required validation evidence for this feature.

## Manual Validation Scenario A: Annular region selects and extrudes as a face with a hole

1. Create a sketch on a standard plane.
2. Draw one closed rectangle.
3. Draw one closed circle fully inside the rectangle.
4. Finish the sketch.
5. Click the area inside the rectangle but outside the circle.
6. Start the Extrude tool.
7. Keep **Operation = New Body** and set a visible distance such as `10 mm`.
8. Confirm the extrude.

**Expected result**:
- The sketch-area selection corresponds to the rectangle-with-hole region, not a fully filled rectangle.
- Extrude preview shows a plate/frame with a circular opening.
- The committed 3D result preserves the circular void.

**Evidence checklist**:
- [ ] Save or paste the successful `cmake --build build --parallel $(nproc)` output used for validation.
- [ ] Capture a screenshot or recording of the sketch, the selected annular region, and the preserved circular opening in the result.
- [ ] Add a reviewer note confirming the previous filled-rectangle behavior no longer appears.
- [ ] Confirm the selection does not silently collapse to the outer rectangle boundary during preview or after commit.

## Manual Validation Scenario B: Clicking inside the hole does not select the surrounding face

1. Reuse the rectangle-with-circle sketch.
2. Click once inside the circular hole.
3. If needed, compare with a click in the annular region outside the circle.
4. Start the Extrude tool only after the hole click outcome is clear.

**Expected result**:
- Clicking inside the circle does not select the surrounding rectangle face from that click.
- No misleading filled-face preview appears from a hole click.
- Existing edge-selection precedence may still apply only when the pointer is on the boundary itself.

**Evidence checklist**:
- [ ] Capture the selection state after a click inside the hole.
- [ ] Capture a comparison screenshot/video showing the different outcome between annular-region and hole-region clicks.
- [ ] Add a reviewer note confirming FR-003 behavior.
- [ ] Confirm any warning shown for invalid bounded faces explains that hole regions are not extrudable material.

## Manual Validation Scenario C: Multiple enclosed holes remain excluded

1. Create a new sketch with one closed outer rectangle.
2. Add two or three fully enclosed circles inside it.
3. Finish the sketch.
4. Click a usable area inside the rectangle but outside every circle.
5. Extrude the selected face.

**Expected result**:
- Selection still resolves to one bounded face.
- Every enclosed circle remains excluded in preview and final geometry.
- No extra material appears in any hole region.

**Evidence checklist**:
- [ ] Capture the sketch showing all inner loops.
- [ ] Capture the extrude preview and final geometry preserving every hole.
- [ ] Add a reviewer note confirming all enclosed voids stayed open.
- [ ] Confirm the result is created as one bounded face selection rather than separate filled islands.

## Manual Validation Scenario D: Invalid bounded-face geometry stays non-misleading

1. Create or edit a sketch so the outer and inner loops no longer define a valid bounded face (for example, make a loop open or make an inner loop intersect the outer boundary).
2. Attempt the same selection/extrude workflow.

**Expected result**:
- The application avoids presenting a misleading filled face.
- Extrude fails clearly or selection is unavailable, following the existing invalid-geometry behavior.
- No unintended filled solid is committed.

**Evidence checklist**:
- [ ] Capture the displayed selection/extrude outcome for the invalid sketch state.
- [ ] Confirm no new unintended body or filled region appears.
- [ ] Add a reviewer note describing the failure mode.

## Review Evidence To Capture

- Successful build output
- Screenshot or short recording of rectangle-with-circle selection and resulting preserved hole
- Screenshot or recording proving a click inside the hole does not select the surrounding face
- Screenshot or recording of the multi-hole scenario
- Reviewer notes confirming the bug is fixed and invalid geometry does not produce misleading filled material

## Implementation Evidence Log

- **Build verification**: `cmake --build build --parallel $(nproc)` succeeded from the repository root on 2026-04-25.
- **Build target reference**: `/var/home/esox/dev/cpp/elcad/CMakeLists.txt`
- **Build output summary**:
  ```text
  [100%] Built target elcad
  ```
- **Manual validation status**: Pending reviewer execution in the desktop app for Scenarios A-D above.
- **Reviewer notes placeholder**:
  - Scenario A:
  - Scenario B:
  - Scenario C:
  - Scenario D:
