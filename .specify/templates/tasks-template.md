---

description: "Task list template for feature implementation"
---

# Tasks: [FEATURE NAME]

**Input**: Design documents from `/specs/[###-feature-name]/`
**Prerequisites**: plan.md (required), spec.md (required for user stories), research.md, data-model.md, contracts/

**Tests**: The examples below include verification tasks. Add automated tests
when practical and explicitly requested; otherwise include concrete manual
validation and evidence-capture tasks.

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- **elcad source**: `src/app/`, `src/document/`, `src/io/`, `src/sketch/`,
  `src/tools/`, `src/ui/`, `src/viewport/`
- **Utilities**: `tools/`
- **Automated tests**: `tests/` if introduced by the feature
- **Feature docs and validation**: `specs/<feature>/`, `README.md`, `docs/`

<!-- 
  ============================================================================
  IMPORTANT: The tasks below are SAMPLE TASKS for illustration purposes only.
  
  The speckit.tasks agent MUST replace these with actual tasks based on:
  - User stories from spec.md (with their priorities P1, P2, P3...)
  - Feature requirements from plan.md
  - Entities from data-model.md
  - Endpoints from contracts/
  
  Tasks MUST be organized by user story so each story can be:
  - Implemented independently
  - Tested independently
  - Delivered as an MVP increment
  
  DO NOT keep these sample tasks in the generated tasks.md file.
  ============================================================================
-->

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and basic structure

- [ ] T001 Confirm affected elcad directories and feature artifacts per implementation plan
- [ ] T002 Update CMake, resources, or dependency wiring needed for the feature
- [ ] T003 [P] Capture verification approach (automated and/or manual) in spec/plan/quickstart artifacts

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

Examples of foundational tasks (adjust based on your project):

- [ ] T004 Extend shared document/model structures needed across stories
- [ ] T005 [P] Add or adapt OCCT/geometry utilities behind required feature guards
- [ ] T006 [P] Wire shared UI, viewport, or tool-state integration points
- [ ] T007 Establish logging, error-result, and undo integration required by all stories
- [ ] T008 Define shared validation scaffolding (manual scenarios, utilities, or tests)
- [ ] T009 Update build/configuration inputs required by the feature

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - [Title] (Priority: P1) 🎯 MVP

**Goal**: [Brief description of what this story delivers]

**Independent Test**: [How to verify this story works on its own]

### Verification for User Story 1 ⚠️

> **NOTE**: Add automated coverage first when practical; otherwise define the
> manual validation steps and evidence to capture before implementation is closed.

- [ ] T010 [P] [US1] Add automated coverage or utility validation for [workflow] in tests/ or tools/
- [ ] T011 [P] [US1] Document manual validation steps for [workflow] in specs/[###-feature-name]/quickstart.md

### Implementation for User Story 1

- [ ] T012 [P] [US1] Update shared model or sketch data in src/document/ or src/sketch/
- [ ] T013 [P] [US1] Update UI or viewport support in src/ui/ or src/viewport/
- [ ] T014 [US1] Implement core behavior in the relevant src/ module (depends on T012, T013)
- [ ] T015 [US1] Add validation, undo behavior, and error handling for the workflow
- [ ] T016 [US1] Add logging and diagnostics for user story 1 operations
- [ ] T017 [US1] Refresh feature docs or in-app affordances affected by the workflow

**Checkpoint**: At this point, User Story 1 should be fully functional and testable independently

---

## Phase 4: User Story 2 - [Title] (Priority: P2)

**Goal**: [Brief description of what this story delivers]

**Independent Test**: [How to verify this story works on its own]

### Verification for User Story 2 ⚠️

- [ ] T018 [P] [US2] Add automated coverage or utility validation for [workflow] in tests/ or tools/
- [ ] T019 [P] [US2] Document manual validation steps for [workflow] in specs/[###-feature-name]/quickstart.md

### Implementation for User Story 2

- [ ] T020 [P] [US2] Update shared data or tool state in src/document/, src/sketch/, or src/tools/
- [ ] T021 [US2] Implement core behavior in the relevant src/ module
- [ ] T022 [US2] Integrate the workflow into src/ui/ or src/viewport/
- [ ] T023 [US2] Connect with User Story 1 components while preserving independent validation

**Checkpoint**: At this point, User Stories 1 AND 2 should both work independently

---

## Phase 5: User Story 3 - [Title] (Priority: P3)

**Goal**: [Brief description of what this story delivers]

**Independent Test**: [How to verify this story works on its own]

### Verification for User Story 3 ⚠️

- [ ] T024 [P] [US3] Add automated coverage or utility validation for [workflow] in tests/ or tools/
- [ ] T025 [P] [US3] Document manual validation steps for [workflow] in specs/[###-feature-name]/quickstart.md

### Implementation for User Story 3

- [ ] T026 [P] [US3] Update the required shared model, import/export, or tool data in src/
- [ ] T027 [US3] Implement core behavior in the relevant src/ module
- [ ] T028 [US3] Integrate UI, viewport, or workflow entrypoints for the feature

**Checkpoint**: All user stories should now be independently functional

---

[Add more user story phases as needed, following the same pattern]

---

## Phase N: Polish & Cross-Cutting Concerns

**Purpose**: Improvements that affect multiple user stories

- [ ] TXXX [P] Documentation updates in README.md, docs/, or specs/
- [ ] TXXX Code cleanup and refactoring
- [ ] TXXX Performance optimization across all stories
- [ ] TXXX [P] Additional automated tests or utility checks in tests/ or tools/
- [ ] TXXX Security hardening
- [ ] TXXX Run quickstart.md validation

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phase 3+)**: All depend on Foundational phase completion
  - User stories can then proceed in parallel (if staffed)
  - Or sequentially in priority order (P1 → P2 → P3)
- **Polish (Final Phase)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational (Phase 2) - No dependencies on other stories
- **User Story 2 (P2)**: Can start after Foundational (Phase 2) - May integrate with US1 but should be independently testable
- **User Story 3 (P3)**: Can start after Foundational (Phase 2) - May integrate with US1/US2 but should be independently testable

### Within Each User Story

- Automated tests (if included) MUST be written and FAIL before implementation
- Shared model/document changes before dependent UI wiring
- Core behavior before integration polish
- Story complete before moving to next priority

### Parallel Opportunities

- All Setup tasks marked [P] can run in parallel
- All Foundational tasks marked [P] can run in parallel (within Phase 2)
- Once Foundational phase completes, all user stories can start in parallel (if team capacity allows)
- All tests for a user story marked [P] can run in parallel
- Models within a story marked [P] can run in parallel
- Different user stories can be worked on in parallel by different team members

---

## Parallel Example: User Story 1

```bash
# Launch verification work for User Story 1 together:
Task: "Add automated coverage or utility validation for [workflow] in tests/ or tools/"
Task: "Document manual validation steps for [workflow] in specs/[###-feature-name]/quickstart.md"

# Launch shared implementation work for User Story 1 together:
Task: "Update shared model or sketch data in src/document/ or src/sketch/"
Task: "Update UI or viewport support in src/ui/ or src/viewport/"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL - blocks all stories)
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: Test User Story 1 independently
5. Deploy/demo if ready

### Incremental Delivery

1. Complete Setup + Foundational → Foundation ready
2. Add User Story 1 → Test independently → Deploy/Demo (MVP!)
3. Add User Story 2 → Test independently → Deploy/Demo
4. Add User Story 3 → Test independently → Deploy/Demo
5. Each story adds value without breaking previous stories

### Parallel Team Strategy

With multiple developers:

1. Team completes Setup + Foundational together
2. Once Foundational is done:
   - Developer A: User Story 1
   - Developer B: User Story 2
   - Developer C: User Story 3
3. Stories complete and integrate independently

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- Each user story should be independently completable and testable
- Verify automated tests fail before implementing when they are part of scope
- Do not commit autonomously; leave commit decisions to the user
- Stop at any checkpoint to validate story independently
- Avoid: vague tasks, same file conflicts, cross-story dependencies that break independence
