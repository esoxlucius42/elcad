# Feature Specification: Fix Sketch Hole Selection

**Feature Branch**: `main`  
**Created**: 2026-04-25  
**Status**: Draft  
**Input**: User description: "face select bug: when user creates sketch with a rectangle and circle inside rectangle and user uses extrude tool and user clicks inside rectangle area but outside circle area then entire rectangle face is selected and extruded; area inside circle should not be selected"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Select a face with a hole correctly (Priority: P1)

As a CAD user, I want a sketch face with an inner loop to stay hollow in the excluded area so an extrusion matches the shape I selected.

**Why this priority**: This is the broken core workflow. Users cannot reliably create common profiles such as plates, frames, or bosses with cutouts if inner loops are ignored during selection.

**Independent Test**: Can be fully tested by creating a rectangle with a closed circle inside it, selecting the area between the two boundaries, starting one extrude operation, and confirming the result preserves the circular opening instead of filling it.

**Acceptance Scenarios**:

1. **Given** a sketch contains a closed outer rectangle and a closed inner circle, **When** the user clicks the area inside the rectangle but outside the circle and extrudes it, **Then** the selected face excludes the circle and the resulting 3D object preserves that circular void.
2. **Given** a sketch face contains multiple closed inner loops, **When** the user selects and extrudes that face, **Then** every enclosed void remains excluded from the resulting 3D object.

---

### User Story 2 - Avoid selecting the surrounding face from inside a void (Priority: P2)

As a CAD user, I want clicks inside a hole to behave as empty space for that face so I do not accidentally extrude material where the sketch defines a cutout.

**Why this priority**: Predictable face picking is necessary for trust in the extrusion workflow. If a click inside a hole still selects the outer face, users cannot tell which material will actually be created.

**Independent Test**: Can be fully tested by creating a rectangle with a closed circle inside it, clicking inside the circle, and confirming that the surrounding face is not selected or extruded from that click.

**Acceptance Scenarios**:

1. **Given** a sketch contains a closed inner void inside a larger closed boundary, **When** the user clicks inside the inner void, **Then** the surrounding face is not selected from that click.
2. **Given** the user clicks once inside the bounded face area and once inside the inner void, **When** the two outcomes are compared, **Then** only the bounded face-area click produces a selectable extrusion source.

---

### Edge Cases

- What happens when a sketch face has several fully enclosed holes? All closed inner holes should remain excluded from both selection and extrusion.
- What happens when the user clicks exactly on an outer or inner boundary edge? Existing edge-selection behavior may take precedence, but the system must not reinterpret the hole as filled face area.
- What happens when the outer and inner loops do not define a valid bounded face? The system should avoid misleading selection and follow the existing invalid-geometry behavior for that sketch state.

## Verification & Evidence *(mandatory)*

### Required Evidence

- **Automated Coverage**: If no existing automated regression workflow can exercise sketch face picking with inner loops, document that gap and rely on the manual validation below for reviewer evidence.
- **Manual Validation**: 1. Launch elcad. 2. Create a sketch with a rectangle and a fully enclosed circle inside it. 3. Finish the sketch. 4. Click the area between the rectangle and circle and confirm it behaves as a face with a hole. 5. Start the extrude tool and confirm the preview and final result preserve the circular opening. 6. Undo, then click inside the circle and confirm the surrounding face is not selected from that click. 7. Repeat with two inner circles and confirm both voids remain excluded.
- **Artifacts / Measurements**: Screenshot or recording of the sketch, the selected face area, and the resulting extrusion with the preserved hole, plus reviewer notes confirming that clicks inside the inner void no longer select the surrounding face.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST recognize a sketch face with one closed outer boundary and one or more closed inner boundaries as a selectable bounded face whose usable area excludes the inner boundaries.
- **FR-002**: When a user clicks within the usable area between an outer boundary and an inner boundary, the system MUST select the bounded face that excludes the enclosed voids rather than treating the outer boundary as a fully filled face.
- **FR-003**: Clicking within a closed inner void MUST NOT select the surrounding bounded face from that click.
- **FR-004**: Extruding a selected bounded face with one or more inner voids MUST create a 3D result that preserves those voids.
- **FR-005**: When multiple closed inner voids exist within the same outer boundary, all such voids MUST remain excluded during both selection and extrusion.
- **FR-006**: Face selection for profiles with holes MUST remain distinct from any other nearby sketch faces so only the intended bounded face is used for extrusion.
- **FR-007**: If the visible loops do not form a valid bounded face with enclosed voids, the system MUST avoid presenting a misleading selectable face.
- **FR-008**: Canceling or undoing an extrusion from a bounded face with holes MUST leave no unintended filled geometry in the model.

### Key Entities *(include if feature involves data)*

- **Bounded Sketch Face**: A selectable sketch region defined by one outer loop and zero or more fully enclosed inner loops.
- **Inner Void Loop**: A closed loop inside a bounded sketch face that defines an excluded area rather than solid material.
- **Face Selection Target**: The region the user picks as the source for one extrude operation.
- **Extruded Result**: The 3D object produced from the selected bounded sketch face, including any preserved voids.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In validation runs covering profiles with 1 to 3 inner holes, 100% of clicks in the usable face area select a face that excludes all defined holes.
- **SC-002**: Across 10 consecutive validation runs of the rectangle-with-circle workflow, reviewers observe zero cases where a click inside the inner circle selects the surrounding face.
- **SC-003**: Reviewers can create an extrusion that preserves an internal circular opening in under 1 minute without using workaround steps such as separate cleanup operations.
- **SC-004**: In validation runs that include additional nearby sketch faces, 100% of resulting extrusions preserve only the intended material and add no material inside excluded hole regions.

## Assumptions

- The feature applies to closed sketch loops that define a valid outer boundary with fully enclosed inner loops.
- Scope is limited to selection and extrusion behavior for sketch faces with holes; other modeling workflows keep their current behavior unless separately specified.
- Inner loops are expected to be fully contained within the outer loop and not intersect or overlap it.
- When the pointer lands directly on a boundary edge, existing edge-picking precedence remains in effect as long as hole regions are not treated as filled face area.
