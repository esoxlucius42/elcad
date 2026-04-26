# Tasks: Fix Sketch Intersections

**Input**: Design documents from `/var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/`
**Prerequisites**: `/var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/plan.md`, `/var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/spec.md`, `/var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/research.md`, `/var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/data-model.md`, `/var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/quickstart.md` (`contracts/` not present)

**Tests**: Automated coverage is required by `/var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/spec.md`, so this plan adds a dedicated sketch-intersection regression target under `/var/home/esox/dev/cpp/elcad/tests/` plus the required manual validation and evidence capture in `/var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/quickstart.md`.

**Organization**: Tasks are grouped by user story priority so each story can be implemented and validated independently.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (`[US1]`, `[US2]`, `[US3]`)
- Every checklist item below uses the required `- [ ] T### ...` format and includes exact file paths

## Path Conventions

- **Sketch topology and profile logic**: `/var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.h`, `/var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp`, `/var/home/esox/dev/cpp/elcad/src/sketch/SketchProfiles.h`
- **Extrusion pipeline**: `/var/home/esox/dev/cpp/elcad/src/sketch/SketchToWire.h`, `/var/home/esox/dev/cpp/elcad/src/sketch/SketchToWire.cpp`, `/var/home/esox/dev/cpp/elcad/src/sketch/ExtrudeOperation.cpp`
- **Document/UI/render integration**: `/var/home/esox/dev/cpp/elcad/src/document/Document.h`, `/var/home/esox/dev/cpp/elcad/src/document/Document.cpp`, `/var/home/esox/dev/cpp/elcad/src/ui/MainWindow.cpp`, `/var/home/esox/dev/cpp/elcad/src/sketch/SketchRenderer.cpp`, `/var/home/esox/dev/cpp/elcad/src/viewport/Renderer.cpp`
- **Automated coverage**: `/var/home/esox/dev/cpp/elcad/CMakeLists.txt`, `/var/home/esox/dev/cpp/elcad/tests/CMakeLists.txt`, `/var/home/esox/dev/cpp/elcad/tests/sketch/`
- **Feature validation docs**: `/var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/quickstart.md`, `/var/home/esox/dev/cpp/elcad/README.md`

---

## Phase 1: Setup (Shared Validation Baseline)

**Purpose**: Align the feature evidence, affected files, and new regression-target scope before blocking implementation starts.

- [X] T001 [P] Align the automated-coverage requirement, manual evidence checklist, and artifact placeholders in /var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/quickstart.md
- [X] T002 [P] Confirm the overlap-selection, extrusion, and refresh touchpoints listed in /var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/plan.md and /var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/spec.md

**Checkpoint**: Validation scope and implementation touchpoints are fixed for the feature.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Create the shared regression harness and bounded-region data contracts required by all user stories.

**⚠️ CRITICAL**: Complete this phase before starting any user story work.

- [X] T003 Add the sketch-intersection regression target wiring in /var/home/esox/dev/cpp/elcad/CMakeLists.txt and /var/home/esox/dev/cpp/elcad/tests/CMakeLists.txt
- [X] T004 [P] Add shared bounded-region, selection-index, and profile metadata declarations in /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.h, /var/home/esox/dev/cpp/elcad/src/sketch/SketchProfiles.h, and /var/home/esox/dev/cpp/elcad/src/document/Document.h
- [X] T005 [P] Add reusable overlap-sketch fixture builders and assertion helpers in /var/home/esox/dev/cpp/elcad/tests/sketch/SketchIntersectionFixtures.h and /var/home/esox/dev/cpp/elcad/tests/sketch/SketchIntersectionFixtures.cpp
- [X] T006 Implement shared bounded-region topology, containment, and material-vs-void classification helpers in /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp

**Checkpoint**: Regression scaffolding exists and bounded-region contracts are ready for story work.

---

## Phase 3: User Story 1 - Select every bounded overlap region (Priority: P1) 🎯 MVP

**Goal**: Make every bounded material region created by overlapping closed sketch shapes independently selectable.

**Independent Test**: Build the regression target, create the rectangle-circle overlap from the spec, and verify that the rectangle-only region, the quarter-circle overlap region, and the outer three-quarter circle region each select independently.

### Verification for User Story 1

- [X] T007 [P] [US1] Add automated bounded-region partitioning and pick-hit coverage in /var/home/esox/dev/cpp/elcad/tests/sketch/SketchIntersectionSelectionTest.cpp
- [X] T008 [P] [US1] Expand the manual selection evidence steps and reviewer checklist in /var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/quickstart.md

### Implementation for User Story 1

- [X] T009 [US1] Generalize overlap-region derivation and selectable-material classification in /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp
- [X] T010 [US1] Map /var/home/esox/dev/cpp/elcad/src/document/Document.h and /var/home/esox/dev/cpp/elcad/src/document/Document.cpp sketch-area indices to resolved bounded regions via /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp
- [X] T011 [US1] Update completed-sketch highlighting and hit feedback for resolved bounded-region indices in /var/home/esox/dev/cpp/elcad/src/sketch/SketchRenderer.cpp and /var/home/esox/dev/cpp/elcad/src/viewport/Renderer.cpp

**Checkpoint**: All bounded overlap regions are selectable and highlight correctly without exposing phantom faces.

---

## Phase 4: User Story 2 - Extrude any selectable sketch face (Priority: P1)

**Goal**: Treat every selectable overlap region as a complete closed profile that extrudes successfully.

**Independent Test**: Reuse the rectangle-circle overlap, select each bounded region in turn, and confirm the automated regression plus manual workflow both show that every selected region extrudes without open-profile failures.

### Verification for User Story 2

- [X] T012 [P] [US2] Add automated resolved-profile extrusion coverage for overlap regions and true-hole boundaries in /var/home/esox/dev/cpp/elcad/tests/sketch/SketchIntersectionExtrudeTest.cpp
- [X] T013 [P] [US2] Expand the manual extrusion evidence checklist in /var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/quickstart.md

### Implementation for User Story 2

- [X] T014 [US2] Resolve each selected bounded region to explicit outer and hole boundaries in /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp and /var/home/esox/dev/cpp/elcad/src/sketch/SketchProfiles.h
- [X] T015 [US2] Build one OCCT face per resolved overlap profile in /var/home/esox/dev/cpp/elcad/src/sketch/SketchToWire.h and /var/home/esox/dev/cpp/elcad/src/sketch/SketchToWire.cpp
- [X] T016 [US2] Extrude resolved overlap profiles and surface closed-profile failures in /var/home/esox/dev/cpp/elcad/src/sketch/ExtrudeOperation.cpp and /var/home/esox/dev/cpp/elcad/src/ui/MainWindow.cpp

**Checkpoint**: Any selectable overlap-generated face extrudes as a valid closed profile.

---

## Phase 5: User Story 3 - Keep regions accurate after sketch changes (Priority: P2)

**Goal**: Refresh bounded-region selection and extrusion inputs after sketch edits, deletions, tangent-only contact, or coincident-boundary changes.

**Independent Test**: Create overlapping closed shapes, verify the expected regions, then move or delete one shape and confirm the next selection attempt reflects the new bounded areas while tangent-only and open cases stay non-selectable.

### Verification for User Story 3

- [X] T017 [P] [US3] Add automated refresh and regression coverage for sketch edits, tangent-only contact, and coincident-boundary cases in /var/home/esox/dev/cpp/elcad/tests/sketch/SketchIntersectionRefreshTest.cpp
- [X] T018 [P] [US3] Expand the manual refresh and negative-case evidence steps in /var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/quickstart.md

### Implementation for User Story 3

- [X] T019 [US3] Invalidate and rebuild bounded-region mappings after sketch edits or deletions in /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp, /var/home/esox/dev/cpp/elcad/src/document/Document.cpp, and /var/home/esox/dev/cpp/elcad/src/ui/MainWindow.cpp
- [X] T020 [US3] Keep hover, highlight, and pick behavior synchronized with refreshed region indices in /var/home/esox/dev/cpp/elcad/src/sketch/SketchRenderer.cpp, /var/home/esox/dev/cpp/elcad/src/viewport/Renderer.cpp, and /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp

**Checkpoint**: Region availability refreshes with sketch edits and negative edge cases stay non-misleading.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Finalize shared documentation, guardrails, and validation evidence across all stories.

- [X] T021 [P] Update the reviewer evidence checklist and regression command references in /var/home/esox/dev/cpp/elcad/README.md and /var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/quickstart.md
- [X] T022 Audit final OCCT guards, diagnostics, and cross-story cleanup in /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp, /var/home/esox/dev/cpp/elcad/src/sketch/SketchToWire.cpp, and /var/home/esox/dev/cpp/elcad/src/sketch/ExtrudeOperation.cpp
- [X] T023 Run the automated sketch-intersection regression target wired from /var/home/esox/dev/cpp/elcad/CMakeLists.txt and /var/home/esox/dev/cpp/elcad/tests/CMakeLists.txt and record the results in /var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/quickstart.md
- [ ] T024 Run the Release build plus manual validation scenarios A-D and capture screenshots, build output, and reviewer notes in /var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/quickstart.md

**Checkpoint**: Automated coverage, manual evidence, and cross-cutting cleanup are complete.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1: Setup** → can start immediately.
- **Phase 2: Foundational** → depends on Phase 1 and blocks all user stories.
- **Phase 3: US1** → depends on Phase 2.
- **Phase 4: US2** → depends on Phase 2 and on the bounded-region outputs from US1 (`T009`-`T011`) because extrusion consumes the resolved selectable regions.
- **Phase 5: US3** → depends on Phase 2 and should land after US1 so refresh behavior is built on the new bounded-region mapping; it should be validated after US2 so refreshed regions remain extrudable.
- **Phase 6: Polish** → depends on all desired user stories being complete.

### User Story Dependency Graph

`Setup -> Foundational -> US1 -> US2 -> US3 -> Polish`

### Within Each User Story

- Add automated coverage before closing implementation for that story.
- Keep the quickstart/manual evidence task in sync with the story’s final behavior before the phase is considered done.
- Finish shared data-model or picker changes before dependent UI/render/extrusion wiring.
- Validate each story independently before moving to the next priority.

### Parallel Opportunities

- **Setup**: `T001` and `T002` can run together.
- **Foundational**: `T004` and `T005` can run together after `T003`; `T006` depends on `T004`.
- **US1**: `T007` and `T008` can run in parallel while `T009` is in progress; `T010` depends on `T009`; `T011` depends on `T010`.
- **US2**: `T012` and `T013` can run in parallel; `T015` depends on `T014`; `T016` depends on `T014` and `T015`.
- **US3**: `T017` and `T018` can run in parallel; `T020` depends on `T019`.
- **Polish**: `T021` can run in parallel with `T022`; execution evidence tasks `T023` and `T024` come last.

---

## Parallel Example: User Story 1

```bash
# Launch selection verification work together:
Task: "T007 [US1] Add automated bounded-region partitioning and pick-hit coverage in /var/home/esox/dev/cpp/elcad/tests/sketch/SketchIntersectionSelectionTest.cpp"
Task: "T008 [US1] Expand the manual selection evidence steps in /var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/quickstart.md"

# Then complete the implementation chain:
Task: "T009 [US1] Generalize overlap-region derivation and selectable-material classification in /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp"
Task: "T010 [US1] Map sketch-area indices to resolved bounded regions in /var/home/esox/dev/cpp/elcad/src/document/Document.cpp"
Task: "T011 [US1] Update completed-sketch highlighting in /var/home/esox/dev/cpp/elcad/src/sketch/SketchRenderer.cpp and /var/home/esox/dev/cpp/elcad/src/viewport/Renderer.cpp"
```

## Parallel Example: User Story 2

```bash
# Launch extrusion verification work together:
Task: "T012 [US2] Add automated resolved-profile extrusion coverage in /var/home/esox/dev/cpp/elcad/tests/sketch/SketchIntersectionExtrudeTest.cpp"
Task: "T013 [US2] Expand the manual extrusion evidence checklist in /var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/quickstart.md"

# Then follow the resolved-profile chain:
Task: "T014 [US2] Resolve selected bounded regions in /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp and /var/home/esox/dev/cpp/elcad/src/sketch/SketchProfiles.h"
Task: "T015 [US2] Build one OCCT face per resolved overlap profile in /var/home/esox/dev/cpp/elcad/src/sketch/SketchToWire.cpp"
Task: "T016 [US2] Extrude resolved overlap profiles in /var/home/esox/dev/cpp/elcad/src/sketch/ExtrudeOperation.cpp and /var/home/esox/dev/cpp/elcad/src/ui/MainWindow.cpp"
```

## Parallel Example: User Story 3

```bash
# Launch refresh verification work together:
Task: "T017 [US3] Add automated refresh and regression coverage in /var/home/esox/dev/cpp/elcad/tests/sketch/SketchIntersectionRefreshTest.cpp"
Task: "T018 [US3] Expand the manual refresh and negative-case evidence steps in /var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/quickstart.md"

# Then finish refresh integration:
Task: "T019 [US3] Invalidate and rebuild bounded-region mappings in /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp, /var/home/esox/dev/cpp/elcad/src/document/Document.cpp, and /var/home/esox/dev/cpp/elcad/src/ui/MainWindow.cpp"
Task: "T020 [US3] Keep hover, highlight, and pick behavior synchronized in /var/home/esox/dev/cpp/elcad/src/sketch/SketchRenderer.cpp and /var/home/esox/dev/cpp/elcad/src/viewport/Renderer.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1 and Phase 2.
2. Complete Phase 3 (`T007`-`T011`).
3. Build and validate the rectangle-circle overlap selection workflow independently.
4. Stop for review once every bounded overlap region is selectable.

### Incremental Delivery

1. Finish Setup + Foundational to establish the regression harness and bounded-region contracts.
2. Deliver US1 so every bounded overlap region can be selected independently.
3. Deliver US2 so every selectable region extrudes as a closed profile.
4. Deliver US3 so edits, deletions, and negative edge cases refresh correctly.
5. Finish Polish with automated and manual evidence capture.

### Parallel Team Strategy

1. One developer owns `/var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp` and `/var/home/esox/dev/cpp/elcad/src/sketch/SketchProfiles.h` topology/profile work.
2. A second developer owns `/var/home/esox/dev/cpp/elcad/tests/sketch/` regression coverage and keeps `/var/home/esox/dev/cpp/elcad/tests/CMakeLists.txt` current.
3. A third developer can update `/var/home/esox/dev/cpp/elcad/src/sketch/SketchRenderer.cpp`, `/var/home/esox/dev/cpp/elcad/src/viewport/Renderer.cpp`, `/var/home/esox/dev/cpp/elcad/src/ui/MainWindow.cpp`, and `/var/home/esox/dev/cpp/elcad/specs/005-fix-sketch-intersections/quickstart.md` once the shared contracts land.

---

## Notes

- `contracts/` is intentionally absent for this feature because the change is internal to desktop sketch selection/extrusion.
- Automated coverage is required even though the repository currently lacks an existing test harness; this plan introduces the minimum shared test scaffolding first.
- Keep all OCCT-dependent work behind `#ifdef ELCAD_HAVE_OCCT`.
- `Document` remains the mutation owner; UI tasks in `/var/home/esox/dev/cpp/elcad/src/ui/MainWindow.cpp` should orchestrate, not own geometry state.
- Do not create branches, switch branches, commit, or push as part of this feature work.
