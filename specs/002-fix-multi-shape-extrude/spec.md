# Feature Specification: Fix Multi-Shape Extrude

**Feature Branch**: `main`  
**Created**: 2026-04-25  
**Status**: Draft  
**Input**: User description: "extrude bug: when user creates nultiple 2D shapes in the same sketch and this shapes don't overlap and user selects multiple faces and tries to extrude them then app throws error: wire builder failed: edges may not be connected expected behaviour is for all shapes to be extruded and multiple 3D objects created"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Extrude multiple disconnected faces (Priority: P1)

As a CAD user, I want to select several non-overlapping closed faces in one sketch and extrude them together so I can create multiple 3D objects in one step.

**Why this priority**: The current failure blocks a core modeling workflow and forces users to break a simple multi-shape operation into manual workarounds or abandon the command.

**Independent Test**: Can be fully tested by creating one sketch with at least two separate closed faces, selecting multiple faces, running one extrude operation, and confirming that the command finishes successfully and creates one 3D object per selected face.

**Acceptance Scenarios**:

1. **Given** a sketch contains two or more closed, non-overlapping faces and the user selects multiple faces, **When** the user confirms one extrude operation, **Then** the system completes without error and creates a corresponding 3D object for each selected face.
2. **Given** three disconnected sketch faces are selected for extrusion, **When** the user applies one shared extrusion distance and confirms the command, **Then** all three selected faces are extruded in that same operation and none of the selected faces are skipped.

---

### User Story 2 - Extrude only the intended faces (Priority: P2)

As a CAD user, I want the extrude command to affect only the faces I selected so I can keep additional shapes in the sketch for later operations.

**Why this priority**: Multi-face extrusion is only trustworthy if it respects the exact selection set. Users often keep several profiles in the same sketch and need precise control over which ones become solids.

**Independent Test**: Can be fully tested by creating a sketch with at least three closed faces, selecting only a subset of them, extruding once, and verifying that only the selected faces produce new 3D objects.

**Acceptance Scenarios**:

1. **Given** a sketch contains selected and unselected closed faces, **When** the user extrudes the selected faces, **Then** only the selected faces create new 3D objects and unselected faces remain available in the sketch unchanged.
2. **Given** the selected faces are spatially separate from one another, **When** the extrusion completes, **Then** the resulting 3D objects remain separate objects positioned over their source faces rather than being combined solely because they were created in one command.

---

### Edge Cases

- What happens when one or more selected items are not valid closed faces? The system should stop the operation before creating unexpected partial geometry and explain which selection cannot be extruded.
- What happens when the sketch contains both selected and unselected disconnected faces? Only the selected faces should participate in the operation.
- What happens when the user cancels the extrude workflow after selecting multiple faces? No new 3D objects should remain in the scene.

## Verification & Evidence *(mandatory)*

### Required Evidence

- **Automated Coverage**: Add or update regression coverage for extruding multiple selected sketch faces so the disconnected-profile failure is caught in future validation runs; if full workflow automation is still impractical, document the gap and pair it with the manual validation below.
- **Manual Validation**: 1. Launch elcad. 2. Create one sketch containing at least three closed, non-overlapping shapes. 3. Finish the sketch and select two or more of its faces. 4. Start the extrude tool. 5. Use one shared extrusion distance and confirm the operation. 6. Verify the operation completes without the current connectivity error. 7. Verify one separate 3D object is created for each selected face. 8. Repeat with one face left unselected and verify it is not extruded.
- **Artifacts / Measurements**: Screenshot or recording showing the source sketch, the multi-face selection, and the resulting separate 3D objects, plus reviewer notes confirming the previous error no longer appears.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST allow a user to start one extrude operation from two or more selected closed faces within the same sketch.
- **FR-002**: When the selected faces are disconnected and non-overlapping, the system MUST complete the extrusion without failing solely because the faces belong to separate loops.
- **FR-003**: On successful completion, the system MUST create a separate 3D object for each selected disconnected face.
- **FR-004**: The system MUST apply the active extrusion settings consistently to every face included in the selected set for that operation.
- **FR-005**: The system MUST extrude only the faces the user selected and MUST leave unselected sketch faces unchanged.
- **FR-006**: If any selected face cannot be extruded under the active operation, the system MUST stop before creating unexpected partial results and MUST present a clear user-facing explanation.
- **FR-007**: Canceling the multi-face extrusion workflow MUST leave the model unchanged and MUST not leave stray or partial 3D objects in the scene.
- **FR-008**: Successful multi-face extrusion MUST preserve the relative placement of the resulting 3D objects so each object aligns with its source sketch face.

### Key Entities *(include if feature involves data)*

- **Sketch Face Selection**: The set of one or more closed sketch faces the user chooses for a single extrude command.
- **Extrude Operation**: One user-initiated modeling action that applies a shared set of extrusion settings to the selected sketch faces.
- **Resulting 3D Object**: A discrete solid created from one selected sketch face and positioned according to that face's location in the sketch.
- **Sketch Profile Set**: The full collection of closed faces in a sketch, including both selected and unselected shapes.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In acceptance testing with sketches containing 2 to 5 separate closed faces, 100% of selected non-overlapping faces produce corresponding 3D objects in a single confirmed extrude operation.
- **SC-002**: Across 10 consecutive validation runs of multi-face extrusion on disconnected profiles, reviewers observe zero occurrences of the current connectivity error.
- **SC-003**: Reviewers can complete the workflow of selecting three disconnected faces and creating three resulting 3D objects in under 1 minute without needing to extrude each face separately.
- **SC-004**: In validation runs that include both selected and unselected faces in the same sketch, 100% of resulting new 3D objects correspond only to the selected faces.

## Assumptions

- The feature applies to the existing Linux desktop modeling workflow described in the repository documentation.
- Scope is limited to closed, non-overlapping sketch faces in the same sketch; overlapping, self-intersecting, or otherwise unsupported profiles continue to follow existing behavior unless separately specified.
- One multi-face extrude action uses one shared set of extrusion settings for every selected face in that command.
- When a multi-face selection includes unsupported geometry, preventing unexpected partial output is preferable to silently creating only some of the requested objects.
