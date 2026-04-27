# Research: Fix Face Highlight

## Decision 1: Treat this as a renderer-side face-selection visualization bug

**Decision**: Focus the fix on the mesh-based face-selection/highlight pipeline in `viewport/Renderer` and the viewport click path that seeds that selection, rather than on OCCT extrusion generation.

**Rationale**: `MainWindow` uses `Renderer::faceOrdinalForTriangle()` to extract the OCCT face first and only falls back to `buildFaceFromTriangles()` when direct face extraction fails. That matches the reported evidence: the extrusion preview/result is correct even while the rendered selected area overflows. The mismatch therefore points to how the selected face is expanded and drawn, not to the underlying extrusion geometry.

**Alternatives considered**:
- Fix only the OCCT extrusion path: rejected because the reported extrusion output is already correct.
- Move face highlighting into `Document`: rejected because persistent selection identity belongs in `Document`, but highlight rendering and mesh expansion already live in `Renderer`.

## Decision 2: Reconcile face highlight rendering with the intended bounded face, not a broad coplanar triangle flood fill

**Decision**: Revisit `Renderer::expandFaceSelection()` and the filled face-highlight draw path so the selected-triangle set stays bounded to the intended face region instead of expanding across neighboring coplanar or smoothly connected triangles that should not be highlighted together.

**Rationale**: `expandFaceSelection()` currently performs BFS using only per-edge normal similarity and explicitly skips the distance/planarity check. That makes sense for some curved-surface selection workflows, but it also creates a clear path for the visual selection set to grow beyond the intended bounded face. The highlight draw path then renders every triangle in the resulting selection set, which explains the overflow shown in the bug screenshot.

**Alternatives considered**:
- Keep the current flood-fill and only change highlight coloring: rejected because the geometry of the highlighted region would still be wrong.
- Disable face expansion entirely: rejected because some workflows rely on selecting a coherent face region rather than a single tessellation triangle.

## Decision 3: Keep extrusion preview and final-result alignment as the validation anchor

**Decision**: Validate the fix by ensuring the selected face overlay, extrude preview, and final extrusion result all correspond to the same face region in the same reproduction case.

**Rationale**: The reported problem is specifically a mismatch between what the viewport highlights and what extrusion actually uses. The most useful evidence is therefore not just “highlight looks smaller,” but “highlight, preview, and final geometry now agree.”

**Alternatives considered**:
- Validate only the selection overlay before commit: rejected because it would miss regressions where preview/result drift from the corrected overlay.
- Validate only the final extrusion result: rejected because that is already known to be correct and would not prove the visual bug is fixed.

## Decision 4: Use full build verification plus manual screenshot-driven review; no contracts artifact

**Decision**: Keep automated validation at the build/regression-target level and rely on manual viewport review with screenshots for the actual bug confirmation. Do not create a `contracts/` directory.

**Rationale**: The defect is primarily visual, and the repository does not expose an external API or protocol for this workflow. Existing build/test commands are still valuable to catch integration breakage, but reviewer screenshots and manual reproduction remain the authoritative evidence for the highlight mismatch.

**Alternatives considered**:
- Add a new public contract artifact: rejected because this is internal desktop interaction behavior.
- Depend exclusively on automated tests: rejected because the bug is about what the user sees in the viewport and needs direct visual confirmation.

## Decision 5: Do not let face-only selection imply whole-body highlight styling

**Decision**: Restrict `Body::selected` and other whole-body highlight styling to explicit body selections only, even when a face on that body is selected for extrusion.

**Rationale**: Investigation after the first fix attempt showed that face picks were still flipping the owning body's selected flag through `Document::syncBodySelectionFlags()`. That caused the full body to brighten and all body edges to turn gold, creating the appearance that the selected face fill was overflowing even when face-specific state was separate. Keeping body highlight styling tied only to explicit body selection removes that misleading overlay without changing single-target face workflows.

**Alternatives considered**:
- Keep body-wide highlighting for any sub-selection: rejected because it visually obscures whether the selected face itself is bounded correctly.
- Move all highlight-state interpretation into `Renderer`: rejected because `Document` already owns the canonical persistent selection state and should decide whether a body is explicitly selected.
