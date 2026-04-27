# Tasks: Fix Multi-Shape Extrude

**Input**: Design documents from `specs/002-fix-multi-shape-extrude/`
**Prerequisites**: plan.md ✅, spec.md ✅, research.md ✅, data-model.md ✅, quickstart.md ✅ (`contracts/` not applicable — internal desktop fix)

**Tests**: No automated test target currently exists in the repository. Tasks include manual validation steps per quickstart.md scenarios and evidence-capture instructions. The automated-coverage gap is documented (per research Decision 4).

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (`[US1]`, `[US2]`)
- All task descriptions include exact file paths

## Path Conventions

- **Sketch helpers**: `src/sketch/SketchPicker.h/.cpp`, `src/sketch/SketchToWire.h/.cpp`, `src/sketch/ExtrudeOperation.h/.cpp`
- **Document/body state**: `src/document/Document.h/.cpp`, `src/document/Body.h/.cpp`
- **UI orchestration**: `src/ui/MainWindow.cpp`
- **Build**: `CMakeLists.txt`
- **Feature validation docs**: `specs/002-fix-multi-shape-extrude/quickstart.md`

---

## Phase 1: Setup (Audit & Baseline)

**Purpose**: Confirm build baseline and map the exact call sites that must change before any code is touched.

- [X] T001 Verify clean CMake configure and build from repository root to establish baseline (`CMakeLists.txt`, `build/`)
- [X] T002 [P] Audit `src/sketch/SketchToWire.cpp` and `src/sketch/SketchPicker.cpp` to locate the single-wire build call that fails on disconnected loops and map existing `findClosedLoops` API surface
- [X] T003 [P] Audit `src/ui/MainWindow.cpp` extrude preview and commit entry points and `src/sketch/ExtrudeOperation.cpp` to map the current single-result extrude flow end-to-end

**Checkpoint**: Baseline build confirmed; call sites identified — ready to add foundational data structures

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core data structures and declarations that MUST exist before either user story can be implemented. No user-story work may begin until this phase is complete.

**⚠️ CRITICAL**: Blocks all user story phases

- [X] T004 Add `SketchFaceSelection` and `SelectedSketchProfile` structs to `src/sketch/SketchPicker.h` (fields: `sketchId`, `loopIndices`, `loopIndex`, `sourceEntityIds`, `plane`, `isClosed` per data model)
- [X] T005 [P] Add `ExtrudeBatchResult` struct to `src/sketch/ExtrudeOperation.h` (fields: `success`, `errorMsg`, `solids: std::vector<TopoDS_Shape>`, `finalTargetShape: std::optional<TopoDS_Shape>` per data model)
- [X] T006 [P] Declare `buildFaceForProfile()` API in `src/sketch/SketchToWire.h` — signature takes a single `SelectedSketchProfile` and returns one `TopoDS_Face` under `#ifdef ELCAD_HAVE_OCCT` guard
- [X] T007 Add selection retrieval helper declaration to `src/document/Document.h` — method groups `Document::SelectedItem::SketchArea` entries by `sketchId` and returns a `SketchFaceSelection` for the active sketch
- [X] T008 [P] Implement the Document selection retrieval helper body in `src/document/Document.cpp` — iterate selected items, collect `loopIndices`, deduplicate, return `SketchFaceSelection`
- [X] T009 [P] Update `CMakeLists.txt` if any new `.cpp` translation units are introduced in Phase 3/4 (no new files expected but confirm scope matches plan)

**Checkpoint**: Data structures declared, Document helper implemented — user story phases may now begin

---

## Phase 3: User Story 1 — Extrude Multiple Disconnected Faces (Priority: P1) 🎯 MVP

**Goal**: A user selects two or more non-overlapping closed faces in one sketch, runs one extrude command, and gets one separate 3D body per selected face — without the current `Wire builder failed: edges may not be connected` error.

**Independent Test**: Create a sketch with three non-overlapping closed shapes, select two or more faces, run Extrude → New Body at any distance, confirm the command completes and each selected face produces a separate body. (Quickstart Scenario A)

### Verification for User Story 1

- [X] T010 [P] [US1] Update `specs/002-fix-multi-shape-extrude/quickstart.md` — confirm Scenario A steps match the final implementation and add a concrete evidence-capture checklist (build output, screenshot of multi-body result, reviewer note confirming error is gone)

### Implementation for User Story 1

- [X] T011 [US1] Implement `resolveSelectedProfiles()` in `src/sketch/SketchPicker.cpp` — call `findClosedLoops`, filter to loop indices in `SketchFaceSelection::loopIndices`, validate each loop is closed and non-degenerate, return `std::vector<SelectedSketchProfile>` (depends on T004, T007, T008)
- [X] T012 [US1] Implement `buildFaceForProfile()` in `src/sketch/SketchToWire.cpp` under `#ifdef ELCAD_HAVE_OCCT` — build one `BRepBuilderAPI_MakeWire` / `BRepBuilderAPI_MakeFace` from a single resolved profile loop instead of the full sketch entity set (depends on T006, T011)
- [X] T013 [US1] Implement `extrudeProfiles()` in `src/sketch/ExtrudeOperation.cpp` — call `buildFaceForProfile` for each `SelectedSketchProfile`, sweep each face with `BRepPrimAPI_MakePrism`, collect results into `ExtrudeBatchResult::solids`; if any step fails set `success=false` and `errorMsg` before mutating `Document` (depends on T005, T012)
- [X] T014 [US1] Update the extrude **commit** path in `src/ui/MainWindow.cpp` — replace single-solid handling with a loop over `ExtrudeBatchResult::solids` and call `Document::addBody` once per solid in New Body mode; propagate `errorMsg` to the existing user-facing error display on failure (depends on T013)
- [X] T015 [US1] Update the extrude **preview** path in `src/ui/MainWindow.cpp` — call `extrudeProfiles()` in preview mode and compose per-profile preview shapes so the viewport shows one ghost solid per selected face (depends on T013)
- [X] T016 [P] [US1] Add `spdlog` diagnostics in `src/sketch/SketchPicker.cpp` and `src/sketch/ExtrudeOperation.cpp` — log resolved profile count, per-profile loop index, and batch success/failure at debug level

**Checkpoint**: At this point User Story 1 is fully functional — multi-face extrude creates one body per selected face without the wire-builder error. Validate independently with Quickstart Scenario A before proceeding.

---

## Phase 4: User Story 2 — Extrude Only the Intended Faces (Priority: P2)

**Goal**: The extrude command affects precisely and only the faces the user selected. Unselected sketch faces remain in the sketch, unchanged, and produce no new bodies.

**Independent Test**: Create a sketch with at least three disconnected closed shapes, select only a subset (e.g., two of three), run one extrude, confirm exactly two new bodies are created and the third sketch face is untouched. (Quickstart Scenario B)

### Verification for User Story 2

- [X] T017 [P] [US2] Update `specs/002-fix-multi-shape-extrude/quickstart.md` — confirm Scenario B steps, add evidence-capture checklist (body count equals selected-face count, unselected face still visible in sketch, no extra solids)

### Implementation for User Story 2

- [X] T018 [US2] Harden `resolveSelectedProfiles()` in `src/sketch/SketchPicker.cpp` — assert that only explicitly selected loop indices are included, reject duplicates silently (deduplicate) or with a logged warning, and verify no fallback to whole-sketch loop enumeration occurs (depends on T011)
- [X] T019 [US2] Guard `extrudeProfiles()` in `src/sketch/ExtrudeOperation.cpp` against whole-sketch fallback — if `SketchFaceSelection::loopIndices` is empty the method must return an error result, never silently extrude all loops (depends on T013)
- [X] T020 [US2] Verify and update `Document::addBody` call sites in `src/document/Document.cpp` — confirm that only the bodies produced from `ExtrudeBatchResult::solids` are committed, and that the sketch entity set for unselected areas is not passed to any wire/face builder (depends on T008, T014)
- [X] T021 [US2] Update the Add/Cut mode path in `src/sketch/ExtrudeOperation.cpp` — apply each selected-profile solid from `ExtrudeBatchResult::solids` to a working copy of the target body using boolean operations, commit only `finalTargetShape` on full success, leaving unselected loops untouched (depends on T013)
- [X] T022 [US2] Verify per-body placement in `src/sketch/ExtrudeOperation.cpp` — confirm each body in `ExtrudeBatchResult::solids` inherits the correct `SketchPlane` transform from its `SelectedSketchProfile::plane` field so resulting bodies align with their source sketch faces (FR-008) (depends on T013)

**Checkpoint**: User Stories 1 AND 2 are both fully functional. Validate Scenario B independently before proceeding to Polish.

---

## Phase 5: Polish & Cross-Cutting Concerns

**Purpose**: Edge-case hardening, atomic-failure guarantee, final validation, and documentation tidy-up.

- [X] T023 [P] Update `specs/002-fix-multi-shape-extrude/quickstart.md` — add Scenario C (invalid selection fails atomically: no partial bodies, clear error shown) evidence steps and confirm the evidence-capture checklist covers all three scenarios
- [X] T024 Implement atomic-failure guard in `src/sketch/ExtrudeOperation.cpp` — if any profile in `extrudeProfiles()` fails validation (non-closed loop, degenerate face, zero-distance extrude) set `success=false` with a descriptive `errorMsg` and return before any `Document` mutation; cover both New Body and Add/Cut modes (FR-006, FR-007) (depends on T013, T021)
- [X] T025 [P] Code cleanup across `src/sketch/SketchPicker.cpp`, `src/sketch/SketchToWire.cpp`, and `src/sketch/ExtrudeOperation.cpp` — remove any temporary scaffolding, finalize inline documentation, ensure `#ifdef ELCAD_HAVE_OCCT` guards are complete and consistent
- [ ] T026 Run full quickstart.md validation scenarios A, B, and C in elcad (`build/` Release binary) and capture review evidence: build log, screenshot of multi-body result, reviewer notes confirming no connectivity error and correct selection scoping

**Checkpoint**: All stories complete, edge cases hardened, evidence captured — feature ready for review.

---

## Dependencies & Execution Order

### Phase Dependencies

```
Phase 1 (Setup)         → no dependencies; start immediately
Phase 2 (Foundational)  → depends on Phase 1 completion; BLOCKS all user stories
Phase 3 (US1 / P1)      → depends on Phase 2 completion
Phase 4 (US2 / P2)      → depends on Phase 2 completion; may run in parallel with Phase 3
                           but US2 implementation tasks depend on US1 task outputs (T011, T013)
Phase 5 (Polish)        → depends on Phase 3 and Phase 4 completion
```

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Phase 2 — no dependency on US2
- **User Story 2 (P2)**: Can start after Phase 2 — implementation tasks depend on T011 and T013 from US1 (resolveSelectedProfiles and extrudeProfiles must exist first); verification (T017) and scoping guards (T018, T019) are independent

### Within Each User Story

- Validation/quickstart tasks (`[P]`) can begin in parallel with implementation
- Data-structure tasks must complete before dependent implementation tasks
- Core logic (T012 → T013) must precede UI wiring (T014, T015)
- Placement/scoping guards (T022, T018) depend on core logic completing

### Task-Level Dependencies (key chains)

```
T004 → T011 → T012 → T013 → T014
T005 → T013                 T015
T006 → T012
T007 → T008 → T011
T011 → T018
T013 → T019, T021, T022, T024
T014 → T020
```

---

## Parallel Execution Examples

### Phase 1 Parallel Work

```
# Run in parallel (different files, no dependencies on each other):
Task T002: Audit src/sketch/SketchToWire.cpp + src/sketch/SketchPicker.cpp
Task T003: Audit src/ui/MainWindow.cpp + src/sketch/ExtrudeOperation.cpp
```

### Phase 2 Parallel Work

```
# After T004 is complete, run in parallel:
Task T005: Add ExtrudeBatchResult to src/sketch/ExtrudeOperation.h
Task T006: Declare buildFaceForProfile() in src/sketch/SketchToWire.h
Task T007+T008: Document selection helper (sequential pair)
Task T009: Verify CMakeLists.txt
```

### Phase 3 Parallel Work

```
# Run in parallel once Phase 2 is complete:
Task T010: Update quickstart.md Scenario A evidence checklist
Task T016: Add spdlog diagnostics in SketchPicker + ExtrudeOperation

# Sequential chain (core implementation):
T011 → T012 → T013 → T014 (commit path)
                  ↘ T015 (preview path, parallel with T014)
```

### Phase 4 Parallel Work

```
# Run in parallel once T013 exists:
Task T017: Update quickstart.md Scenario B evidence checklist
Task T018: Harden resolveSelectedProfiles() selection scope
Task T019: Guard extrudeProfiles() against empty-selection fallback

# After T013 + T021 complete:
Task T022: Verify per-body placement alignment
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001–T003) — ~30 min
2. Complete Phase 2: Foundational (T004–T009) — **blocks all stories**
3. Complete Phase 3: User Story 1 (T010–T016)
4. **STOP and VALIDATE**: Run Quickstart Scenario A, confirm multi-body result, no wire error
5. Deliver MVP — the blocking bug (FR-001/FR-002) is fixed

### Incremental Delivery

1. Setup + Foundational → baseline data structures ready
2. User Story 1 (T010–T016) → MVP: multi-face extrude works → validate independently
3. User Story 2 (T017–T022) → selection precision: only intended faces extruded → validate independently
4. Polish (T023–T026) → atomic failure, cleanup, full evidence capture → ready for review
5. Each story adds value without breaking previous stories

### Parallel Developer Strategy

With two developers available after Phase 2 completes:

```
Developer A: Phase 3 — T011 → T012 → T013 → T014 → T015 → T016
Developer B: Phase 4 — T017 (quickstart) → T018 → T019 (wait for T013 from Dev A) → T020 → T021 → T022
```

---

## Notes

- `[P]` tasks operate on different files or are documentation-only — safe to run in parallel
- `[US1]` / `[US2]` labels map directly to spec.md User Story 1 (P1) and User Story 2 (P2) for traceability
- No automated test target currently exists; manual quickstart evidence is the primary verification mechanism (documented gap, per research.md Decision 4)
- All OCCT code stays under `#ifdef ELCAD_HAVE_OCCT` guards as required by the plan
- `Document` remains the mutation owner — no UI code may directly mutate Body state
- Do not commit, tag, or push as part of any task — leave those git decisions to the developer
- Stop at each Phase checkpoint to validate independently before proceeding
