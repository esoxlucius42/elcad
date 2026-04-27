# Tasks: Double-Sided Face Rendering

**Input**: Design documents from `/specs/006-double-sided-face-rendering/`
**Prerequisites**: `plan.md` ✅, `spec.md` ✅, `research.md` ✅, `data-model.md` ✅, `quickstart.md` ✅ (`contracts/` not applicable — internal viewport/rendering fix)

**Tests**: No dedicated automated viewport regression target exists in this repository. Keep the implementation small and surgical in `src/shaders/` and `src/viewport/`, then validate with the existing Release CMake build plus the manual viewport scenarios in `specs/006-double-sided-face-rendering/quickstart.md`.

**Organization**: Tasks are grouped by user story so each story can be implemented and validated independently.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (`[US1]`, `[US2]`, `[US3]`)
- All task descriptions include exact file paths

## Path Conventions

- **Shared surface shading**: `src/shaders/phong.frag`, `src/shaders/phong.vert`
- **Viewport orchestration**: `src/viewport/Renderer.h`, `src/viewport/Renderer.cpp`, `src/viewport/MeshBuffer.cpp`
- **Feature validation docs**: `specs/006-double-sided-face-rendering/plan.md`, `specs/006-double-sided-face-rendering/quickstart.md`
- **Build validation**: `CMakeLists.txt`, `build/`

---

## Phase 1: Setup (Scope & Validation Baseline)

**Purpose**: Confirm the narrow render-path scope and refresh reviewer validation assets before code changes.

- [x] T001 Confirm the narrow rendering touchpoints for this fix in `src/shaders/phong.frag`, `src/viewport/Renderer.cpp`, `src/viewport/Renderer.h`, and `specs/006-double-sided-face-rendering/plan.md`
- [x] T002 [P] Refresh the Release-build and evidence checklist for scenarios A-D in `specs/006-double-sided-face-rendering/quickstart.md`

**Checkpoint**: Scope and validation expectations are locked to the shared shader/viewport path.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Establish one shared Phong-surface binding seam for committed bodies and preview meshes before story work begins.

**⚠️ CRITICAL**: Complete this phase before starting user story work.

- [x] T003 Add a shared Phong surface-uniform binding helper in `src/viewport/Renderer.h` and `src/viewport/Renderer.cpp` so committed bodies and preview meshes use the same camera/light/view inputs before any story-specific rendering changes

**Checkpoint**: The shared surface-rendering seam is ready for the double-sided visibility fix.

---

## Phase 3: User Story 1 - See every extruded face (Priority: P1) 🎯 MVP

**Goal**: Make a previously missing extruded face render immediately even when its orientation is reversed.

**Independent Test**: Reproduce the known missing-face extrusion and confirm the face is visible from the original viewpoint and when the camera moves to the opposite side.

### Verification for User Story 1

- [x] T004 [P] [US1] Add the known missing-face reproduction evidence checklist and expected front/back visibility notes in `specs/006-double-sided-face-rendering/quickstart.md`

### Implementation for User Story 1

- [x] T005 [US1] Implement camera-facing two-sided Phong shading in `src/shaders/phong.frag` so each surface fragment flips its shading normal toward the viewer before ambient, diffuse, fill, and specular evaluation
- [x] T006 [US1] Route committed body draws through the shared Phong helper in `src/viewport/Renderer.cpp` so opaque extruded faces keep the `uViewPos`, light, and alpha inputs required by the two-sided shader

**Checkpoint**: The known missing-face extrusion renders as a complete body from either side.

---

## Phase 4: User Story 2 - Inspect completed geometry from any angle (Priority: P1)

**Goal**: Keep extruded surfaces visible through orbit inspection so orientation alone no longer creates gaps.

**Independent Test**: Extrude a mixed-orientation body, orbit around it, and confirm no outward-facing extruded surface disappears solely because of face orientation.

### Verification for User Story 2

- [x] T007 [P] [US2] Add orbit/back-side inspection evidence steps and screenshot capture notes in `specs/006-double-sided-face-rendering/quickstart.md`

### Implementation for User Story 2

- [x] T008 [US2] Route preview/ghost extrusion draws through the shared Phong helper in `src/viewport/Renderer.cpp` so active extrude previews use the same two-sided surface shading as committed bodies
- [x] T009 [US2] Keep preview render-state transitions scoped in `src/viewport/Renderer.cpp` and `src/viewport/Renderer.h` so orbit inspection stays two-sided without re-enabling face culling on the shared surface pass

**Checkpoint**: Orbiting around committed and preview extrusions no longer causes orientation-only face disappearance.

---

## Phase 5: User Story 3 - Preserve normal rendering behavior for unaffected faces (Priority: P2)

**Goal**: Restore missing reversed faces without introducing see-through artifacts or regressions on already-correct extrusions.

**Independent Test**: Review representative existing extrusions and a closed/thin solid, then confirm unaffected faces remain complete and hidden surfaces stay occluded.

### Verification for User Story 3

- [x] T010 [P] [US3] Add unaffected-scene and hidden-surface evidence steps for scenarios C-D in `specs/006-double-sided-face-rendering/quickstart.md`

### Implementation for User Story 3

- [x] T011 [US3] Preserve depth-tested opaque rendering in `src/viewport/Renderer.cpp` while limiting the visibility change to the shared Phong surface path in `src/shaders/phong.frag`
- [x] T012 [US3] Confirm `MeshBuffer` triangle upload, face ordinals, and picking data remain unchanged in `src/viewport/MeshBuffer.cpp` and `src/viewport/Renderer.cpp` by avoiding triangle duplication, secondary opaque passes, or index remapping

**Checkpoint**: Already-correct scenes still render normally and hidden faces remain occluded.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Clean up the surgical render-path change and capture end-to-end evidence.

- [x] T013 Remove temporary rendering diagnostics and finalize inline comments in `src/shaders/phong.frag` and `src/viewport/Renderer.cpp`
- [ ] T014 Run `cmake -B build -DCMAKE_BUILD_TYPE=Release` and `cmake --build build --parallel $(nproc)` against `CMakeLists.txt`, then execute scenarios A-D with `build/bin/elcad` and capture screenshots/notes in `specs/006-double-sided-face-rendering/quickstart.md`

**Checkpoint**: The double-sided rendering fix is validated, documented, and ready for review.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 → Phase 2**: Confirm scope and validation expectations before introducing shared renderer helpers.
- **Phase 2 → Phases 3-5**: The shared Phong-surface helper in `src/viewport/Renderer.*` blocks all user story work.
- **Phase 3 (US1)**: MVP and recommended first delivery slice.
- **Phase 4 (US2)**: Starts after Phase 2, but its implementation reuses the two-sided shader behavior delivered in US1.
- **Phase 5 (US3)**: Starts after Phase 2; validate it after US1 and US2 stabilize.
- **Phase 6**: Starts after the desired user stories are complete.

### User Story Dependencies

- **US1**: No dependency on other stories after Phase 2.
- **US2**: Verification task `T007` can start after Phase 2; implementation tasks `T008-T009` depend on `T003` and the two-sided shader behavior from `T005-T006`.
- **US3**: Verification task `T010` can start after Phase 2; implementation tasks `T011-T012` depend on the shared two-sided path from `T003`, `T005`, and `T008` being in place.

### Recommended Completion Order

1. Phase 1: Setup
2. Phase 2: Foundational
3. Phase 3: US1 (MVP)
4. Phase 4: US2
5. Phase 5: US3
6. Phase 6: Polish

### Task-Level Dependency Highlights

- `T001 -> T003`
- `T003 -> T006, T008, T009`
- `T005 -> T006, T011`
- `T006 -> US1 complete`
- `T008 -> T009, T011`
- `T011 -> T014`
- `T012 -> T014`

### Parallel Opportunities

- `T001` and `T002` can run in parallel during setup.
- `T004` can run in parallel with `T005` because they touch `quickstart.md` and `phong.frag` separately.
- `T007` can run in parallel with `T008` because they touch `quickstart.md` and `Renderer.cpp` separately.
- `T010` can run in parallel with `T012` because they touch `quickstart.md` and `MeshBuffer.cpp` separately.
- After Phase 2, separate engineers can own shader work (`src/shaders/phong.frag`), renderer work (`src/viewport/Renderer.*`), and reviewer evidence (`specs/006-double-sided-face-rendering/quickstart.md`) with coordination on the shared validation criteria.

---

## Parallel Example: User Story 1

```text
T004 Add the missing-face reproduction evidence checklist in specs/006-double-sided-face-rendering/quickstart.md
T005 Implement camera-facing two-sided Phong shading in src/shaders/phong.frag
```

## Parallel Example: User Story 2

```text
T007 Add orbit/back-side inspection evidence steps in specs/006-double-sided-face-rendering/quickstart.md
T008 Route preview/ghost extrusion draws through the shared Phong helper in src/viewport/Renderer.cpp
```

## Parallel Example: User Story 3

```text
T010 Add unaffected-scene and hidden-surface evidence steps in specs/006-double-sided-face-rendering/quickstart.md
T012 Confirm MeshBuffer face ordinals/picking behavior remain unchanged in src/viewport/MeshBuffer.cpp and src/viewport/Renderer.cpp
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1 and Phase 2.
2. Deliver Phase 3 to make the known reversed-orientation extruded face render from both sides.
3. Run the US1 quickstart scenario and capture front/back evidence.
4. Stop and review before broadening validation.

### Incremental Delivery

1. Finish Setup + Foundational to lock the shared Phong path.
2. Deliver US1 and validate the known repro case.
3. Deliver US2 and validate full orbit inspection for committed and preview geometry.
4. Deliver US3 and validate already-correct scenes plus hidden-surface occlusion.
5. Finish Polish and rerun the full Release-build/manual checklist.

### Parallel Team Strategy

1. One engineer owns the shared renderer helper in `src/viewport/Renderer.h` and `src/viewport/Renderer.cpp`.
2. After Phase 2, split work across shader changes in `src/shaders/phong.frag`, preview/render-state adjustments in `src/viewport/Renderer.*`, and reviewer evidence updates in `specs/006-double-sided-face-rendering/quickstart.md`.
3. Merge with shared-file coordination on `src/viewport/Renderer.cpp` and the final validation checklist.

---

## Notes

- All tasks use the required checklist format with task IDs and exact file paths.
- `[P]` marks tasks that can proceed independently once their prerequisite phase is ready.
- No `contracts/` tasks are included because this feature does not add an external API, CLI, or protocol surface.
- The plan keeps scope centered on `src/shaders/` and `src/viewport/` and avoids geometry duplication or multi-pass opaque rendering unless implementation evidence proves the shader-only path insufficient.
- Do not create branches, switch branches, commit, push, or tag as part of this task list.
