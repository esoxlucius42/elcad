# Research: Multi-Object Selection

## Decision 1: Keep `Document` as the only persistent owner of the selection set

**Decision**: Reuse the existing `Document::SelectedItem` vector and selection helper APIs as the authoritative selection model instead of introducing a separate selection manager or per-widget shadow state.

**Rationale**: `Document` already supports mixed-type multi-selection through `addSelection()`, `removeSelection()`, `toggleSelection()`, `selectionItems()`, and `selectionChanged()`. `Renderer`, `PropertiesPanel`, `BodyListPanel`, and command entry points already consume document selection state, so the current problem is the click policy feeding that model, not the absence of a multi-selection-capable data structure.

**Alternatives considered**:
- Create a new global selection controller: rejected because it would duplicate state that `Document` already owns and increase the risk of UI/model drift.
- Keep whole-body selection in `Body::selected()` and rebuild everything else around it: rejected because mixed sketch/body selection already depends on `selectionItems()` as the richer source of truth.

## Decision 2: Make plain selectable clicks additive; keep deselection explicit

**Decision**: Treat a plain click on any selectable object as an additive action, and reserve full-selection clearing for explicit clear gestures: clicking empty space or pressing Space.

**Rationale**: The feature spec explicitly says a second click on another selectable object should leave both selected, and it separately defines the two clear-selection gestures. `ViewportWidget::handlePickClick()` is the current seam that still clears selection on plain hits, while `Document::clearSelection()` already centralizes body-flag resets and `selectionChanged()` emission for explicit clears.

**Alternatives considered**:
- Keep Control as the additive modifier and only update docs: rejected because it does not satisfy the requested behavior change.
- Toggle an already-selected item off on plain click: rejected because the spec does not ask for click-to-remove behavior and that would widen scope into selection editing semantics.
- Preserve replace-selection for some object types: rejected because the requested rule is type-agnostic across points, lines, faces, and 3D objects.

## Decision 3: Preserve command-specific single-target validation instead of weakening tool rules

**Decision**: Leave commands such as mirror, gizmo transforms, and sketch-specific operations responsible for validating whether the current selection is acceptable, rather than assuming additive selection should make every command operate on multiple items.

**Rationale**: The codebase already contains explicit single-target queries such as `Document::singleSelectedBody()` and sketch-face resolution helpers. Changing core selection accumulation should not silently turn single-body operations into multi-body operations; instead, those commands should continue to require exactly one valid target and present the existing warning or summary behavior when that requirement is not met.

**Alternatives considered**:
- Automatically apply every body operation to all selected bodies: rejected because it changes command scope well beyond the selection request.
- Implicitly choose the last selected object for all single-target tools: rejected because it hides ambiguity and makes multi-selection unpredictable for users.

## Decision 4: Pair focused regression coverage with manual viewport validation; omit contracts

**Decision**: Plan a small focused regression target for document-level selection semantics and any extracted click-policy helper, and pair it with manual validation for mixed viewport selections, empty-space clearing, and the Space shortcut. Do not create a `contracts/` artifact.

**Rationale**: The repository’s existing automated coverage is lightweight, but this feature’s core semantics are testable without full GUI automation if the additive/clear rules are kept in or near document-facing helpers. The feature does not define an external protocol or reusable API, so a contracts artifact would add ceremony without describing a meaningful integration surface.

**Alternatives considered**:
- Manual validation only: rejected because selection accumulation and clear semantics are deterministic enough to justify a focused regression seam.
- Full Qt event-driven GUI automation: rejected because it adds disproportionate complexity relative to the narrow behavior change.
- Create a UI contract document: rejected because this remains internal desktop interaction behavior rather than a public interface.
