# Tasks: Fix Extrude Shape Fidelity

**Input**: Design documents from `/specs/007-fix-extrude-shape-fidelity/`
**Prerequisites**: `plan.md`, `spec.md`, `research.md`, `data-model.md`, `quickstart.md`

**Tests**: This feature explicitly calls for focused regression coverage when practical. Add or update failing automated extrusion-fidelity checks in the existing sketch regression harness before closing each user story, and keep the manual validation/evidence checklist current in `specs/007-fix-extrude-shape-fidelity/quickstart.md`.

**Organization**: Tasks are grouped by user story so each story can be implemented and validated independently.

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Confirm the existing regression harness and reviewer validation assets for extrusion-fidelity work before code changes.

- [X] T001 Confirm extrusion-fidelity regression wiring in `tests/CMakeLists.txt` and `tests/sketch/SketchIntersectionRegressionMain.cpp`
- [ ] T002 [P] Align reviewer validation scope and evidence targets in `specs/007-fix-extrude-shape-fidelity/quickstart.md`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Establish shared profile-boundary and regression infrastructure that blocks all user stories.

**⚠️ CRITICAL**: Complete this phase before starting user story work.

- [ ] T003 Extend shared profile-boundary declarations for arc direction and hole fidelity in `src/sketch/SketchProfiles.h` and `src/sketch/SketchPicker.h`
- [X] T004 Implement shared arc-direction preservation and boundary-merging helpers in `src/sketch/SketchPicker.cpp`
- [X] T005 [P] Add reusable curved-profile and hole-profile fixtures in `tests/sketch/SketchIntersectionFixtures.h` and `tests/sketch/SketchIntersectionFixtures.cpp`
- [ ] T006 [P] Wire shared extrusion-fidelity regression entry points in `tests/sketch/SketchIntersectionRegressionMain.cpp` and `tests/CMakeLists.txt`

**Checkpoint**: Shared profile metadata, arc-orientation handling, reusable fixtures, and regression wiring are ready for story-specific work.

---

## Phase 3: User Story 1 - Preserve selected profile shape (Priority: P1) 🎯 MVP

**Goal**: Make a single selected sketch face extrude into a solid that matches the source face outline, including curved boundaries.

**Independent Test**: Select one valid closed sketch face with straight and curved boundaries, extrude it, and verify that the resulting solid follows the same outline instead of collapsing into a wedge or other simplified shape.

### Verification for User Story 1

- [X] T007 [US1] Add failing single-profile curved-boundary extrusion assertions in `tests/sketch/SketchIntersectionExtrudeTest.cpp`
- [ ] T008 [P] [US1] Document single-profile shape-fidelity checks in `specs/007-fix-extrude-shape-fidelity/quickstart.md`

### Implementation for User Story 1

- [X] T009 [US1] Preserve source arc direction and outer-loop ordering while resolving selected profiles in `src/sketch/SketchPicker.cpp`
- [X] T010 [US1] Rebuild planar faces from resolved curved boundaries in `src/sketch/SketchToWire.h` and `src/sketch/SketchToWire.cpp`
- [ ] T011 [US1] Route single-face extrude preview and commit through the corrected profile-face path in `src/sketch/ExtrudeOperation.cpp` and `src/ui/MainWindow.cpp`
- [ ] T012 [US1] Add user-facing error handling and diagnostics for single-face fidelity failures in `src/sketch/SketchToWire.cpp` and `src/sketch/ExtrudeOperation.cpp`

**Checkpoint**: A single selected sketch face extrudes into a solid whose outline matches the selected profile, including curved segments.

---

## Phase 4: User Story 2 - Keep each extrusion aligned to its source face (Priority: P2)

**Goal**: Preserve per-profile identity, placement, and internal voids when multiple sketch faces are extruded together.

**Independent Test**: Select multiple separated sketch faces while leaving at least one valid face unselected, extrude the selected faces together, and verify that each resulting solid stays aligned with its own source face, preserves its own voids, and leaves unselected faces unchanged.

### Verification for User Story 2

- [X] T013 [US2] Add failing multi-profile alignment and hole-preservation assertions in `tests/sketch/SketchIntersectionExtrudeTest.cpp`
- [ ] T014 [P] [US2] Document multi-face alignment and internal-void checks in `specs/007-fix-extrude-shape-fidelity/quickstart.md`
- [X] T015 [P] [US2] Add failing assertions that unselected sketch faces remain unchanged during multi-face extrusion in `tests/sketch/SketchIntersectionExtrudeTest.cpp`

### Implementation for User Story 2

- [ ] T016 [US2] Preserve child hole boundaries and per-profile identity during selected-profile resolution in `src/sketch/SketchPicker.cpp` and `src/sketch/SketchProfiles.h`
- [ ] T017 [US2] Ensure planar face construction keeps internal holes attached to the correct profile in `src/sketch/SketchToWire.cpp`
- [ ] T018 [US2] Ensure batch extrusion preserves one-result-per-profile mapping in `src/sketch/ExtrudeOperation.cpp`
- [ ] T019 [US2] Keep multi-face extrude preview and commit orchestration aligned with corrected profile selection and leave unselected faces unaffected in `src/ui/MainWindow.cpp` and `src/ui/ExtrudeDialog.cpp`

**Checkpoint**: Multi-face extrusion produces one faithful solid per selected face, with no geometry borrowing and no filled-in internal voids.

---

## Phase 5: Polish & Cross-Cutting Concerns

**Purpose**: Finish diagnostics, documentation, and full validation that span both stories.

- [ ] T020 Tighten final fidelity-failure diagnostics across `src/sketch/SketchPicker.cpp`, `src/sketch/SketchToWire.cpp`, and `src/sketch/ExtrudeOperation.cpp`
- [X] T021 [P] Run the Release configure/build validation documented in `specs/007-fix-extrude-shape-fidelity/quickstart.md`
- [X] T022 [P] Run `sketch_intersection_regression` and `ctest --test-dir build --output-on-failure -R sketch-intersection-regression` from `tests/CMakeLists.txt` and `specs/007-fix-extrude-shape-fidelity/quickstart.md`
- [ ] T023 Run manual validation scenarios A-E and capture required evidence in `specs/007-fix-extrude-shape-fidelity/quickstart.md`
- [ ] T024 [P] Refresh completed automated/manual evidence notes in `specs/007-fix-extrude-shape-fidelity/quickstart.md`
- [ ] T025 Update finalized workflow guidance and plan references in `specs/007-fix-extrude-shape-fidelity/quickstart.md` and `.github/copilot-instructions.md`

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 → Phase 2**: Setup must finish before shared profile and regression infrastructure changes begin.
- **Phase 2 → Phases 3-4**: Foundational profile-boundary and fixture work blocks both user stories.
- **Phase 3 (US1)**: Recommended MVP and first delivery slice.
- **Phase 4 (US2)**: Starts after Phase 2; validate after US1 if working sequentially.
- **Phase 5**: Starts after the desired user stories are complete.

### User Story Dependencies

- **US1**: No dependency on other stories after Phase 2.
- **US2**: No hard dependency on US1 after Phase 2, but it reuses the same resolved-profile pipeline and is safest once US1 behavior is stable.

### Recommended Completion Order

1. Phase 1: Setup
2. Phase 2: Foundational
3. Phase 3: US1 (MVP)
4. Phase 4: US2
5. Phase 5: Polish

### Parallel Opportunities

- `T001` and `T002` can run in parallel during setup.
- `T005` and `T006` can run in parallel after `T003`-`T004`.
- In **US1**, `T007` and `T008` can run in parallel before `T009`-`T012`.
- In **US2**, `T013`, `T014`, and `T015` can run in parallel before `T016`-`T019`.
- `T021` and `T022` can run in parallel once implementation is complete; `T024` follows evidence capture.
- After Phase 2, different engineers can work on US1 and US2 concurrently if they coordinate shared-file edits in `src/sketch/SketchPicker.cpp`, `src/sketch/SketchToWire.cpp`, `src/sketch/ExtrudeOperation.cpp`, and `specs/007-fix-extrude-shape-fidelity/quickstart.md`.

---

## Parallel Example: User Story 1

```text
T007 Add failing single-profile curved-boundary extrusion assertions in tests/sketch/SketchIntersectionExtrudeTest.cpp
T008 Document single-profile shape-fidelity checks in specs/007-fix-extrude-shape-fidelity/quickstart.md
```

## Parallel Example: User Story 2

```text
T013 Add failing multi-profile alignment and hole-preservation assertions in tests/sketch/SketchIntersectionExtrudeTest.cpp
T014 Document multi-face alignment and internal-void checks in specs/007-fix-extrude-shape-fidelity/quickstart.md
T015 Add failing assertions that unselected sketch faces remain unchanged during multi-face extrusion in tests/sketch/SketchIntersectionExtrudeTest.cpp
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Finish Phase 1 and Phase 2.
2. Deliver Phase 3 to make single-face extrusions preserve the selected profile shape.
3. Run the US1 regression and manual curved-boundary checks.
4. Stop and review before starting multi-face/void preservation work.

### Incremental Delivery

1. Complete Setup + Foundational.
2. Deliver US1 and validate single-face shape fidelity.
3. Deliver US2 and validate multi-face alignment, internal-void preservation, and unselected-face non-impact.
4. Finish Polish and rerun the full build/regression/manual checklist.

### Parallel Team Strategy

1. One engineer owns shared foundation work in `src/sketch/` and the regression harness under `tests/sketch/`.
2. After Phase 2, split work across US1 single-profile fixes and US2 multi-profile/void handling.
3. Merge with shared-file coordination on `src/sketch/SketchPicker.cpp`, `src/sketch/SketchToWire.cpp`, `src/sketch/ExtrudeOperation.cpp`, `src/ui/MainWindow.cpp`, and `specs/007-fix-extrude-shape-fidelity/quickstart.md`.

---

## Notes

- All tasks use the required checklist format with task IDs, optional `[P]` markers, required `[US#]` labels for story phases, and exact file paths.
- Automated regression tasks are included because the spec and plan explicitly call for focused extrusion-fidelity coverage when practical.
- No `contracts/` tasks are included because this feature does not add an external interface.
- Do not create branches, commit, push, or tag as part of this task list.
