# Quickstart: Validate Double-Sided Face Rendering

## Prerequisites

1. From the repository root, configure the project:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   ```
2. Build the application:
   ```bash
   cmake --build build --parallel $(nproc)
   ```
3. Launch the desktop app:
   ```bash
   ./build/bin/elcad
   ```

## Verification Scope

This feature validates that extrude-generated faces remain visible from either viewing side without breaking normal depth-based occlusion. Evidence must cover the known missing-face reproduction, full orbit inspection, comparison against an already-correct extrusion, and hidden-surface behavior.

## Implementation Notes

- The planned fix stays in the shared body-surface rendering path so committed bodies and preview meshes are validated the same way.
- The intended behavior is two-sided visibility, not see-through rendering; depth testing must still hide farther surfaces.
- Capture evidence with enough context to prove the same face is visible from the original repro angle, the opposite side, and a continuous orbit pass.
- No `contracts/` artifact is required because this feature changes internal desktop rendering behavior and does not add a public API, CLI, or protocol.

## Manual Validation Scenario A: Known missing-face extrusion now renders from the initial viewpoint

1. Start from the sketch/profile that previously reproduced the missing-face bug.
2. Extrude the profile into a solid using the normal UI workflow.
3. Stop on the original viewpoint that previously showed the gap.

**Expected result**:
- The previously missing face is visible immediately after extrusion.
- The body appears visually complete instead of showing a hole where the face should be.

**Evidence checklist**:
- [ ] Capture the source sketch or body setup used to reproduce the bug.
- [ ] Capture the post-extrusion view that previously showed the missing face.
- [ ] Add reviewer notes confirming the same face is visible from the original repro angle immediately after extrusion.

## Manual Validation Scenario B: The same face stays visible when viewed from the opposite side

1. Reuse the body from Scenario A.
2. Orbit the camera until the same face is viewed from the opposite side/back side.
3. Continue rotating through a full inspection pass around the affected body.

**Expected result**:
- The face remains visible throughout the orbit.
- No orientation-only disappearance occurs as the camera crosses to the other side of the face.

**Evidence checklist**:
- [ ] Capture front-side and back-side views of the same face with matching callouts.
- [ ] Capture a short screen recording or annotated screenshots from the orbit pass showing no orientation-only disappearance.
- [ ] Add reviewer notes confirming SC-002 and SC-003 from the spec.

## Manual Validation Scenario C: Already-correct extrusions stay correct

1. Create or load at least one representative extrusion that already rendered correctly before this change.
2. Inspect the body from several normal viewing angles.
3. Compare the result to the expected baseline appearance.

**Expected result**:
- Exterior surfaces that were already correct remain complete.
- The change does not introduce new missing faces or obvious lighting regressions on unaffected scenes.

**Evidence checklist**:
- [ ] Capture at least one comparison scene that was already rendering correctly.
- [ ] Add reviewer notes confirming no regression was observed in normal exterior views or face-targeted inspection.

## Manual Validation Scenario D: Hidden surfaces still do not show through

1. Use a closed solid or thin-walled extrusion where farther faces would be obvious if depth handling regressed.
2. Orbit to viewpoints where a nearer exterior face should hide farther surfaces.
3. If possible, inspect a thin or mixed-orientation case noted in the spec edge cases.

**Expected result**:
- Nearer surfaces continue to occlude farther faces.
- Two-sided visibility does not create see-through artifacts in closed regions.

**Evidence checklist**:
- [ ] Capture at least one viewpoint where hidden surfaces remain properly occluded.
- [ ] Add reviewer notes confirming FR-004 and SC-004 from the spec with no see-through artifact.

## Evidence To Capture

- Successful output from `cmake -B build -DCMAKE_BUILD_TYPE=Release`
- Successful output from `cmake --build build --parallel $(nproc)`
- Screenshot or recording of the known repro before/after the fix
- Front-side and back-side views of the same extruded face
- Screenshot or recording confirming hidden faces do not show through nearer geometry
- Reviewer notes covering the unaffected comparison scene

## Current Automated Validation

- [ ] Focused automated viewport regression added if implementation makes one practical
- [x] `cmake -B build -DCMAKE_BUILD_TYPE=Release`
- [x] `cmake --build build --parallel $(nproc)`
- [ ] Manual desktop validation scenarios A-D completed and captured by a reviewer
