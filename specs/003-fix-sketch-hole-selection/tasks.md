# Tasks: Fix Sketch Hole Selection

**Input**: Design documents from `/var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/`
**Prerequisites**: `/var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/plan.md`, `/var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/spec.md`, `/var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/research.md`, `/var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/data-model.md`, `/var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/quickstart.md`

**Tests**: No dedicated automated regression harness exists for this workflow, so use CMake build verification plus the required manual validation and evidence capture in `/var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/quickstart.md`.

**Organization**: Tasks are grouped by user story so each story can be implemented and validated independently.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (`US1`, `US2`)
- Every task below includes an exact file path

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Align validation evidence and implementation notes before code changes.

- [x] T001 [P] Align manual evidence steps and the automated-regression-gap note in /var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/quickstart.md
- [x] T002 [P] Confirm affected sketch/OCCT touchpoints and implementation notes in /var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/plan.md

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Shared hole-aware selection data and topology logic required by all user stories.

**⚠️ CRITICAL**: Complete this phase before starting user story work.

- [x] T003 [P] Extend hole-aware profile and hole-boundary structs in /var/home/esox/dev/cpp/elcad/src/sketch/SketchProfiles.h
- [x] T004 [P] Add derived loop-topology declarations and containment helper signatures in /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.h
- [x] T005 Implement derived loop containment and nesting utilities in /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp

**Checkpoint**: Hole-aware loop metadata is available for selection and extrusion work.

---

## Phase 3: User Story 1 - Select a face with a hole correctly (Priority: P1) 🎯 MVP

**Goal**: Let annular and multi-hole sketch regions resolve as one selected face whose extrusion preserves every enclosed void.

**Independent Test**: Build elcad, create a rectangle with one enclosed circle, click the annular region, extrude it, and confirm preview plus committed geometry preserve the circular opening; repeat with two inner circles and confirm every hole stays open.

### Verification for User Story 1

- [x] T006 [P] [US1] Expand annular-region and multi-hole reviewer evidence steps in /var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/quickstart.md

### Implementation for User Story 1

- [x] T007 [P] [US1] Resolve selected outer loops into hole-aware sketch profiles in /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp
- [x] T008 [P] [US1] Build OCCT faces from one outer wire plus hole wires in /var/home/esox/dev/cpp/elcad/src/sketch/SketchToWire.h and /var/home/esox/dev/cpp/elcad/src/sketch/SketchToWire.cpp
- [x] T009 [US1] Preserve hole boundaries through selected-profile extrusion in /var/home/esox/dev/cpp/elcad/src/sketch/ExtrudeOperation.cpp
- [x] T010 [US1] Surface hole-aware profile-resolution failures during sketch extrude startup in /var/home/esox/dev/cpp/elcad/src/ui/MainWindow.cpp

**Checkpoint**: Annular and multi-hole profiles extrude as bounded faces with preserved voids.

---

## Phase 4: User Story 2 - Avoid selecting the surrounding face from inside a void (Priority: P2)

**Goal**: Treat clicks inside a hole as empty space so the surrounding face is not selected from that click.

**Independent Test**: Build elcad, create a rectangle with an enclosed circle, click inside the circle, and confirm no surrounding-face selection or misleading extrude source appears; compare with a second click in the annular region to confirm only that click selects a valid face.

### Verification for User Story 2

- [x] T011 [P] [US2] Add hole-click and invalid-geometry reviewer evidence steps in /var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/quickstart.md

### Implementation for User Story 2

- [x] T012 [US2] Reject sketch-area hits that fall inside derived hole loops while preserving edge/point precedence in /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp
- [x] T013 [P] [US2] Clarify invalid bounded-face warning handling for hole-selection failures in /var/home/esox/dev/cpp/elcad/src/ui/MainWindow.cpp

**Checkpoint**: Hole clicks no longer select the surrounding bounded face, and invalid topology stays non-misleading.

---

## Phase 5: Polish & Cross-Cutting Concerns

**Purpose**: Final validation, evidence capture, and cleanup spanning both stories.

- [x] T014 Consolidate the final reviewer evidence checklist in /var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/quickstart.md
- [x] T015 Record successful `cmake --build build --parallel $(nproc)` verification against /var/home/esox/dev/cpp/elcad/CMakeLists.txt in /var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/quickstart.md
- [ ] T016 Run rectangle-with-circle, multi-hole, hole-click, and invalid-geometry validation and capture reviewer notes in /var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/quickstart.md

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1: Setup** → can start immediately.
- **Phase 2: Foundational** → depends on Phase 1 and blocks both user stories.
- **Phase 3: US1** → depends on Phase 2.
- **Phase 4: US2** → depends on Phase 2; recommended to land after US1 for MVP-first delivery, though it can proceed in parallel if file-level coordination on `/var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp` is managed.
- **Phase 5: Polish** → depends on all desired user stories being complete.

### User Story Dependency Graph

`Setup -> Foundational -> US1 -> US2 -> Polish`

### Within Each User Story

- Complete verification/documentation updates before closing implementation tasks.
- Finish shared picker/profile changes before dependent extrusion or UI handling.
- Validate each story independently before moving to the next priority.

### Parallel Opportunities

- **Setup**: `T001` and `T002` can run together.
- **Foundational**: `T003` and `T004` can run together before `T005`.
- **US1**: `T006`, `T007`, and `T008` can run in parallel after Phase 2; `T009` depends on `T007` and `T008`.
- **US2**: `T011` and `T013` can run in parallel while `T012` implements the picker behavior.

---

## Parallel Example: User Story 1

```bash
# Launch documentation and hole-aware implementation work together:
Task: "T006 [US1] Expand annular-region and multi-hole reviewer evidence steps in /var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/quickstart.md"
Task: "T007 [US1] Resolve selected outer loops into hole-aware sketch profiles in /var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp"
Task: "T008 [US1] Build OCCT faces from one outer wire plus hole wires in /var/home/esox/dev/cpp/elcad/src/sketch/SketchToWire.cpp"
```

## Parallel Example: User Story 2

```bash
# Launch reviewer-evidence and UI-warning work together:
Task: "T011 [US2] Add hole-click and invalid-geometry reviewer evidence steps in /var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/quickstart.md"
Task: "T013 [US2] Clarify invalid bounded-face warning handling for hole-selection failures in /var/home/esox/dev/cpp/elcad/src/ui/MainWindow.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1 and Phase 2.
2. Complete Phase 3 (US1).
3. Build and manually validate the rectangle-with-circle and multi-hole workflows.
4. Stop for review before starting US2.

### Incremental Delivery

1. Ship the shared containment/profile foundation.
2. Deliver US1 so annular selection and extrusion preserve holes.
3. Deliver US2 so clicks inside holes stop selecting the surrounding face.
4. Finish with build evidence and manual validation updates.

### Parallel Team Strategy

1. One developer handles `/var/home/esox/dev/cpp/elcad/src/sketch/SketchPicker.cpp` containment work.
2. A second developer handles `/var/home/esox/dev/cpp/elcad/src/sketch/SketchToWire.cpp` and `/var/home/esox/dev/cpp/elcad/src/sketch/ExtrudeOperation.cpp` after the shared profile contract is defined.
3. A third developer can keep `/var/home/esox/dev/cpp/elcad/specs/003-fix-sketch-hole-selection/quickstart.md` evidence steps current throughout implementation.

---

## Notes

- `contracts/` remains intentionally omitted for this feature.
- No autonomous git actions are included; remain on `main`.
- All tasks use the required checklist format with IDs, optional `[P]`, required story labels on story tasks, and exact file paths.
