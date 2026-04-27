# Tasks: Fix Sketch Intersections

**Input**: Design documents from `/specs/005-fix-sketch-intersections/`
**Prerequisites**: `plan.md`, `spec.md`, `research.md`, `data-model.md`, `quickstart.md`

**Tests**: This feature explicitly requires automated regression coverage. Add or update failing automated tests before implementation for each user story, then keep the manual validation and evidence checklist current in `specs/005-fix-sketch-intersections/quickstart.md`.

**Organization**: Tasks are grouped by user story so each story can be implemented and validated independently.

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Confirm build/test wiring and reviewer validation assets before code changes.

- [X] T001 Confirm sketch intersection regression target inputs in tests/CMakeLists.txt and ./CMakeLists.txt
- [X] T002 [P] Refresh reviewer validation and evidence checklist in specs/005-fix-sketch-intersections/quickstart.md

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Establish shared bounded-region/profile infrastructure that blocks all user stories.

**⚠️ CRITICAL**: Complete this phase before starting user story work.

- [X] T003 Extend resolved sketch-region/profile data structures in src/sketch/SketchProfiles.h and src/sketch/SketchPicker.h
- [X] T004 Implement shared bounded-region topology classification and containment helpers in src/sketch/SketchPicker.cpp
- [X] T005 [P] Align sketch-area selection semantics and stale-index clearing in src/document/Document.h and src/document/Document.cpp
- [X] T006 [P] Update completed-sketch overlay consumers for resolved region indices in src/sketch/SketchRenderer.cpp and src/viewport/Renderer.cpp

**Checkpoint**: Region indices, profile payloads, selection semantics, and overlay consumers are ready for story-specific work.

---

## Phase 3: User Story 1 - Select every bounded overlap region (Priority: P1) 🎯 MVP

**Goal**: Make every bounded region formed by overlapping sketch geometry independently selectable.

**Independent Test**: Build the rectangle-plus-corner-circle sketch and verify the rectangle-only region, quarter-circle overlap region, and outer three-quarter circle region each highlight and select independently.

### Verification for User Story 1

- [X] T007 [US1] Add overlap, tangent, open-boundary, and duplicate-boundary fixtures in tests/sketch/SketchIntersectionFixtures.h and tests/sketch/SketchIntersectionFixtures.cpp
- [X] T008 [US1] Add failing bounded-region selection assertions in tests/sketch/SketchIntersectionSelectionTest.cpp
- [X] T009 [P] [US1] Document overlap-region selection checks in specs/005-fix-sketch-intersections/quickstart.md

### Implementation for User Story 1

- [X] T010 [US1] Implement bounded-region hit testing for every selectable overlap face in src/sketch/SketchPicker.cpp
- [X] T011 [US1] Integrate overlap-region hover and highlight behavior in src/sketch/SketchRenderer.cpp and src/viewport/Renderer.cpp

**Checkpoint**: Completed sketches expose all bounded overlap regions as independently selectable sketch faces.

---

## Phase 4: User Story 2 - Extrude any selectable sketch face (Priority: P1)

**Goal**: Treat every selectable bounded region as a closed profile that can be extruded successfully.

**Independent Test**: Reuse the overlap sketch, select each bounded region individually, and confirm each region extrudes without open-profile or incomplete-face failures.

### Verification for User Story 2

- [X] T012 [US2] Add failing resolved-profile and extrusion assertions in tests/sketch/SketchIntersectionExtrudeTest.cpp
- [X] T013 [P] [US2] Document overlap-face extrusion checks in specs/005-fix-sketch-intersections/quickstart.md

### Implementation for User Story 2

- [X] T014 [US2] Resolve selected bounded regions into explicit closed profiles and hole boundaries in src/sketch/SketchPicker.cpp and src/sketch/SketchProfiles.h
- [X] T015 [US2] Build OCCT faces from resolved profile boundaries in src/sketch/SketchToWire.h and src/sketch/SketchToWire.cpp
- [X] T016 [US2] Route sketch-face preview and commit extrusion through resolved profiles in src/sketch/ExtrudeOperation.cpp and src/ui/MainWindow.cpp

**Checkpoint**: Any selectable bounded sketch face can be previewed and extruded as a valid closed profile.

---

## Phase 5: User Story 3 - Keep regions accurate after sketch changes (Priority: P2)

**Goal**: Refresh bounded-region availability after sketch edits, deletion, undo, or overlap removal.

**Independent Test**: Edit or remove one overlapping entity, finish the sketch, and verify only the currently valid bounded regions remain selectable and extrudable.

### Verification for User Story 3

- [X] T017 [US3] Add failing refresh assertions for re-edit, deletion, tangent-only, and open-boundary cases in tests/sketch/SketchIntersectionRefreshTest.cpp
- [X] T018 [P] [US3] Document sketch-edit refresh and negative edge-case checks in specs/005-fix-sketch-intersections/quickstart.md

### Implementation for User Story 3

- [X] T019 [US3] Invalidate stale sketch-area selections when sketches are reactivated or hidden in src/document/Document.h and src/document/Document.cpp
- [X] T020 [US3] Refresh completed-sketch hover and selection state after sketch edits in src/ui/ViewportWidget.cpp and src/viewport/Renderer.cpp

**Checkpoint**: Sketch-region selection stays synchronized with the current completed-sketch geometry after edits.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Finish diagnostics, documentation, and end-to-end validation that span multiple stories.

- [X] T021 Tighten invalid-region and open-profile diagnostics in src/sketch/SketchPicker.cpp, src/sketch/SketchToWire.cpp, and src/sketch/ExtrudeOperation.cpp
- [X] T022 [P] Document finalized overlap-region workflow notes in ./README.md and specs/005-fix-sketch-intersections/quickstart.md
- [X] T023 Update final automated/manual evidence notes in specs/005-fix-sketch-intersections/quickstart.md

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 → Phase 2**: Setup must finish before shared region/profile infrastructure changes begin.
- **Phase 2 → Phases 3-5**: Foundational bounded-region, selection, and overlay work blocks all story phases.
- **Phase 3 (US1)**: Recommended first delivery slice and MVP.
- **Phase 4 (US2)**: Starts after Phase 2; validate after US1 if working sequentially.
- **Phase 5 (US3)**: Starts after Phase 2; close it after US1/US2 behavior is stable.
- **Phase 6**: Starts after the desired user stories are complete.

### User Story Dependencies

- **US1**: No dependency on other stories after Phase 2.
- **US2**: No hard dependency on US1 after Phase 2, but it reuses the same resolved bounded-region infrastructure.
- **US3**: No hard dependency on US1/US2 after Phase 2, but it should be validated against the finished selection/extrusion behavior.

### Recommended Completion Order

1. Phase 1: Setup
2. Phase 2: Foundational
3. Phase 3: US1 (MVP)
4. Phase 4: US2
5. Phase 5: US3
6. Phase 6: Polish

### Parallel Opportunities

- `T001` and `T002` can run together at setup time.
- `T005` and `T006` can run in parallel after `T003`-`T004` land.
- In **US1**, `T007` and `T009` can run in parallel before `T008`/`T010`.
- In **US2**, `T012` and `T013` can run in parallel before `T014`-`T016`.
- In **US3**, `T017` and `T018` can run in parallel before `T019`-`T020`.
- After Phase 2, different engineers can work US1, US2, and US3 concurrently if they coordinate shared-file edits in `src/sketch/SketchPicker.cpp`, `src/viewport/Renderer.cpp`, and `specs/005-fix-sketch-intersections/quickstart.md`.

---

## Parallel Example: User Story 1

```text
T007 Add overlap/tangent/open/duplicate fixtures in tests/sketch/SketchIntersectionFixtures.h and tests/sketch/SketchIntersectionFixtures.cpp
T009 Document overlap-region selection checks in specs/005-fix-sketch-intersections/quickstart.md
```

## Parallel Example: User Story 2

```text
T012 Add resolved-profile and extrusion assertions in tests/sketch/SketchIntersectionExtrudeTest.cpp
T013 Document overlap-face extrusion checks in specs/005-fix-sketch-intersections/quickstart.md
```

## Parallel Example: User Story 3

```text
T017 Add refresh assertions in tests/sketch/SketchIntersectionRefreshTest.cpp
T018 Document sketch-edit refresh checks in specs/005-fix-sketch-intersections/quickstart.md
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Finish Phase 1 and Phase 2.
2. Deliver Phase 3 to make all bounded overlap regions selectable.
3. Run the US1 regression and manual overlap-selection checks.
4. Stop and review before starting extrusion work.

### Incremental Delivery

1. Complete Setup + Foundational.
2. Deliver US1 and validate overlap-region selection.
3. Deliver US2 and validate extrusion for every selectable region.
4. Deliver US3 and validate edit-refresh behavior.
5. Finish Polish and rerun the full regression/manual checklist.

### Parallel Team Strategy

1. One engineer owns shared foundation in `src/sketch/` and `src/document/`.
2. After Phase 2, split work across US1 test/rendering tasks, US2 profile/extrude tasks, and US3 refresh tasks.
3. Merge with shared-file coordination on `src/sketch/SketchPicker.cpp`, `src/viewport/Renderer.cpp`, and `specs/005-fix-sketch-intersections/quickstart.md`.

---

## Notes

- All tasks use the required checklist format with task IDs and exact file paths.
- `[P]` marks tasks that can proceed independently once their prerequisite phase is ready.
- Automated regression tasks are included because the feature spec explicitly requires coverage for selection, extrusion, and refresh behavior.
- No `contracts/` tasks are included because this feature does not add an external interface.
- Do not create branches, commit, push, or tag as part of this task list.
