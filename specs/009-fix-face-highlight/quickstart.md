# Quickstart: Validate Face Highlight Fix

## Prerequisites

1. From the repository root, configure the project:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   ```
2. Build the application:
   ```bash
   cmake --build build --parallel $(nproc)
   ```
3. Run focused regression targets already present in the repository as baseline verification:
   ```bash
    cmake --build build --target selection_regression --parallel $(nproc)
    ctest --test-dir build --output-on-failure -R selection-regression
    cmake --build build --target sketch_intersection_regression --parallel $(nproc)
    ctest --test-dir build --output-on-failure -R sketch-intersection-regression
   ```
4. Launch the desktop app:
   ```bash
   ./build/bin/elcad
   ```

## Verification Scope

This fix targets a visual mismatch where face-selection highlighting for extrusion overflows the intended boundary even though the extrusion preview and final result are correct. Validation must prove that the visible selected face, the extrusion preview, and the final extruded solid now refer to the same bounded region.

## Manual Validation Scenario A: Reproduce the reported buggy highlight

1. Open the geometry that previously reproduced the bug.
2. Select the face intended for extrusion.
3. Compare the current viewport highlight to the previously captured `selection-bug.png`.

**Expected result**:
- The corrected highlight stays within the intended face boundary.
- The previous overflow into adjacent space no longer appears.
- The rest of the body does not brighten or show whole-body selected edge styling when only a face is selected.

## Manual Validation Scenario B: Confirm highlight matches preview

1. With the same face selected, start the extrusion workflow.
2. Observe the live preview while changing preview parameters.

**Expected result**:
- The highlighted face remains aligned to the same bounded region.
- The extrude preview corresponds to that same region throughout the interaction.
- Preview updates do not reintroduce whole-body selection styling for a face-only selection.

## Manual Validation Scenario C: Confirm highlight matches final result

1. Commit the extrusion after previewing it.
2. Compare the result to the previously correct outcome shown in `preview-ok.png` and the final 3D-object screenshot referenced in the bug report.

**Expected result**:
- The final extrusion matches the highlighted face region.
- There is no mismatch between what was shown as selected and what was actually extruded.

## Evidence To Capture

- Successful output from `cmake -B build -DCMAKE_BUILD_TYPE=Release`
- Successful output from `cmake --build build --parallel $(nproc)`
- Successful output from `cmake --build build --target selection_regression --parallel $(nproc)`
- Successful output from `ctest --test-dir build --output-on-failure -R selection-regression`
- Successful output from `cmake --build build --target sketch_intersection_regression --parallel $(nproc)`
- Successful output from `ctest --test-dir build --output-on-failure -R sketch-intersection-regression`
- Updated screenshot replacing the old overflow case from `selection-bug.png`
- Screenshot or recording showing the corrected highlight and matching preview
- Screenshot or recording showing the corrected highlight and final 3D result agreement

## Current Validation Status

- **Automated validation**: Succeeded on 2026-04-27 for Release configure/build plus both `selection_regression` and `sketch_intersection_regression`.
- **Manual validation**: Still required. Scenario A-C screenshots and final reviewer evidence have not been captured in-session yet.

## Contracts

No `contracts/` artifact is required for this feature because it changes internal desktop rendering behavior and does not add a reusable external API, CLI surface, or protocol.
