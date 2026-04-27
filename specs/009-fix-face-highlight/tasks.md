# Tasks: Fix Face Highlight

**Input**: Design documents from `/specs/009-fix-face-highlight/`
**Prerequisites**: `plan.md`, `spec.md`, `research.md`, `data-model.md`, `quickstart.md`

**Tests**: This feature calls for focused regression coverage where a reliable non-visual seam exists, plus explicit manual screenshot-based validation in `specs/009-fix-face-highlight/quickstart.md`.

**Organization**: Tasks are grouped by user story so each story can be implemented and validated independently.

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Confirm the affected face-highlight seams and align the reviewer evidence checklist before code changes.

- [X] T001 Confirm the affected face-selection, whole-body highlight, and extrude-preview seams in `src/document/Document.cpp`, `src/ui/ViewportWidget.cpp`, `src/ui/MainWindow.cpp`, `src/viewport/Renderer.cpp`, and `tests/`
- [X] T002 [P] Align the build, regression, and screenshot evidence checklist in `specs/009-fix-face-highlight/quickstart.md`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Establish shared regression fixtures and a single face-resolution / highlight-state foundation that blocks all user story work.

**⚠️ CRITICAL**: Complete this phase before starting user story work.

- [X] T003 Extend reusable face-selection/extrude regression fixtures in `tests/sketch/SketchIntersectionFixtures.h` and `tests/sketch/SketchIntersectionFixtures.cpp`
- [X] T004 [P] Add or refine shared bounded-face helper declarations in `src/viewport/Renderer.h` and `src/viewport/Renderer.cpp`
- [X] T005 [P] Refine explicit body-highlight state synchronization in `src/document/Document.cpp` and `tests/selection/AdditiveSelectionTest.cpp`
- [X] T006 Reuse the same bounded-face resolution seam across viewport pick and extrude entrypoints in `src/ui/ViewportWidget.cpp` and `src/ui/MainWindow.cpp`

**Checkpoint**: Shared regression fixtures, bounded-face resolution, and explicit body-highlight semantics are ready for story-specific work.

---

## Phase 3: User Story 1 - Show the correct selected face region (Priority: P1) 🎯 MVP

**Goal**: Keep the visible selected-face highlight constrained to the intended bounded face without brightening the whole body when only a face is selected.

**Independent Test**: Select a face near adjacent boundaries, confirm the highlight stays within the intended outline, confirm the rest of the body does not show whole-body selected styling, and verify the extruded result still matches that same face region.

### Verification for User Story 1

- [X] T007 [US1] Add failing bounded-face and no-whole-body-highlight assertions in `tests/sketch/SketchIntersectionSelectionTest.cpp` and `tests/selection/AdditiveSelectionTest.cpp`
- [X] T008 [P] [US1] Document face-boundary and face-only styling validation steps in `specs/009-fix-face-highlight/quickstart.md`

### Implementation for User Story 1

- [X] T009 [US1] Constrain resolved face-selection triangles to the intended bounded face in `src/viewport/Renderer.cpp`
- [X] T010 [US1] Route face click selection through the corrected bounded-face result in `src/ui/ViewportWidget.cpp` and `src/viewport/Renderer.cpp`
- [X] T011 [US1] Prevent face-only selection from enabling whole-body highlight styling in `src/document/Document.cpp` and ensure renderer styling consumers stay aligned in `src/viewport/Renderer.cpp`
- [X] T012 [US1] Keep fallback extrude face construction aligned with the corrected selected triangle set in `src/ui/MainWindow.cpp` and `src/viewport/Renderer.cpp`

**Checkpoint**: Face selection highlights stay inside the intended face boundary, only the selected face is emphasized, and extrusion still uses the same region.

---

## Phase 4: User Story 2 - Keep selection rendering stable during preview (Priority: P2)

**Goal**: Preserve the same selected-face overlay and face-only styling while live extrude preview updates so users can trust the preview state.

**Independent Test**: Select a face, start extrude preview, adjust preview parameters, and confirm the face overlay remains aligned to the same bounded region throughout the preview interaction without reintroducing whole-body highlight styling.

### Verification for User Story 2

- [X] T013 [US2] Add failing preview-alignment and face-only styling assertions in `tests/sketch/SketchIntersectionExtrudeTest.cpp`
- [X] T014 [P] [US2] Document preview-stability and face-only styling evidence capture in `specs/009-fix-face-highlight/quickstart.md`

### Implementation for User Story 2

- [X] T015 [US2] Preserve selected face overlay alignment during live preview recompute in `src/ui/MainWindow.cpp` and `src/viewport/Renderer.cpp`
- [X] T016 [US2] Keep cancel/apply preview cleanup from leaving stale or expanded face highlights in `src/ui/MainWindow.cpp` and `src/ui/ViewportWidget.cpp`
- [X] T017 [US2] Ensure preview updates do not reintroduce whole-body selected styling for face-only selection in `src/document/Document.cpp`, `src/ui/MainWindow.cpp`, and `src/viewport/Renderer.cpp`

**Checkpoint**: Preview updates no longer broaden the selected face overlay or re-enable whole-body highlight styling for face-only selection.

---

## Phase 5: Polish & Cross-Cutting Concerns

**Purpose**: Finish verification, evidence capture, and final workflow documentation.

- [X] T018 [P] Run Release configure/build validation from `specs/009-fix-face-highlight/quickstart.md`
- [X] T019 [P] Run focused `selection_regression` and `sketch_intersection_regression` validation via `tests/CMakeLists.txt` and `specs/009-fix-face-highlight/quickstart.md`
- [ ] T020 Run manual validation scenarios A-C and capture updated screenshots in `specs/009-fix-face-highlight/quickstart.md`
- [ ] T021 [P] Refresh final automated/manual evidence notes in `specs/009-fix-face-highlight/quickstart.md`

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 → Phase 2**: Setup must finish before shared regression-fixture and face-resolution/highlight-state work begins.
- **Phase 2 → Phases 3-4**: The bounded-face seam and explicit body-highlight semantics block all user stories.
- **Phase 3 (US1)**: Recommended MVP and first delivery slice.
- **Phase 4 (US2)**: Starts after Phase 2 and is safest after US1 because it depends on the corrected bounded-face and face-only styling behavior.
- **Phase 5**: Starts after the desired user stories are complete.

### User Story Dependencies

- **US1**: No dependency on other stories after Phase 2.
- **US2**: Depends on the corrected bounded-face and face-only styling behavior from US1 to keep preview rendering aligned with the same selected face.

### Recommended Completion Order

1. Phase 1: Setup
2. Phase 2: Foundational
3. Phase 3: US1 (MVP)
4. Phase 4: US2
5. Phase 5: Polish

### Parallel Opportunities

- `T001` and `T002` can run in parallel during setup.
- `T003`, `T004`, and `T005` can run in parallel during Phase 2 before `T006` closes the shared seam.
- In **US1**, `T007` and `T008` can run in parallel before `T009`-`T012`.
- In **US2**, `T013` and `T014` can run in parallel before `T015`-`T017`.
- `T018` and `T019` can run in parallel once implementation is complete; `T021` follows the manual evidence capture in `T020`.

---

## Parallel Example: User Story 1

```text
T007 Add failing bounded-face and no-whole-body-highlight assertions in tests/sketch/SketchIntersectionSelectionTest.cpp and tests/selection/AdditiveSelectionTest.cpp
T008 Document face-boundary and face-only styling validation steps in specs/009-fix-face-highlight/quickstart.md
```

## Parallel Example: User Story 2

```text
T013 Add failing preview-alignment and face-only styling assertions in tests/sketch/SketchIntersectionExtrudeTest.cpp
T014 Document preview-stability and face-only styling evidence capture in specs/009-fix-face-highlight/quickstart.md
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Finish Phase 1 and Phase 2.
2. Deliver Phase 3 so the selected face highlight stays within the intended boundary and no longer triggers whole-body highlight styling.
3. Run the focused regressions and manual boundary/styling checks for US1.
4. Stop and review before preview-stability work.

### Incremental Delivery

1. Complete Setup + Foundational.
2. Deliver US1 and validate bounded face highlighting plus face-only styling.
3. Deliver US2 and validate preview stability against the same selected face.
4. Finish Polish and rerun the full build/regression/manual checklist.

### Parallel Team Strategy

1. One engineer owns shared renderer and fixture work in `src/viewport/` and `tests/sketch/`.
2. A second engineer can own explicit body-highlight semantics and regression coverage in `src/document/` and `tests/selection/`.
3. After Phase 2, merge with coordination on `src/viewport/Renderer.cpp`, `src/ui/MainWindow.cpp`, `src/ui/ViewportWidget.cpp`, `src/document/Document.cpp`, and `specs/009-fix-face-highlight/quickstart.md`.

---

## Notes

- All tasks use the required checklist format with task IDs, optional `[P]` markers, required `[US#]` labels for story phases, and exact file paths.
- Focused automated regression tasks are included because the spec explicitly allows targeted coverage when a reliable non-visual seam exists.
- No `contracts/` tasks are included because this feature does not add an external interface.
- Do not commit autonomously; leave commit decisions to the user.
