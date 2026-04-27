# Implementation Plan: Multi-Object Selection

**Branch**: `008-multi-object-selection` | **Date**: 2026-04-27 | **Spec**: `specs/008-multi-object-selection/spec.md`
**Input**: Feature specification from `/specs/008-multi-object-selection/spec.md`

## Summary

Change desktop selection behavior so each click on another selectable target adds it to the current selection instead of replacing the prior selection. The design keeps persistent selection state in `document/Document`, moves the behavior change into the viewport click-policy seam that currently treats plain clicks as replace-selection, preserves explicit clear actions through empty-space clicks and the Space shortcut, and keeps downstream tools that require a single body or a sketch-specific selection validating that requirement explicitly rather than depending on implicit single-selection behavior.

## Technical Context

**Language/Version**: C++17  
**Primary Dependencies**: Qt6, OpenCASCADE 7.9, OpenGL 3.3, spdlog  
**Storage**: In-memory `Document` selection set (`std::vector<Document::SelectedItem>`), per-body selected flags for whole-body highlighting/gizmo anchoring, and sketch-owned selection state for active/completed sketches  
**Testing**: Full Release build (`cmake -B build -DCMAKE_BUILD_TYPE=Release`, `cmake --build build --parallel $(nproc)`), a focused selection regression target for document/selection-policy behavior (`cmake --build build --target selection_regression --parallel $(nproc)`, `ctest --test-dir build --output-on-failure -R selection-regression`) if added during implementation, and manual desktop validation from `specs/008-multi-object-selection/quickstart.md`  
**Target Platform**: Linux desktop  
**Project Type**: Native desktop CAD application  
**Performance Goals**: Keep click selection, empty-space deselection, and Space-key clearing visually immediate with no extra render pass or scene-wide recomputation beyond the existing pick path; preserve current viewport responsiveness for dense scenes and completed-sketch overlays  
**Constraints**: Millimetre units; `document/Document` remains the persistent selection owner; selection rendering stays under `viewport/`; UI widgets observe selection state through existing Qt signals/slots; operations that need exactly one body or one sketch face set must continue to enforce that requirement explicitly; no autonomous commits, pushes, or tags  
**Scale/Scope**: `src/document/Document.h`, `src/document/Document.cpp`, `src/ui/ViewportWidget.h`, `src/ui/ViewportWidget.cpp`, `src/ui/BodyListPanel.cpp`, `src/ui/PropertiesPanel.cpp`, `src/ui/ShortcutsDialog.cpp`, `src/ui/MainWindow.cpp`, `src/viewport/Renderer.cpp`, `tests/CMakeLists.txt`, planned focused selection tests under `tests/selection/`, feature docs in `specs/008-multi-object-selection/`, and `.github/copilot-instructions.md`

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Pre-Research Gate

- **PASS — Native Linux CAD Product Integrity**: The scope is limited to core CAD interaction behavior in the Linux desktop viewport and scene-selection workflow.
- **PASS — Document-Centered Architecture**: The plan keeps persistent selection ownership in `document/Document`, with `ViewportWidget`, panels, and renderer reacting through existing document APIs and Qt signals rather than introducing a parallel selection owner.
- **PASS — Buildable Native Stack Discipline**: The design stays inside the current C++17/CMake/Qt6/OpenGL/OCCT-compatible stack and does not add new dependencies or services.
- **PASS — Verification Evidence Required**: The plan defines both concrete manual validation for mixed-type selection behavior and a focused automated regression seam for document/selection-policy logic if implementation extracts or exposes that seam cleanly.
- **PASS — Spec Kit Alignment & Human-Controlled Delivery**: The plan uses repository custom-agent guidance, updates `.github/copilot-instructions.md`, and introduces no autonomous git actions while allowing branch use when the workflow needs it.

### Post-Design Re-Check

- **PASS — Scope**: `research.md`, `data-model.md`, and `quickstart.md` stay focused on additive selection, explicit deselection, shortcut behavior, and single-target command safety rather than unrelated modeling features.
- **PASS — Architecture**: The design preserves `Document` as the selection source of truth, keeps pick policy in `ViewportWidget`, and leaves renderer/panels as consumers of `selectionItems()` and `selectionChanged()`.
- **PASS — Native Stack**: Design artifacts require no new libraries, services, or runtime processes.
- **PASS — Verification**: Design artifacts pair a focused regression target for selection semantics with explicit reviewer-run desktop scenarios covering mixed selectable types, empty-space clearing, and Space-key clearing.
- **PASS — Delivery Control**: `copilot-instructions.md` is updated to point to `specs/008-multi-object-selection/plan.md`, and no autonomous commit, push, or tag action is introduced.

## Project Structure

### Documentation (this feature)

```text
specs/008-multi-object-selection/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── spec.md
└── checklists/
```

### Source Code (repository root)

```text
src/
├── document/
│   ├── Body.h
│   ├── Document.h
│   └── Document.cpp
├── ui/
│   ├── BodyListPanel.cpp
│   ├── MainWindow.cpp
│   ├── PropertiesPanel.cpp
│   ├── ShortcutsDialog.cpp
│   ├── ViewportWidget.h
│   └── ViewportWidget.cpp
└── viewport/
    └── Renderer.cpp

tests/
├── CMakeLists.txt
└── selection/
    ├── SelectionFixtures.cpp
    ├── SelectionFixtures.h
    ├── SelectionRegressionMain.cpp
    └── SelectionSetTest.cpp
```

**Structure Decision**: Keep selection-state mutations in `Document`, adapt viewport click and box-select policy in `ViewportWidget`, leave rendering/highlight consumption in `Renderer`, sync dock widgets through existing `selectionChanged()` observers, and add a narrow focused regression target under `tests/selection/` rather than trying to automate full GUI interaction. `contracts/` is intentionally omitted because this feature changes internal desktop CAD behavior and does not add a reusable public API, CLI surface, or protocol.

## Complexity Tracking

No constitution violations or extra complexity exceptions are required for this plan.
