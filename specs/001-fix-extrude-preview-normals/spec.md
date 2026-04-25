# Feature Specification: Fix Extrude Preview Normals

**Feature Branch**: `001-fix-extrude-preview-normals`  
**Created**: 2026-04-24  
**Status**: Closed  
**Input**: User description: "there is a bug when extruding. user selects a face from sketch and clicks extrude tool; then preview is shown but normals apear inverted so user cant see top side of preview or other sides facing him, all he can see is back facing sides that shold be ocluded"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Read extrusion direction correctly (Priority: P1)

As a CAD user, I want the extrusion preview to show the surfaces that face me so I can confirm the shape and direction before finishing the operation.

**Why this priority**: The preview is the main decision aid for the extrusion workflow. If it appears inside-out, users cannot reliably judge the result and may abandon or misapply the tool.

**Independent Test**: Can be fully tested by selecting a valid sketch face, starting the extrude tool, and verifying that the visible top and side surfaces match the expected volume from the current camera angle.

**Acceptance Scenarios**:

1. **Given** a valid closed sketch face is selected, **When** the user starts the extrude tool from an isometric or top-facing view, **Then** the preview shows the outward-facing top and side surfaces instead of only hidden back faces.
2. **Given** an active extrusion preview, **When** the user adjusts the preview depth in the default direction, **Then** the visible surfaces continue to match the shape the user is about to create.

---

### User Story 2 - Inspect the preview from different angles (Priority: P2)

As a CAD user, I want the extrusion preview to remain visually correct while I orbit, pan, zoom, or switch views so I can inspect the shape before committing it.

**Why this priority**: Users often validate the extrusion from multiple viewpoints before accepting it. A correct preview must remain trustworthy during normal viewport inspection.

**Independent Test**: Can be fully tested by starting an extrusion preview and moving the camera through top, isometric, side, and underside viewpoints while checking that camera-facing surfaces remain visible and hidden surfaces remain occluded.

**Acceptance Scenarios**:

1. **Given** an active extrusion preview, **When** the user orbits or switches to another preset view, **Then** the preview updates so only the surfaces facing the camera remain visible.
2. **Given** an active extrusion preview, **When** the user views the preview from the opposite side of the part, **Then** the newly camera-facing surfaces become visible and the previously hidden surfaces are no longer shown through the volume.

---

### Edge Cases

- What happens when the user previews an extrusion that extends in the opposite direction from the initially expected side? The preview must still show the correct outward-facing surfaces for the chosen direction.
- How does the system handle an extrusion preview viewed nearly edge-on? The preview must not flash hidden back faces through the volume or become unreadable from normal camera movement alone.
- What happens when the user cancels the extrude tool after rotating the camera several times? The preview must disappear cleanly without leaving misleading geometry on screen.

## Verification & Evidence *(mandatory)*

### Required Evidence

- **Automated Coverage**: Automated viewport rendering regression tests are not currently practical for elcad due to the absence of a headless rendering harness or viewport snapshot infrastructure. The manual validation workflow below provides the required verification evidence.
- **Manual Validation**: 1. Launch elcad. 2. Create or open a sketch with a valid closed face. 3. Select the face and start the extrude tool. 4. Confirm the initial preview shows the top and side surfaces that face the camera. 5. Change the preview depth. 6. Orbit, pan, zoom, and switch between top, isometric, side, and opposite-side views. 7. Confirm the visible surfaces always match the current viewpoint and hidden faces do not show through. 8. Cancel and restart the tool to verify the behavior is repeatable.
- **Artifacts / Measurements**: Screenshot or short recording of the preview from at least two viewpoints, plus build/test output or reviewer notes confirming the defect no longer reproduces.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: When a user starts the extrude tool from a valid sketch face, the system MUST display a volumetric preview whose camera-facing top and side surfaces are visible from the current viewpoint.
- **FR-002**: The extrusion preview MUST hide back-facing surfaces that should be occluded by the visible volume from the current viewpoint.
- **FR-003**: The preview MUST remain visually consistent while the user changes preview depth or inspects the part with normal viewport navigation.
- **FR-004**: The preview MUST present the same visible-facing orientation regardless of whether the extrusion extends in the initially expected direction or the opposite direction.
- **FR-005**: Canceling or exiting the extrude tool MUST remove the preview cleanly without leaving misleading surfaces visible in the viewport.
- **FR-006**: The preview MUST provide enough visual clarity for a reviewer to distinguish which surfaces will be visible on the resulting extrusion before confirming the operation.

### Key Entities *(include if feature involves data)*

- **Sketch Face Selection**: A closed region chosen by the user as the source profile for the extrusion preview.
- **Extrude Preview Volume**: The temporary visual representation of the shape that would be created if the user confirms the extrusion.
- **Viewport Perspective**: The user’s current camera position and viewing angle, which determines which preview surfaces should be visible or hidden.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In acceptance testing across top, isometric, side, and opposite-side viewpoints, 100% of tested extrusion previews show camera-facing surfaces and hide hidden back faces correctly.
- **SC-002**: In at least 9 out of 10 validation runs, a reviewer can identify the intended extrusion direction within 5 seconds of activating the preview.
- **SC-003**: During 20 consecutive camera adjustments with an active extrusion preview, reviewers observe zero cases where hidden back faces remain visible through the previewed volume.
- **SC-004**: Acceptance reviewers can complete the full select-face → start-extrude → inspect-preview workflow without reporting the original “inside-out preview” defect.

## Assumptions

- The defect scope is limited to how the extrusion preview is displayed in the viewport; the underlying ability to create an extrusion from a valid sketch face already exists.
- The feature applies to the existing Linux desktop workflow described in the project documentation.
- Users inspect extrusion previews through normal viewport controls such as orbit, pan, zoom, and preset views.
- Valid closed sketch faces, including typical profiles used for 3D printing workflows, remain the expected starting point for this feature.
