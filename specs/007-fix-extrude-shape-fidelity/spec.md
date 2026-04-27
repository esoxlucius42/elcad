# Feature Specification: Fix Extrude Shape Fidelity

**Feature Branch**: `main`  
**Created**: 2026-04-27  
**Status**: Draft  
**Input**: User description: "extrude tool is not working properly; see images before-extrude.png and after-extrude.png that show that extruded objects don't look like the shapes selected for extrude"

## Clarifications

### Session 2026-04-27

- Q: Should shape fidelity include internal holes and voids within a selected face, or only the outer outline? → A: Preserve the complete selected face, including any holes or internal voids.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Preserve selected profile shape (Priority: P1)

As a CAD user, I want an extruded solid to match the exact face I selected so I can trust that the 3D result represents the 2D profile I chose.

**Why this priority**: If the resulting solid does not match the selected face, the core modeling workflow becomes unreliable and users cannot safely create intended geometry.

**Independent Test**: Can be fully tested by selecting one valid closed sketch face with both straight and curved boundaries, extruding it, and confirming that the resulting solid's top face and side walls follow the same outline as the selected face.

**Acceptance Scenarios**:

1. **Given** a valid closed sketch face with straight and curved edges is selected, **When** the user confirms an extrude operation, **Then** the resulting solid matches the selected face's boundary instead of collapsing into a different or simplified shape.
2. **Given** a selected sketch face that forms a partial circular region, **When** the user extrudes it, **Then** the resulting solid preserves that curved region and does not replace it with an unrelated wedge-like outline.
3. **Given** a selected sketch face contains one or more enclosed voids, **When** the user extrudes it, **Then** the resulting solid preserves those voids instead of filling them in.

---

### User Story 2 - Keep each extrusion aligned to its source face (Priority: P2)

As a CAD user, I want each extruded result to stay aligned with the location and orientation of the face I selected so I can build multiple solids from one sketch without losing geometric correspondence.

**Why this priority**: Users often extrude several profiles from one sketch. The operation is only trustworthy when every resulting solid clearly maps back to its own selected face.

**Independent Test**: Can be fully tested by selecting multiple separated sketch faces, running one extrude workflow, and verifying that each resulting solid appears above its source face with the expected orientation and shape.

**Acceptance Scenarios**:

1. **Given** a sketch contains several separated closed faces and the user selects more than one of them, **When** the user confirms one extrude operation, **Then** the system creates one solid per selected face and each solid stays aligned with its own source face.
2. **Given** selected faces have different curved boundaries but share the same extrude settings, **When** the operation completes, **Then** none of the resulting solids borrow edges, corners, or orientation from another selected face.

---

### Edge Cases

- What happens when the selected face includes both arc segments and straight segments? The resulting solid must preserve both boundary types in the same places as the source face.
- What happens when the selected face contains internal holes or voids? The resulting solid must preserve those openings rather than treating the face as a solid outer loop.
- How does the system handle multiple separated selections in one sketch? Each resulting solid must remain distinct and correspond only to its own source face.
- What happens when a selected face cannot be extruded without changing its outline? The system must stop before creating misleading geometry and explain that the selected face could not be extruded faithfully.

## Verification & Evidence *(mandatory)*

### Required Evidence

- **Automated Coverage**: Add or update focused regression coverage for extrusion result fidelity if practical; if full workflow automation remains unavailable, document the limitation and pair it with the manual validation below.
- **Manual Validation**: 1. Launch elcad. 2. Create or open a sketch with at least one closed face that combines curved and straight boundaries. 3. Select one face and extrude it. 4. Confirm the resulting solid matches the selected face outline. 5. Repeat with multiple separated selected faces similar to the provided example. 6. Confirm each resulting solid matches its own source face in shape, placement, and orientation. 7. Verify no result appears as a collapsed wedge, mirrored outline, or otherwise unrelated profile.
- **Artifacts / Measurements**: Before-and-after screenshots for representative single-face and multi-face extrusions, plus reviewer notes confirming that the distorted-result defect no longer reproduces.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: When a user extrudes a valid closed sketch face, the system MUST create a solid whose resulting profile matches the full boundary of the selected face.
- **FR-001a**: When the selected sketch face contains internal holes or enclosed voids, the system MUST preserve those voids in the resulting solid.
- **FR-002**: The system MUST preserve curved boundaries from the selected face in the resulting solid instead of converting them into unintended straight edges or collapsed corners.
- **FR-003**: The system MUST preserve the selected face's orientation and placement so the resulting solid remains aligned with its source face.
- **FR-004**: When multiple sketch faces are selected for one extrusion workflow, the system MUST create one corresponding solid for each selected face without mixing geometry between selections.
- **FR-005**: The system MUST not rotate, mirror, or substitute the selected profile outline with a different profile during extrusion result generation.
- **FR-006**: If the system cannot produce an extrusion that preserves the selected face's outline, it MUST stop before creating misleading geometry and MUST present a clear user-facing explanation.
- **FR-007**: The system MUST leave unselected sketch faces unaffected during a multi-face extrusion workflow.

### Key Entities *(include if feature involves data)*

- **Sketch Face Selection**: A closed 2D region chosen by the user as the source profile for an extrusion.
- **Source Profile Boundary**: The complete outline of the selected face, including straight and curved segments that define the intended shape.
- **Internal Void**: An enclosed opening inside a selected face that is part of the face definition and must remain open in the resulting solid.
- **Extruded Result**: The 3D solid produced from one selected face and expected to remain faithful to that face's outline, placement, and orientation.
- **Multi-Face Extrude Set**: The collection of selected sketch faces that participate together in one extrusion workflow.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In acceptance testing across at least 10 representative sketch faces, including curved-edge profiles, 100% of resulting solids match the selected face outline without visible distortion.
- **SC-002**: Across 10 consecutive validation runs using 2 to 4 separated selected faces, reviewers observe zero cases where one result inherits the wrong outline, position, or orientation from another face.
- **SC-003**: Reviewers can compare the selected sketch faces to the created solids and confirm the correspondence for every result within 10 seconds in each validation scene.
- **SC-004**: In the validation scene modeled after the provided screenshots, every selected face produces a result that visually matches its source face and none of the results appear as wedge-like substitutes.

## Assumptions

- Scope is limited to the existing desktop extrusion workflow for valid closed sketch faces.
- A valid closed sketch face may include internal holes or voids, and preserving them is in scope for this feature.
- Existing extrusion settings, confirmation flow, and distance behavior remain in scope only insofar as needed to ensure faithful geometry results.
- Visual correspondence between a selected face and its resulting solid is the primary success measure for this feature.
- When faithful geometry cannot be produced, preventing incorrect output is preferable to creating a distorted solid.
