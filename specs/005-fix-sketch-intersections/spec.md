# Feature Specification: Fix Sketch Intersections

**Feature Branch**: `[005-fix-sketch-intersections]`  
**Created**: 2026-04-26  
**Status**: Draft  
**Input**: User description: "bugs in sketch mode: user draws rectangle and then a circle whose center is one of rects corner points. circle intersects rect so that 1/4 of circle area is inside rect. user can select rectangle area outside circle and the 1/4 circle area inside the rectangle, but cannot select the remaining 3/4 circle area outside the rectangle. Selectable areas also cannot be extruded because the selected sketch face is not treated as a closed profile."

## Clarifications

### Session 2026-04-26

- Q: Which sketch geometry combinations are in scope for overlap-region detection and extrusion? → A: Any sketch geometry combination is in scope, including open/partial curves if they enclose an area.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Select every bounded overlap region (Priority: P1)

As a sketch user, I want every closed area created by overlapping sketch shapes to become individually selectable so I can work with any valid region I draw.

**Why this priority**: If users cannot select all valid regions, sketch editing and downstream solid creation break for common overlapping-shape workflows.

**Independent Test**: Create a rectangle and an overlapping circle whose center lies on a rectangle corner. Verify that all three bounded regions created by the overlap can be selected independently.

**Acceptance Scenarios**:

1. **Given** a rectangle and circle overlap to create multiple bounded regions, **When** the user clicks each visible bounded region, **Then** each region becomes selectable on its own.
2. **Given** the rectangle-circle overlap where one quarter of the circle lies inside the rectangle, **When** the user clicks the outer three-quarter circle region, **Then** that region is selectable just like the other two regions.

---

### User Story 2 - Extrude any selectable sketch face (Priority: P1)

As a modeler, I want every selectable sketch face to be a valid closed profile so I can extrude it without profile validity failures.

**Why this priority**: Selection without successful extrusion still leaves the workflow blocked and prevents users from turning sketches into solids.

**Independent Test**: Using the same overlapping rectangle-circle sketch, select each bounded region in turn and confirm that each one can be extruded successfully.

**Acceptance Scenarios**:

1. **Given** a selectable bounded region created by overlapping closed shapes, **When** the user starts an extrusion from that region, **Then** the extrusion proceeds without reporting that the profile is open or incomplete.
2. **Given** multiple selectable regions from the same sketch, **When** the user extrudes any one of them, **Then** the chosen region is treated as a complete closed face.

---

### User Story 3 - Keep regions accurate after sketch changes (Priority: P2)

As a sketch user, I want region availability to update when intersecting shapes are added, edited, or removed so the sketch always reflects the current geometry.

**Why this priority**: Users often refine sketches iteratively, and stale regions would reintroduce missing selections or invalid faces.

**Independent Test**: Create overlapping closed shapes, confirm the resulting regions, then move or remove one shape and verify that the selectable regions update to match the new sketch.

**Acceptance Scenarios**:

1. **Given** a sketch with overlapping closed shapes, **When** the user changes one shape so the overlap changes, **Then** the set of selectable bounded regions updates before the next selection attempt.
2. **Given** an overlap is removed, **When** the user returns to selection, **Then** only the bounded regions that still exist remain selectable.

---

### Edge Cases

- When two closed shapes only touch tangentially and do not create a bounded area, the system should not invent an extra selectable face.
- When overlapping shapes are edited so that a formerly closed region becomes open, that open area should no longer be selectable or extrudable as a sketch face.
- When duplicate or coincident boundaries exist along part of a region, the system should avoid creating phantom regions or hiding valid ones.
- When a user undoes or deletes one of the intersecting shapes, the selectable region set should return to the correct prior state.

## Verification & Evidence *(mandatory)*

### Required Evidence

- **Automated Coverage**: Add or update automated checks that verify region partitioning for overlapping closed shapes, independent selection of each resulting bounded region, and successful extrusion of those regions.
- **Manual Validation**: 1) Open sketch mode. 2) Draw a rectangle. 3) Draw a circle centered on one rectangle corner so one quarter of the circle lies inside the rectangle. 4) Confirm that the rectangle-minus-circle region, the quarter-circle overlap region, and the outer three-quarter circle region are each selectable. 5) Extrude each selectable region and confirm the operation succeeds. 6) Edit or delete one shape and confirm the selectable regions refresh correctly.
- **Artifacts / Measurements**: Capture evidence showing all three regions highlighted independently and at least one successful extrusion from an overlap-generated region; include any regression test results relevant to overlapping sketch faces.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST detect every bounded sketch region created when any sketch geometry combination encloses an area through intersection or overlap, including cases where open or partial curves participate in a closed boundary.
- **FR-002**: The system MUST make each detected bounded sketch region independently selectable, including regions that lie mostly outside one of the original shapes.
- **FR-003**: The system MUST treat every selectable sketch region as a complete closed profile for downstream solid-creation tools.
- **FR-004**: The system MUST prevent open or unbounded areas from being exposed as selectable sketch faces.
- **FR-005**: The system MUST refresh the set of selectable sketch regions whenever sketch edits change intersections or remove them.
- **FR-006**: The system MUST preserve existing selection and extrusion behavior for sketches containing a single non-overlapping closed shape.
- **FR-007**: The system MUST handle region detection and extrusion consistently for any supported sketch geometry combination that forms a bounded region, regardless of which entities were drawn first.

### Key Entities *(include if feature involves data)*

- **Closed Sketch Shape**: A user-drawn planar shape intended to bound an area, such as a rectangle or circle.
- **Closed Boundary Path**: A complete planar loop formed by one or more sketch entities, including combinations that use open or partial curves as long as the overall boundary is closed.
- **Intersection Region**: A bounded area produced when one or more sketch entities overlap or combine to form a closed planar boundary.
- **Selectable Sketch Face**: A bounded sketch region the user can select and pass to downstream operations such as extrusion.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In the rectangle-circle overlap scenario described in this feature, users can select all three resulting bounded regions on the first attempt.
- **SC-002**: 100% of bounded regions that are selectable in supported overlapping-shape sketches can be used to start an extrusion without profile validity failures.
- **SC-003**: In manual validation of at least five representative overlapping-shape sketches, reviewers find no missing bounded regions and no falsely selectable open areas.
- **SC-004**: After editing an overlapping sketch, the available selectable regions match the visible bounded areas before the user makes the next selection.

## Assumptions

- This feature applies to planar sketch mode workflows that already support selecting closed sketch faces and extruding them.
- Sketch geometry that forms closed planar boundaries is expected to be partitioned automatically into all valid bounded regions without requiring extra cleanup by the user.
- The scope of this feature is limited to sketch region detection, selection, and profile validity for extrusion; it does not change the behavior of completed solids after extrusion.
- Any supported sketch entity combination is in scope when the resulting geometry forms a bounded planar region, even if some participating entities are individually open or partial curves.
