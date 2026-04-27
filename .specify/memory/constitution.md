<!--
Sync Impact Report
Version change: template -> 1.0.0
Modified principles:
- Template principle 1 -> I. Native Linux CAD Product Integrity
- Template principle 2 -> II. Document-Centered Architecture
- Template principle 3 -> III. Buildable Native Stack Discipline
- Template principle 4 -> IV. Verification Evidence Required
- Template principle 5 -> V. Spec Kit Alignment & Human-Controlled Delivery
Added sections:
- Project Constraints
- Delivery Workflow & Quality Gates
Removed sections:
- None
Templates requiring updates:
- ✅ .specify/templates/plan-template.md
- ✅ .specify/templates/spec-template.md
- ✅ .specify/templates/tasks-template.md
- ✅ .specify/templates/checklist-template.md
- ✅ .github/agents/speckit.analyze.agent.md
- ✅ .github/agents/speckit.checklist.agent.md
- ✅ .github/agents/speckit.clarify.agent.md
- ✅ .github/agents/speckit.git.feature.agent.md
- ✅ .github/agents/speckit.implement.agent.md
- ✅ .github/agents/speckit.specify.agent.md
Follow-up TODOs:
- None
-->
# elcad Constitution

## Core Principles

### I. Native Linux CAD Product Integrity
elcad MUST remain a Linux-first, sketch-based parametric 3D CAD application.
Changes MUST preserve millimetre-based modeling, deterministic editing behavior,
and practical STEP-centric workflows for design and 3D printing. Features that
do not advance modeling, viewport usability, sketching, transforms, import/export,
or directly supporting tooling MUST be explicitly justified in the plan.

Rationale: elcad is a focused desktop CAD product, not a general UI sandbox.
This principle keeps scope decisions anchored to modeling outcomes and protects
the behaviors users rely on.

### II. Document-Centered Architecture
Persistent model state MUST be owned and mutated through `document/Document`
and its undoable operations. UI code MUST observe state through Qt signals/slots
instead of bypassing the document model, rendering responsibilities MUST stay
under `viewport/`, and OCCT geometry logic MUST be isolated from general UI code
behind document, operation, or utility boundaries.

Rationale: the current architecture depends on `Document` as the single source of
truth, Qt signals for coordination, and clear viewport/geometry boundaries.
Preserving those seams reduces regressions and keeps undo, rendering, and sketch
state coherent.

### III. Buildable Native Stack Discipline
Production changes MUST target the established native stack: C++17, CMake,
Qt6, OpenGL 3.3, and OpenCASCADE compatibility on Linux. OCCT-dependent code
MUST be guarded with `#ifdef ELCAD_HAVE_OCCT` where required, include paths
MUST remain relative to `src/`, and new dependencies MUST use the repository's
existing CMake patterns unless a plan documents and justifies an exception.

Rationale: elcad already encodes platform, build, and dependency expectations in
its build system and instructions. Consistency here keeps the project buildable
for contributors and avoids ad hoc integration patterns.

### IV. Verification Evidence Required
Every change MUST provide reviewable verification evidence before merge. When
automated tests are practical, contributors MUST add or update them. When
automated coverage is not yet practical, the specification, plan, tasks, and
change summary MUST include explicit manual validation steps and measurable
evidence such as build results, viewport inspection scenarios, import/export
round-trips, logs, or utility output like `sphere_tess_count`.

Rationale: the repository currently relies heavily on manual validation, so the
absence of automated tests cannot mean the absence of verification. This rule
makes validation explicit, repeatable, and reviewable.

### V. Spec Kit Alignment & Human-Controlled Delivery
Spec Kit artifacts MUST stay aligned with this constitution and with the
repository's Copilot integration model. In GitHub Copilot CLI, contributors
MUST refer to repository custom agents under `.github/agents/` rather than
repository-defined `/speckit...` slash commands. Agents and contributors MUST
NOT create commits, tags, or pushes autonomously; branch creation and branch
switching MAY be performed when the workflow requires them.

Rationale: elcad has already standardized on custom agents plus manual control
over history-changing git actions. Keeping specs, templates, and guidance
aligned to that workflow prevents tool confusion while preserving the
repository's no-autocommit rule.

## Project Constraints

- The supported product target is a Linux desktop application.
- Canonical modeling units MUST remain millimetres unless a separately approved
  amendment changes that rule.
- Geometry and import/export work MUST preserve valid OCCT shape handling and
  degrade safely when OCCT functionality is unavailable.
- User-facing failures MUST follow the project's established error pattern:
  result objects with `success` and `errorMsg`, surfaced appropriately in the UI.
- Logging and diagnostics MUST use project logging macros instead of ad hoc
  console or Qt debug output in production code.

## Delivery Workflow & Quality Gates

- Feature specifications MUST define independent user scenarios, measurable
  success criteria, relevant edge cases, and required verification evidence.
- Implementation plans MUST complete the Constitution Check with explicit review
  of Linux CAD scope, document architecture, native stack constraints, verification
  evidence, and the no-autonomous-commit rule.
- Task lists MUST stay organized by user story, include verification work
  alongside implementation work, and call out documentation or quickstart
  updates when user workflows change.
- Runtime guidance such as `README.md` and `.github/copilot-instructions.md`
  MUST stay consistent with supported build steps, validation practices, and
  custom-agent entrypoints.

## Governance

This constitution supersedes conflicting local habits and template defaults.
Amendments MUST be made by updating `.specify/memory/constitution.md` directly,
including a refreshed Sync Impact Report and any dependent template or guidance
changes required to keep the repository consistent.

Versioning follows semantic versioning for governance:
- MAJOR: remove or redefine a principle in a backward-incompatible way.
- MINOR: add a new principle/section or materially expand required behavior.
- PATCH: clarify wording or make non-semantic editorial improvements.

Compliance review is mandatory for every specification, plan, task list, and
code review. Any deviation MUST be documented explicitly in the relevant plan or
review context, with rationale and owner approval; silent exceptions are not
permitted.

**Version**: 1.0.0 | **Ratified**: 2026-04-24 | **Last Amended**: 2026-04-24
