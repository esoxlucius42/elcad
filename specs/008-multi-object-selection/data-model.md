# Data Model: Multi-Object Selection

## 1. Selectable Object Reference

**Purpose**: Represents one concrete object the user can select in the modeling workflow, matching the existing `Document::SelectedItem` identity fields.

**Fields**:
- `type: enum` — body, face, edge, vertex, sketch point, sketch line, or sketch area
- `bodyId: quint64` — owning body for body/face/edge/vertex selections
- `sketchId: quint64` — owning sketch for sketch entities and sketch areas
- `index: int` — face/edge/vertex index or resolved bounded-region index for sketch areas
- `entityId: quint64` — specific sketch entity identity for point/line or full-circle selections

**Validation rules**:
- The full tuple (`type`, `bodyId`, `sketchId`, `index`, `entityId`) uniquely identifies one selected object.
- Body-related selections must reference an existing body.
- Sketch-related selections must reference an existing sketch or active sketch entity.
- Duplicate references must not be stored twice in the selection set.

**Relationships**:
- Many `Selectable Object Reference` items belong to one `Selection Set`.
- A `Selectable Object Reference` may be consumed by renderer highlighting, property summaries, sketch-face resolution, or single-target commands.

## 2. Selection Set

**Purpose**: The complete ordered collection of currently selected objects that persists until explicitly cleared.

**Fields**:
- `items: ordered list<Selectable Object Reference>`
- `count: int`
- `containsBodySelection: bool`
- `containsSketchSelection: bool`
- `selectedBodyIds: unique list<quint64>`

**Validation rules**:
- The set may contain mixed selectable types.
- Adding a new valid object preserves existing members and increases `count` only when the object was not already selected.
- Clearing the set removes all items and resets any derived whole-body selected flags.
- Consumers that require a narrower shape of selection, such as one selected body or one sketch’s selected faces, must validate that shape explicitly instead of assuming the set is singular.

**Relationships**:
- One `Selection Set` owns many `Selectable Object Reference` entries.
- `Renderer`, `PropertiesPanel`, `BodyListPanel`, and command handlers read from the `Selection Set`.

## 3. Clear Selection Command

**Purpose**: Represents an explicit user action that resets the current selection to empty.

**Fields**:
- `source: enum` — empty-space click, keyboard shortcut, or widget-driven clear
- `requiresViewportFocus: bool`
- `resultingCount: int`

**Validation rules**:
- An empty-space click must resolve only when no selectable object was hit.
- The Space shortcut only applies when the viewport has focus and the user is not typing into another control.
- Executing the command leaves the `Selection Set` empty and removes active selection highlights.

**Relationships**:
- One `Clear Selection Command` transitions one `Selection Set` from any size to empty.

## 4. Selection Consumer State

**Purpose**: Captures how downstream tools interpret the current selection without changing the selection set itself.

**Fields**:
- `singleSelectedBody: optional<quint64>`
- `selectedSketchFaces: optional<SketchFaceSelection>`
- `summaryLabel: QString`
- `gizmoVisible: bool`

**Validation rules**:
- `singleSelectedBody` is present only when exactly one body remains selected.
- `selectedSketchFaces` is present only when the selection resolves to valid sketch areas from one sketch.
- Summary UI must fall back to count-based messaging when the selection is mixed or contains more than one top-level target.

**Relationships**:
- `Selection Consumer State` is derived from one `Selection Set`.
- `MainWindow`, `Renderer`, and UI panels consume this derived state to decide which actions, summaries, and highlights are appropriate.

## State Transitions

- `Empty selection -> click selectable object -> Selection Set contains 1 item`
- `Non-empty selection -> click different selectable object -> Selection Set grows and preserves previous members`
- `Any selection size -> click empty space -> Selection Set becomes empty`
- `Any selection size -> press Space with viewport focus -> Selection Set becomes empty`
- `Selection Set with exactly one body -> single-body command remains eligible`
- `Selection Set with multiple or mixed items -> consumers fall back to summary/highlight state or warn when a narrower command target is required`
