# Feature Specification: Fix Face Highlight

**Feature Branch**: `009-fix-face-highlight`  
**Created**: 2026-04-27  
**Status**: Draft  
**Input**: User description: "there are three screenshots that show bug when selecting face to extrude, face is not correctly rendered but it is correctly extruded. selection-bug.png shows wrong selection where selected area overflows boundary. preview-ok.png and 3dobject-ok.png show that extrusion was correct"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Show the correct selected face region (Priority: P1)

As a CAD user, I want the highlighted face-selection region to match the actual face that will be extruded so I can trust what the viewport is showing before I commit an operation.

**Why this priority**: This is the reported bug. The extrusion result is already correct, but the incorrect highlight misleads users during a core modeling workflow.

**Independent Test**: Can be fully tested by selecting a face for extrusion on a body with nearby boundaries, confirming the highlighted region stays inside the intended face outline, and then verifying the extruded result matches that same region.

**Acceptance Scenarios**:

1. **Given** a user is selecting a face to extrude, **When** they click a valid face region, **Then** the visible selection highlight matches the intended face boundary and does not overflow into adjacent space.
2. **Given** a face is highlighted for extrusion, **When** the user previews or completes the extrusion, **Then** the highlighted region, previewed extrusion, and resulting solid all represent the same selected face.

---

### User Story 2 - Keep selection rendering stable during preview (Priority: P2)

As a CAD user, I want the face-selection overlay to remain visually consistent while the extrude preview is active so I can inspect the chosen face without second-guessing the preview state.

**Why this priority**: Even if the final extrusion is correct, unstable or oversized selection rendering during preview reduces confidence and makes it harder to verify the pending operation.

**Independent Test**: Can be fully tested by selecting a face, opening the extrude workflow, adjusting the preview, and confirming the selected face overlay remains aligned to the same face throughout the preview interaction.

**Acceptance Scenarios**:

1. **Given** a face has been selected for extrusion, **When** the user views the live preview, **Then** the selected face overlay remains aligned to the same bounded face region.
2. **Given** the user changes preview parameters, **When** the viewport redraws, **Then** the face-selection highlight does not expand beyond the selected face boundary.

## Edge Cases

- What happens when the selected face sits next to coplanar or nearly coplanar neighboring triangles that share edges but should not appear as one oversized highlighted region?
- How does the system behave when the user cancels the extrude operation after inspecting a highlighted face?
- What happens when the selected face belongs to imported geometry with dense tessellation or curved boundaries?

## Verification & Evidence *(mandatory)*

### Required Evidence

- **Automated Coverage**: Keep existing focused regression coverage for face-selection/extrude workflows where practical, and add or extend targeted checks only if a reliable non-visual seam exists for the underlying face-selection state.
- **Manual Validation**: 1. Open a model that reproduces the reported issue. 2. Select the face intended for extrusion. 3. Confirm the highlight stays within the intended face boundary. 4. Start the extrusion preview and confirm the preview still corresponds to the highlighted face. 5. Complete the extrusion and confirm the final solid matches the same face region.
- **Artifacts / Measurements**: Reviewer screenshots showing the incorrect highlight (`selection-bug.png`) and correct extrusion outcomes (`preview-ok.png` and the final 3D-object screenshot), plus updated screenshots or recordings showing the corrected face highlight and matching extrusion result.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST render the selected face-for-extrusion highlight only within the boundary of the face the user selected.
- **FR-002**: System MUST keep the selected face highlight visually aligned with the same face during extrusion preview updates.
- **FR-003**: System MUST ensure the highlighted face region, extrusion preview, and final extruded result represent the same selected geometry.
- **FR-004**: System MUST NOT visually include neighboring geometry in the selected face highlight when that geometry is not part of the selected extrusion face.
- **FR-005**: System MUST preserve current extrusion correctness while correcting the face-selection rendering behavior.

### Key Entities *(include if feature involves data)*

- **Selected Face Region**: The bounded face the user has chosen as the source for an extrusion workflow.
- **Face Selection Highlight**: The viewport overlay that communicates which face region is currently selected.
- **Extrusion Preview State**: The in-progress visual state that shows the pending extruded result before the operation is committed.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In reviewer validation, 100% of reproduced face-selection cases show a highlight that stays within the intended face boundary before extrusion is committed.
- **SC-002**: In reviewer validation, the highlighted face, extrusion preview, and final extruded result match in every exercised reproduction case.
- **SC-003**: Reviewers can identify the intended extrusion face without ambiguity in the corrected viewport rendering.
- **SC-004**: The fix eliminates the specific overflow behavior shown in the reported bug screenshots.

## Assumptions

- The extrusion geometry calculation is already correct, and the defect is limited to how the selected face is rendered or visualized before commit.
- The reported screenshots are representative of the bug the user wants fixed, even if the exact final 3D-object screenshot file is not present in the repository.
- The scope is limited to correcting face-selection rendering and preview consistency for extrusion workflows, not redesigning the overall extrusion tool.
