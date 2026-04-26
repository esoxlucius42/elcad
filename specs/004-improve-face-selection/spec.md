# Feature Specification: Improve Face Selection

**Feature Branch**: `004-improve-face-selection`  
**Created**: 2026-04-25  
**Status**: Draft  
**Input**: User description: "sketch bug: if circle is fully inside rect selection is ok. when circle is partialy outside rect selection is wrong and 3d body can't extruded. analyze and improve face selection"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Select the correct clipped face (Priority: P1)

As a CAD user, I want to click a bounded sketch region created by a rectangle and a partially overlapping circle and have the exact visible face selected so I can extrude the intended shape.

**Why this priority**: This is the broken core workflow. When the wrong region is selected, users cannot create the expected 3D body from a common sketch layout that mixes straight and curved boundaries.

**Independent Test**: Can be fully tested by creating a rectangle and a circle that crosses the rectangle boundary, clicking the intended bounded area inside the rectangle, and confirming that the selected face matches the visible region the user clicked.

**Acceptance Scenarios**:

1. **Given** a sketch contains a rectangle and a circle that intersects the rectangle boundary, **When** the user clicks a bounded region inside the rectangle that is trimmed by the circle, **Then** the system selects only that bounded region rather than a larger incorrect face.
2. **Given** the same sketch contains two or more bounded regions created by the overlap, **When** the user clicks one specific region, **Then** only the clicked region becomes the active face selection.

---

### User Story 2 - Extrude a partially overlapping profile successfully (Priority: P2)

As a CAD user, I want a correctly selected clipped face to extrude into a valid 3D body so I can continue modeling without redrawing the sketch.

**Why this priority**: Correct face picking only delivers value if the selected region can also complete the downstream extrusion workflow. The current defect blocks both selection trust and body creation.

**Independent Test**: Can be fully tested by creating a rectangle with a circle partially outside it, selecting the intended bounded region, running one extrude operation, and confirming the command completes successfully with a body that matches the selected face.

**Acceptance Scenarios**:

1. **Given** a bounded region formed by intersecting sketch shapes is selected, **When** the user starts and confirms an extrusion, **Then** the system creates a 3D body whose footprint matches that selected bounded region.
2. **Given** a sketch where the fully enclosed circle case already works, **When** the user extrudes that existing valid face pattern, **Then** the operation still succeeds and preserves the previously correct behavior.

---

### Edge Cases

- What happens when the circle only touches the rectangle at a single point? The system should avoid presenting a misleading face if the geometry does not create a usable bounded region.
- What happens when the user clicks very near an intersection point between the line and arc boundaries? The selection should resolve to the intended bounded region or follow existing edge-picking behavior, but it must not jump to an unrelated larger face.
- What happens when several bounded regions share portions of the same circle or rectangle edges? Each region should remain individually selectable and extrudable based on where the user clicks.

## Verification & Evidence *(mandatory)*

### Required Evidence

- **Automated Coverage**: Add or update regression coverage for sketch face selection and extrusion using partially overlapping closed shapes so the incorrect-region selection is caught in future validation runs; if full workflow automation is still impractical, document the gap and pair it with the manual validation below.
- **Manual Validation**: 1. Launch elcad. 2. Create a sketch with a rectangle. 3. Add a circle that is partially outside the rectangle so the two boundaries intersect. 4. Finish the sketch. 5. Click each visible bounded region created by the overlap and confirm the highlighted face matches the clicked region. 6. Select the intended clipped region and start the extrude tool. 7. Confirm the operation finishes successfully and the resulting 3D body matches that region. 8. Repeat the validation with a fully enclosed circle to confirm the already working case still behaves correctly.
- **Artifacts / Measurements**: Screenshot or recording of the overlapping sketch, the selected clipped face, and the resulting 3D body, plus reviewer notes confirming the previous mis-selection and extrusion failure no longer reproduce.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST identify bounded sketch regions created by intersecting closed shapes, including regions whose perimeter combines straight and curved segments.
- **FR-002**: When a user clicks within a visible bounded region created by a partially overlapping circle and rectangle, the system MUST select the specific bounded region that contains the click and MUST NOT substitute a larger enclosing face.
- **FR-003**: When multiple bounded regions are present in the same overlap configuration, the system MUST keep those regions distinct so each can be selected independently.
- **FR-004**: Extruding a selected bounded region from an intersecting-shape sketch MUST complete successfully when that region forms a valid closed face.
- **FR-005**: The resulting 3D body MUST match the exact boundary of the selected bounded region, including any clipped arc or line segments that define the face.
- **FR-006**: If the visible sketch geometry does not form a valid bounded region for extrusion, the system MUST avoid presenting a misleading face selection and MUST provide a clear user-facing failure outcome instead of creating incorrect geometry.
- **FR-007**: Face selection behavior for the already working fully enclosed circle-inside-rectangle case MUST remain correct after this change.
- **FR-008**: Canceling or undoing an extrusion based on an intersecting-shape face selection MUST leave no unintended 3D geometry in the model.

### Key Entities *(include if feature involves data)*

- **Bounded Sketch Region**: A selectable area in a sketch defined by the visible closed boundary segments produced by one or more overlapping shapes.
- **Intersection Boundary**: The shared line or arc transition points where sketch shapes cross and split the sketch into separate bounded regions.
- **Face Selection Target**: The specific bounded sketch region the user clicks as the source for one extrusion.
- **Extruded Body**: The 3D result created from one selected bounded sketch region.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In validation runs covering at least 5 sketches with partially overlapping circle-and-rectangle layouts, 100% of clicks inside a bounded region select the region actually clicked.
- **SC-002**: Across 10 consecutive validation runs of the partial-overlap workflow, reviewers observe zero cases where the application selects a larger unintended face or fails to extrude a valid selected region.
- **SC-003**: Reviewers can complete the create sketch → select clipped face → extrude workflow in under 1 minute without redrawing the sketch or using workaround geometry.
- **SC-004**: In regression validation covering both partially overlapping and fully enclosed circle cases, 100% of accepted extrusions produce bodies whose footprints match the selected sketch faces.

## Assumptions

- The feature applies to closed sketch shapes that visibly define bounded regions suitable for face selection.
- Scope is limited to sketch face selection and extrusion behavior for intersecting circle-and-rectangle style profiles and comparable overlapping closed-shape cases; unrelated modeling workflows remain unchanged unless separately specified.
- Users initiate this workflow by clicking within face area, not by selecting boundary edges as the primary action.
- When a sketch arrangement does not create a valid closed bounded region, preventing misleading selection is preferable to creating partial or incorrect 3D output.
