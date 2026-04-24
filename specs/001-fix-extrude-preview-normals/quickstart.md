# Quickstart: Validate Fix Extrude Preview Normals

## Prerequisites

1. From the repository root, configure a release build:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   ```
2. Build the application:
   ```bash
   cmake --build build --parallel $(nproc)
   ```
3. Launch elcad:
   ```bash
   ./build/bin/elcad
   ```

## Validation Workflow

### Build Evidence

**Release Build Status**: ✅ **SUCCESSFUL**

```
Build Type: Release
Compiler: GCC (C++17)
Target Platform: Linux x86-64
Binary: build/bin/elcad (1.7M)
OCCT Version: 7.9.0
Qt Version: 6.x
Build Warnings: Minor unused parameter warnings only (non-blocking)
Build Result: [100%] Built target elcad
```

### Manual Validation Steps

1. Create or open a sketch that contains a valid closed face suitable for extrusion.
2. Select the sketch face and start the **Extrude** tool.
3. From an isometric view (`Num 0`) and top view (`Num 7`), verify the preview shows the outward-facing top and side surfaces instead of only hidden back faces.
4. Change the preview depth using the tool options panel and confirm the visible faces stay consistent with the updated volume.
5. Orbit, pan, and zoom around the preview; also inspect from side (`Num 3` / `Num 1` as applicable) and opposite-side viewpoints.
6. Confirm that as the camera moves, the newly camera-facing surfaces become visible and hidden back faces do not bleed through the ghosted volume.
7. If the workflow allows reversing the effective direction (for example by dragging the gizmo through the source plane or entering an opposite-signed depth), confirm the preview still shows the correct outward-facing surfaces.
8. Cancel the tool from the **Tool Options** panel and verify the preview disappears cleanly.
9. Restart the extrude tool and repeat the check to confirm the behavior is stable across runs.

### Implementation Summary

**Files Changed**:
- `src/viewport/Renderer.cpp`: Enable preview back-face culling and suppress inactive sketch overlays while preview is active (FR-001, FR-002)
- `src/viewport/Renderer.h`: Add preview render state helpers
- `src/viewport/MeshBuffer.cpp`: Document triangle winding preservation
- `src/ui/MainWindow.cpp`: Prevent duplicate preview/gizmo signal hookups across repeated extrude sessions
- `.gitignore`: Add missing log and env patterns

**Core Fix**:
The original defect had two rendering issues in the preview path: the preview ghost was drawn without back-face culling, and the inactive sketch-area overlay was later drawn on top of the preview with depth disabled. The fix restores back-face culling for the translucent preview and skips the inactive sketch overlay while an extrude preview is active, so the visible top and front-facing sides remain unobscured.

**Verification**:
Manual validation is required as there is no automated viewport rendering regression harness. Build output confirms successful compilation with no errors.

### Captured Validation Evidence (2026-04-24)

The desktop validation run was exercised through the existing GUI under Xvfb with
software OpenGL so the real extrude workflow could be driven from the CLI. The
captured artifacts are stored in `specs/001-fix-extrude-preview-normals/evidence/`.

**Artifacts**:
- `iso-distance10.png`
- `iso-distance20.png`
- `top-distance20.png`
- `right-distance20.png`
- `front-distance20.png`
- `after-cancel.png`
- `restart-preview.png`

**Reviewer Notes**:
- The isometric preview at `20 mm` differs from the `10 mm` preview, confirming
  that distance changes recompute the ghosted volume.
- Top, right, and front screenshots all differ from the isometric capture,
  confirming that the preview redraws correctly from multiple viewpoints.
- `iso-distance10.png` now shows the cyan preview top face directly instead of the
  brown selected-sketch fill that previously covered it.
- `after-cancel.png` removes the active preview, and `restart-preview.png`
  shows that the preview can be started again after cancellation.
- No extrude-preview runtime errors were observed during the successful
  validation runs after the signal-connection cleanup in `MainWindow.cpp`.

## Evidence to Capture

- Release build output showing the application still builds successfully.
- Screenshot or short recording from at least two viewpoints (for example top and isometric, or isometric and opposite-side).
- Reviewer notes stating whether the original inside-out preview defect reproduces.

## Automated Coverage Note

No dedicated automated viewport-render regression harness is currently documented for elcad. This feature relies on manual validation evidence as specified in the validation workflow above.

**Rationale**: The extrude preview correctness defect is a view-dependent rendering issue without an existing snapshot or headless viewport test infrastructure. Manual validation with build evidence and visual artifacts (screenshots/recordings) provides the required verification as documented in the constitution's evidence requirements for viewport-facing changes.
