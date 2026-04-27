# Quickstart: Validate Multi-Object Selection

## Prerequisites

1. From the repository root, configure the project:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   ```
2. Build the application:
   ```bash
   cmake --build build --parallel $(nproc)
   ```
3. Build and run the focused selection regression target:
   ```bash
   cmake --build build --target selection_regression --parallel $(nproc)
   ctest --test-dir build --output-on-failure -R selection-regression
   ```
4. Launch the desktop app:
   ```bash
   ./build/bin/elcad
   ```

## Verification Scope

This feature validates that selection can accumulate across selectable points, lines, faces, and 3D objects without requiring a modifier key, and that explicit clear gestures remove the full selection set. Evidence should cover both viewport behavior and any downstream UI summaries that depend on the current selection.

## Implementation Notes

- Keep `Document` as the persistent owner of the selection set.
- Change plain-click selection policy in the viewport rather than introducing a second selection model.
- Preserve explicit clear actions through empty-space clicks and the Space shortcut.
- Existing commands that require exactly one body or one sketch selection should continue to validate that requirement explicitly.
- No `contracts/` artifact is required because this feature changes internal desktop CAD behavior and does not introduce a public API, CLI surface, or protocol.

## Manual Validation Scenario A: Plain clicks accumulate whole-object selection

1. Launch elcad with a scene that contains at least two selectable 3D bodies.
2. Click the first body.
3. Click a second body without holding Control.

**Expected result**:
- Both bodies remain selected after the second click.
- The selection summary reflects two selected items.
- The second click does not clear the first selection.

**Evidence checklist**:
- [ ] Capture the first selected body.
- [ ] Capture both bodies selected after the second click.
- [ ] Add reviewer notes confirming FR-001, FR-002, and SC-001 from the spec.

## Manual Validation Scenario B: Mixed selectable types stay selected together

1. Open or create content that exposes at least one selectable point, one line, one face, and one 3D object.
2. Select one type, then click another type without using a modifier key.
3. Continue until at least three mixed-type items are selected together.

**Expected result**:
- Mixed object types remain selected in one shared selection set.
- Highlighting and the selection summary continue to reflect all selected items.
- No previously selected item is lost when another selectable type is clicked.

**Evidence checklist**:
- [ ] Capture one mixed-type multi-selection state.
- [ ] Add reviewer notes confirming FR-003, FR-004, and SC-002 from the spec.

## Manual Validation Scenario C: Clicking empty space clears the entire selection

1. Build a selection containing at least two objects.
2. Click a viewport area that does not hit any selectable object.

**Expected result**:
- The full selection clears immediately.
- No highlight remains on previously selected objects.
- The selection summary returns to the empty state.

**Evidence checklist**:
- [ ] Capture the selection before the empty-space click.
- [ ] Capture the cleared state after the click.
- [ ] Add reviewer notes confirming FR-005, FR-007, FR-008, and SC-003 from the spec.

## Manual Validation Scenario D: Space clears the entire selection

1. Build a selection containing one or more objects.
2. Ensure the viewport has keyboard focus.
3. Press Space.

**Expected result**:
- The full selection clears immediately.
- No unrelated modeling action starts.
- Shortcut guidance, if shown in the UI, matches the new clear-selection behavior.

**Evidence checklist**:
- [ ] Capture the selection before pressing Space.
- [ ] Capture the cleared state after pressing Space.
- [ ] Add reviewer notes confirming FR-006, FR-007, and SC-004 from the spec.

## Manual Validation Scenario E: Single-target commands still require a single valid target

1. Select multiple objects, including at least two bodies if possible.
2. Invoke a command that currently requires one selected body, such as mirror or a body gizmo workflow.

**Expected result**:
- The command still enforces its existing single-target rule instead of silently acting on multiple bodies.
- The user receives the current guardrail behavior (summary state, disabled affordance, or warning) rather than an ambiguous result.

**Evidence checklist**:
- [ ] Capture the multi-selection state before invoking the command.
- [ ] Capture the guarded outcome.
- [ ] Add reviewer notes confirming the feature did not broaden unrelated command scope.

## Evidence To Capture

- Successful output from `cmake -B build -DCMAKE_BUILD_TYPE=Release`
- Successful output from `cmake --build build --parallel $(nproc)`
- Successful output from `cmake --build build --target selection_regression --parallel $(nproc)`
- Successful output from `ctest --test-dir build --output-on-failure -R selection-regression`
- Screenshot or recording showing additive body selection
- Screenshot or recording showing mixed-type multi-selection
- Screenshot or recording showing empty-space clearing
- Screenshot or recording showing Space-key clearing
- Reviewer notes for any single-target command guardrail scenario exercised during review

## Current Automated Validation

**Current session status**:
- Automated validation succeeded in this session after rerunning the documented configure/build/test commands directly.
- The dedicated `selection_regression` target now exists in the regenerated `build/` tree and its CTest entry passed.
- Manual desktop validation scenarios were not run in this session.

- [X] `cmake -B build -DCMAKE_BUILD_TYPE=Release`
- [X] `cmake --build build --parallel $(nproc)`
- [X] `cmake --build build --target selection_regression --parallel $(nproc)`
- [X] `ctest --test-dir build --output-on-failure -R selection-regression`
- [ ] Manual desktop validation scenarios A-E completed and captured by a reviewer

## Contracts

No `contracts/` artifact is required for this feature because it changes internal desktop CAD behavior and does not add a reusable external API, CLI surface, or protocol.
