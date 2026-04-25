# Implementation Plan: [FEATURE]

**Branch**: `[###-feature-name]` | **Date**: [DATE] | **Spec**: [link]
**Input**: Feature specification from `/specs/[###-feature-name]/spec.md`

**Note**: This template is filled in by the `speckit.plan` agent. See
`.specify/templates/plan-template.md` for the execution workflow.

## Summary

[Extract from feature spec: primary requirement + technical approach from research]

## Technical Context

<!--
  ACTION REQUIRED: Replace the content in this section with the technical details
  for the project. The structure here is presented in advisory capacity to guide
  the iteration process.
-->

**Language/Version**: C++17  
**Primary Dependencies**: Qt6, OpenCASCADE 7.9, OpenGL 3.3, spdlog  
**Storage**: In-memory document model with project/file I/O as applicable  
**Testing**: Manual validation scenarios, build verification, project utilities,
and automated tests when introduced  
**Target Platform**: Linux desktop
**Project Type**: Native desktop CAD application  
**Performance Goals**: Interactive viewport behavior and acceptable modeling /
import-export latency for the affected workflow  
**Constraints**: Millimetre units, `document/Document` ownership boundaries,
`ELCAD_HAVE_OCCT` guards, no autonomous commits or branch creation, stay on
`main` unless explicitly directed otherwise  
**Scale/Scope**: Sketching, modeling, viewport, import/export, and supporting
desktop workflows within elcad

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- Linux-first CAD scope preserved; units and workflow impact identified.
- `document/Document`, undo, Qt signal/slot, and viewport boundaries respected.
- Native stack constraints and dependency changes are justified.
- Verification evidence is defined (automated and/or manual with concrete steps).
- Copilot/Spec Kit guidance uses custom agents, excludes autonomous commits and
  branch creation, and keeps agent work on `main` unless explicitly directed
  otherwise.

## Project Structure

### Documentation (this feature)

```text
specs/[###-feature]/
├── plan.md              # This file (speckit.plan agent output)
├── research.md          # Phase 0 output (speckit.plan agent)
├── data-model.md        # Phase 1 output (speckit.plan agent)
├── quickstart.md        # Phase 1 output (speckit.plan agent)
├── contracts/           # Phase 1 output (speckit.plan agent)
└── tasks.md             # Phase 2 output (speckit.tasks agent)
```

### Source Code (repository root)
<!--
  ACTION REQUIRED: Replace the placeholder tree below with the concrete layout
  for this feature. Delete unused options and expand the chosen structure with
  real paths (e.g., apps/admin, packages/something). The delivered plan must
  not include Option labels.
-->

```text
src/
├── app/
├── document/
├── io/
├── shaders/
├── sketch/
├── tools/
├── ui/
└── viewport/

tools/
└── sphere_tess_count*.cpp

tests/
└── ...                  # Add when automated coverage is introduced

docs/
└── ...                  # Optional user or developer documentation
```

**Structure Decision**: [Document the selected elcad directories affected by this
feature and explain any new paths added]

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| [e.g., 4th project] | [current need] | [why 3 projects insufficient] |
| [e.g., Repository pattern] | [specific problem] | [why direct DB access insufficient] |
