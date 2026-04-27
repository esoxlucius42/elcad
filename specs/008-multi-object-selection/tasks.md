# Tasks: Multi-Object Selection

**Input**: Design documents from `/specs/008-multi-object-selection/`
**Prerequisites**: `plan.md`, `spec.md`, `research.md`, `data-model.md`, `quickstart.md`

**Tests**: This feature calls for focused regression coverage when practical. Add a narrow selection regression target for document-facing selection semantics, and keep the manual validation/evidence checklist current in `specs/008-multi-object-selection/quickstart.md`.

**Organization**: Tasks are grouped by user story so each story can be implemented and validated independently.

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Confirm the affected selection surfaces and reviewer validation assets before code changes.

- [X] T001 Confirm affected selection entry points in `src/ui/ViewportWidget.cpp`, `src/document/Document.cpp`, `src/viewport/Renderer.cpp`, `src/ui/BodyListPanel.cpp`, and `src/ui/MainWindow.cpp`
- [X] T002 [P] Align reviewer validation scope and evidence targets in `specs/008-multi-object-selection/quickstart.md`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Establish shared selection-consumer and regression infrastructure that blocks all user stories.

**⚠️ CRITICAL**: Complete this phase before starting user story work.

- [X] T003 Create focused selection regression target wiring in `tests/CMakeLists.txt` and `tests/selection/SelectionRegressionMain.cpp`
- [X] T004 [P] Add reusable selection fixtures and helpers in `tests/selection/SelectionFixtures.h` and `tests/selection/SelectionFixtures.cpp`
- [X] T005 Refine shared single-target selection helpers and body-flag semantics in `src/document/Document.h` and `src/document/Document.cpp`
- [X] T006 [P] Update shared multi-selection consumers and guardrails in `src/viewport/Renderer.cpp`, `src/ui/BodyListPanel.cpp`, `src/ui/PropertiesPanel.cpp`, and `src/ui/MainWindow.cpp`

**Checkpoint**: Shared selection helpers, consumer guardrails, and focused regression wiring are ready for story-specific work.

---

## Phase 3: User Story 1 - Build a multi-selection set (Priority: P1) 🎯 MVP

**Goal**: Let each click on another selectable object add that target to the current selection instead of replacing the previous selection.

**Independent Test**: Select one object, click a different selectable object without holding a modifier, and confirm both remain selected. Repeat with mixed selectable types and confirm the selection summary still reflects all selected items.

### Verification for User Story 1

- [X] T007 [US1] Add failing additive-selection assertions in `tests/selection/AdditiveSelectionTest.cpp`
- [X] T008 [P] [US1] Document additive multi-selection checks in `specs/008-multi-object-selection/quickstart.md`

### Implementation for User Story 1

- [X] T009 [US1] Make selectable hit handling additive for body, face, and completed-sketch picks in `src/ui/ViewportWidget.cpp`
- [X] T010 [US1] Preserve deduplicated mixed-type selection membership during additive picks in `src/document/Document.cpp`
- [X] T011 [US1] Present multi-body selection without ambiguous single-body gizmo targeting in `src/viewport/Renderer.cpp` and `src/ui/MainWindow.cpp`
- [X] T012 [US1] Keep docked selection summaries aligned with additive selection state in `src/ui/BodyListPanel.cpp` and `src/ui/PropertiesPanel.cpp`

**Checkpoint**: Clicking additional selectable objects grows the active selection set instead of replacing it, and the UI reflects the accumulated selection correctly.

---

## Phase 4: User Story 2 - Clear selection by clicking empty space (Priority: P2)

**Goal**: Let a click on empty space clear the full selection set in one step.

**Independent Test**: Build a selection containing at least two items, click a viewport area that does not resolve to any selectable object, and confirm all selections and highlights are cleared.

### Verification for User Story 2

- [X] T013 [US2] Add failing empty-space clear-selection assertions in `tests/selection/EmptySpaceClearTest.cpp`
- [X] T014 [P] [US2] Document empty-space deselection checks in `specs/008-multi-object-selection/quickstart.md`

### Implementation for User Story 2

- [X] T015 [US2] Clear the full selection only on true miss paths in `src/ui/ViewportWidget.cpp`
- [X] T016 [US2] Reset body highlights and sketch-selection state consistently for empty-space clears in `src/document/Document.cpp`
- [X] T017 [US2] Refresh empty-selection summaries after miss-based clearing in `src/ui/BodyListPanel.cpp` and `src/ui/PropertiesPanel.cpp`

**Checkpoint**: Clicking empty space reliably clears the full selection set and returns the UI to the empty-selection state.

---

## Phase 5: User Story 3 - Clear selection with Space (Priority: P3)

**Goal**: Let the Space shortcut clear the current selection set without triggering unrelated viewport actions.

**Independent Test**: Build a selection, ensure the viewport has focus, press Space, and confirm the full selection clears immediately while no unrelated action starts.

### Verification for User Story 3

- [X] T018 [US3] Add failing keyboard clear-selection assertions in `tests/selection/ShortcutClearTest.cpp`
- [X] T019 [P] [US3] Document Space shortcut validation in `specs/008-multi-object-selection/quickstart.md`

### Implementation for User Story 3

- [X] T020 [US3] Route Space to clear the active selection set without conflicting with existing viewport shortcuts in `src/ui/ViewportWidget.cpp`
- [X] T021 [US3] Update in-app shortcut guidance for Space-based clear-selection in `src/ui/ShortcutsDialog.cpp`

**Checkpoint**: Pressing Space with viewport focus clears the full selection set and the shortcut guidance matches the implemented behavior.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Finish validation, workflow safety, and cross-story documentation.

- [X] T022 Harden single-target body-command guardrails for multi-selection in `src/ui/MainWindow.cpp` and `src/viewport/Renderer.cpp`
- [X] T023 [P] Run Release configure/build validation from `specs/008-multi-object-selection/quickstart.md`
- [X] T024 [P] Run the focused `selection_regression` target and `ctest --test-dir build --output-on-failure -R selection-regression` from `tests/CMakeLists.txt` and `specs/008-multi-object-selection/quickstart.md`
- [ ] T025 Run manual validation scenarios A-E and capture evidence in `specs/008-multi-object-selection/quickstart.md`
- [X] T026 [P] Refresh final automated/manual evidence notes in `specs/008-multi-object-selection/quickstart.md`

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 → Phase 2**: Setup must finish before shared regression and selection-consumer infrastructure changes begin.
- **Phase 2 → Phases 3-5**: Foundational selection-helper and regression wiring blocks all user stories.
- **Phase 3 (US1)**: Recommended MVP and first delivery slice.
- **Phase 4 (US2)**: Starts after Phase 2; safest after US1 if working sequentially because it reuses the same viewport/document selection flow.
- **Phase 5 (US3)**: Starts after Phase 2; can proceed independently once viewport clear-selection semantics are stable.
- **Phase 6**: Starts after the desired user stories are complete.

### User Story Dependencies

- **US1**: No dependency on other stories after Phase 2.
- **US2**: No hard dependency on US1 after Phase 2, but it touches the same viewport/document clear-selection path and is safest after US1 stabilizes additive selection behavior.
- **US3**: No hard dependency on US1 or US2 after Phase 2, but it shares `ViewportWidget.cpp` with both stories and should merge after those edits are coordinated.

### Recommended Completion Order

1. Phase 1: Setup
2. Phase 2: Foundational
3. Phase 3: US1 (MVP)
4. Phase 4: US2
5. Phase 5: US3
6. Phase 6: Polish

### Parallel Opportunities

- `T001` and `T002` can run in parallel during setup.
- `T004` and `T006` can run in parallel after `T003` and `T005` are underway.
- In **US1**, `T007` and `T008` can run in parallel before `T009`-`T012`.
- In **US2**, `T013` and `T014` can run in parallel before `T015`-`T017`.
- In **US3**, `T018` and `T019` can run in parallel before `T020`-`T021`.
- `T023` and `T024` can run in parallel once implementation is complete; `T026` follows evidence capture.
- After Phase 2, different engineers can work on US1, US2, and US3 concurrently if they coordinate shared-file edits in `src/ui/ViewportWidget.cpp`, `src/document/Document.cpp`, `src/viewport/Renderer.cpp`, `src/ui/BodyListPanel.cpp`, `src/ui/PropertiesPanel.cpp`, and `specs/008-multi-object-selection/quickstart.md`.

---

## Parallel Example: User Story 1

```text
T007 Add failing additive-selection assertions in tests/selection/AdditiveSelectionTest.cpp
T008 Document additive multi-selection checks in specs/008-multi-object-selection/quickstart.md
```

## Parallel Example: User Story 2

```text
T013 Add failing empty-space clear-selection assertions in tests/selection/EmptySpaceClearTest.cpp
T014 Document empty-space deselection checks in specs/008-multi-object-selection/quickstart.md
```

## Parallel Example: User Story 3

```text
T018 Add failing keyboard clear-selection assertions in tests/selection/ShortcutClearTest.cpp
T019 Document Space shortcut validation in specs/008-multi-object-selection/quickstart.md
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Finish Phase 1 and Phase 2.
2. Deliver Phase 3 so plain clicks build a multi-selection set.
3. Run the US1 regression and manual additive-selection checks.
4. Stop and review before starting deselection and shortcut work.

### Incremental Delivery

1. Complete Setup + Foundational.
2. Deliver US1 and validate additive multi-selection.
3. Deliver US2 and validate empty-space clearing.
4. Deliver US3 and validate the Space shortcut.
5. Finish Polish and rerun the full build/regression/manual checklist.

### Parallel Team Strategy

1. One engineer owns shared selection-helper and regression wiring in `src/document/`, `src/viewport/`, and `tests/selection/`.
2. After Phase 2, split work across US1 additive picks, US2 miss-based clearing, and US3 shortcut clearing.
3. Merge with shared-file coordination on `src/ui/ViewportWidget.cpp`, `src/document/Document.cpp`, `src/viewport/Renderer.cpp`, `src/ui/BodyListPanel.cpp`, `src/ui/PropertiesPanel.cpp`, `src/ui/MainWindow.cpp`, and `specs/008-multi-object-selection/quickstart.md`.

---

## Notes

- All tasks use the required checklist format with task IDs, optional `[P]` markers, required `[US#]` labels for story phases, and exact file paths.
- Focused automated regression tasks are included because the spec and plan call for regression coverage when a narrow non-GUI seam is practical.
- No `contracts/` tasks are included because this feature does not add an external interface.
- Branch creation is allowed when the workflow needs it; do not commit, push, or tag as part of this task list.
