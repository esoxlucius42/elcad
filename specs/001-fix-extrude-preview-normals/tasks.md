# Tasks: Fix Extrude Preview Normals

**Input**: Design documents from `/specs/001-fix-extrude-preview-normals/`
**Prerequisites**: `plan.md`, `spec.md`, `research.md`, `data-model.md`, `quickstart.md`

**Tests**: Manual viewport validation is the primary verification path for this feature. Do not add automated viewport regression work unless a practical harness emerges during implementation.

**Organization**: Tasks are grouped by user story so each story can be implemented and validated independently.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no blocking dependency)
- **[Story]**: User story label for story-specific phases (`[US1]`, `[US2]`)
- Every task includes exact file paths

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Confirm scope, validation, and implementation boundaries before changing renderer code.

- [X] T001 Review preview-fix scope and affected render paths in `src/ui/MainWindow.cpp`, `src/viewport/Renderer.h`, and `src/viewport/Renderer.cpp`
- [X] T002 [P] Align manual validation and evidence requirements in `specs/001-fix-extrude-preview-normals/quickstart.md`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Establish mesh and renderer prerequisites that all preview-fix stories depend on.

**⚠️ CRITICAL**: Complete this phase before starting user story implementation.

- [X] T003 [P] Preserve preview mesh triangle winding and normals needed for face culling in `src/viewport/MeshBuffer.cpp`
- [X] T004 [P] Add renderer-owned preview render-state plumbing for the Phong ghost path in `src/viewport/Renderer.h` and `src/viewport/Renderer.cpp`

**Checkpoint**: Preview mesh orientation and renderer state scaffolding are ready for story work.

---

## Phase 3: User Story 1 - Read extrusion direction correctly (Priority: P1) 🎯 MVP

**Goal**: Make the initial extrude preview and depth updates show the camera-facing top and side surfaces instead of hidden back faces.

**Independent Test**: Start the extrude tool from a valid sketch face in top or isometric view, change preview depth, and confirm the visible top and side surfaces match the pending volume.

### Verification for User Story 1

- [X] T005 [P] [US1] Refine initial-preview and depth-change validation steps in `specs/001-fix-extrude-preview-normals/quickstart.md`

### Implementation for User Story 1

- [X] T006 [US1] Implement camera-facing translucent preview drawing with correct culling and depth-state restoration in `src/viewport/Renderer.cpp`
- [X] T007 [US1] Declare preview-state helpers and preview pass invariants in `src/viewport/Renderer.h`
- [X] T008 [US1] Keep sketch-face extrude preview recompute and viewport update flow aligned with the new preview pass in `src/ui/MainWindow.cpp`

**Checkpoint**: User Story 1 is functional and manually testable on its own.

---

## Phase 4: User Story 2 - Inspect the preview from different angles (Priority: P2)

**Goal**: Keep the preview visually correct while the user changes views, inspects the opposite side, and exits or restarts the tool.

**Independent Test**: Start an extrusion preview, orbit/pan/zoom through top, isometric, side, and opposite-side views, then cancel and restart the tool while confirming camera-facing surfaces stay visible and hidden faces stay occluded.

### Verification for User Story 2

- [X] T009 [P] [US2] Expand multi-view, opposite-direction, and cancel-restart checks in `specs/001-fix-extrude-preview-normals/quickstart.md`

### Implementation for User Story 2

- [X] T010 [US2] Update preview rendering to keep camera-facing surfaces correct during orbit, pan, zoom, and opposite-side views in `src/viewport/Renderer.cpp`
- [X] T011 [P] [US2] Ensure preview shape queueing and cleanup remain stable across cancel and restart cycles in `src/ui/MainWindow.cpp`
- [X] T012 [US2] Harden preview clear and reset behavior so preview render state does not leak after tool exit in `src/viewport/Renderer.cpp`

**Checkpoint**: User Stories 1 and 2 both work with independent manual validation.

---

## Phase 5: Polish & Cross-Cutting Concerns

**Purpose**: Final validation, readability tuning, and reviewer-facing evidence updates across the feature.

- [X] T013 [P] Document the manual-evidence-first verification note and automated-harness limitation in `specs/001-fix-extrude-preview-normals/spec.md` and `specs/001-fix-extrude-preview-normals/quickstart.md`
- [X] T014 [P] Recheck preview readability against the existing Phong ghost shading constants in `src/viewport/Renderer.cpp` and `src/shaders/phong.frag`
- [X] T015 [P] Confirm release-build verification steps stay current in `specs/001-fix-extrude-preview-normals/quickstart.md` and `CMakeLists.txt`
- [X] T016 Run the full manual extrude-preview validation pass and capture reviewer evidence requirements in `specs/001-fix-extrude-preview-normals/quickstart.md`

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies; start immediately.
- **Phase 2 (Foundational)**: Depends on Phase 1 and blocks all user story work.
- **Phase 3 (US1)**: Depends on Phase 2.
- **Phase 4 (US2)**: Depends on Phase 2 and is best sequenced after US1 because both stories change `src/viewport/Renderer.cpp`.
- **Phase 5 (Polish)**: Depends on the user stories selected for delivery.

### User Story Dependency Graph

```text
Setup -> Foundational -> US1 -> US2 -> Polish
```

### Within Each User Story

- Finish manual validation updates before closing implementation.
- Complete `src/viewport/MeshBuffer.cpp` and `src/viewport/Renderer.h` prerequisites before changing the main preview pass in `src/viewport/Renderer.cpp`.
- Land renderer behavior before `src/ui/MainWindow.cpp` integration cleanup.

### Parallel Opportunities

- `T002`, `T003`, and `T004` can run in parallel after scope review.
- In US1, `T005` can run in parallel with early renderer/header work while `T006` and `T007` are being prepared.
- In US2, `T009` and `T011` can run in parallel because they touch different files.
- In Polish, `T013`, `T014`, and `T015` can run in parallel before the final manual pass in `T016`.

---

## Parallel Example: User Story 1

```bash
# Run documentation and renderer-prep work together
Task: "Refine initial-preview and depth-change validation steps in specs/001-fix-extrude-preview-normals/quickstart.md"
Task: "Declare preview-state helpers and preview pass invariants in src/viewport/Renderer.h"
```

## Parallel Example: User Story 2

```bash
# Run manual-check updates and lifecycle wiring together
Task: "Expand multi-view, opposite-direction, and cancel-restart checks in specs/001-fix-extrude-preview-normals/quickstart.md"
Task: "Ensure preview shape queueing and cleanup remain stable across cancel and restart cycles in src/ui/MainWindow.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup.
2. Complete Phase 2: Foundational.
3. Complete Phase 3: User Story 1.
4. Run the manual viewport validation from `specs/001-fix-extrude-preview-normals/quickstart.md`.
5. Stop and review screenshots/notes before expanding scope.

### Incremental Delivery

1. Setup + Foundational establish safe preview-rendering scaffolding.
2. Deliver US1 to fix the core inside-out extrude preview defect.
3. Deliver US2 to harden behavior across camera movement, opposite-side inspection, and cancel/restart cycles.
4. Finish with Polish to capture evidence and final readability/build validation.

### Suggested MVP Scope

- **MVP**: Phase 1 + Phase 2 + Phase 3 (User Story 1)
- **Follow-up**: Phase 4 (User Story 2) and Phase 5 (Polish)

---

## Notes

- Manual viewport validation is the required verification path for this feature.
- Automated viewport regression work is intentionally omitted because the current docs only allow it if a practical harness emerges.
- Do not create autonomous commits, tags, or pushes while implementing these tasks.
- Every task above follows the required checklist format with checkbox, task ID, optional `[P]`, optional story label, and exact file paths.
