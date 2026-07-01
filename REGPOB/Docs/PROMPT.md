# EuroScope Plugin Development Prompt – Registration & POB Integration for Departure List

## Context

You are developing a native C++ EuroScope plugin using the official EuroScope Plug-in SDK and APIs.

Before making any changes, thoroughly analyze:

- EuroScopePlugInDevelopment.pdf
- TopSky plugin Developer Guide
- Ground Radar plugin Developer Guide
- EuroScope Plug-in SDK headers and sample code
- Existing flight list implementation patterns
- Existing plugin examples that interact with flight plans and aircraft lists

The goal is to create a production-quality EuroScope plugin that adds editable Registration and POB fields to the Departure List and automatically synchronizes them into the flight plan remarks section.

---

# Objective

Create a EuroScope plugin that adds **two new columns** to the Departure List:

1. Registration
2. POB

These columns must be editable directly from the Departure List.

When a controller enters or modifies values in either column:

- The corresponding aircraft flight plan remarks must be automatically updated.
- Existing remarks content must be preserved.
- Registration and POB values must always remain synchronized with the departure list values.

---

# Column Requirements

## Registration Column

Column Name:

```text
REG
```

Validation:

- Alphanumeric only
- Allow letters A-Z
- Allow numbers 0-9
- Remove spaces automatically
- Convert to uppercase automatically

Examples:

```text
VTIBY
VTJFK
N123AB
A6EON
```

Invalid:

```text
VT-IBY
VT IBY
VT/IBY
```

---

## POB Column

Column Name:

```text
POB
```

Validation:

- Numbers only
- Positive integer
- No letters
- No special characters

Examples:

```text
1
2
45
180
```

Invalid:

```text
2A
12P
P12
12/3
```

---

# Flight Plan Remark Synchronization

The plugin must update the flight plan remarks field automatically.

## Registration Format

Registration must appear in remarks as:

```text
REG/<registration>
```

Example:

```text
REG/VTIBY
```

---

## POB Format

POB must appear in remarks as:

```text
P/<number>/S
```

Examples:

```text
P/1/S
P/180/S
P/45/S
```

---

# Critical Behaviour

The remarks field may contain:

- REG already present
- P already present
- Both present
- Neither present
- Arbitrary free text
- ICAO FPL fields
- Additional RMK information

The plugin must intelligently update the remark string.

---

# Example Cases

---

## Case 1

Existing Remarks:

```text
PBN/A1B1C1D1L1O1S2 DOF/260623 RMK/TCAS
```

Registration entered:

```text
VTIBY
```

Result:

```text
PBN/A1B1C1D1L1O1S2 DOF/260623 REG/VTIBY RMK/TCAS
```

---

## Case 2

Existing Remarks:

```text
PBN/A1B1C1D1L1O1S2 DOF/260623 RMK/TCAS
```

POB entered:

```text
67
```

Result:

```text
PBN/A1B1C1D1L1O1S2 DOF/260623 P/67/S RMK/TCAS
```

---

## Case 3

Existing Remarks:

```text
PBN/A1B1C1D1L1O1S2 DOF/260623 RMK/TCAS
```

Registration:

```text
VTIBY
```

POB:

```text
67
```

Result:

```text
PBN/A1B1C1D1L1O1S2 DOF/260623 REG/VTIBY P/67/S RMK/TCAS
```

---

## Case 4

Existing Remarks:

```text
PBN/A1B1C1D1L1O1S2 REG/OLDREG RMK/TCAS
```

Registration changed to:

```text
VTIBY
```

Result:

```text
PBN/A1B1C1D1L1O1S2 REG/VTIBY RMK/TCAS
```

Replace existing REG entry.

Do not create duplicates.

---

## Case 5

Existing Remarks:

```text
PBN/A1B1C1D1L1O1S2 P/12/S RMK/TCAS
```

POB changed to:

```text
67
```

Result:

```text
PBN/A1B1C1D1L1O1S2 P/67/S RMK/TCAS
```

Replace existing P entry.

Do not create duplicates.

---

## Case 6

Existing Remarks:

```text
PBN/A1B1C1D1L1O1S2 REG/OLD P/10/S RMK/TCAS
```

New values:

```text
REG = VTIBY
POB = 67
```

Result:

```text
PBN/A1B1C1D1L1O1S2 REG/VTIBY P/67/S RMK/TCAS
```

Both existing entries must be replaced.

---

# Placement Rules

The formatting must remain ICAO-like and readable.

Priority:

1. Preserve all existing remarks text.
2. Preserve spacing.
3. Preserve order of unrelated fields.
4. Never create duplicate REG entries.
5. Never create duplicate P entries.

Recommended order:

```text
... REG/XXXXX P/XX/S ...
```

If both are missing, insert them together.

---

# Special Handling for /V/

Many VATSIM flight plans contain:

```text
... P/67/S /V/
```

or

```text
... REG/VTIBY P/67/S /V/
```

Requirement:

If `/V/` exists:

- POB must appear immediately before `/V/`
- REG should also appear before `/V/`

Example:

Before:

```text
PBN/A1B1C1D1L1O1S2 /V/
```

After:

```text
PBN/A1B1C1D1L1O1S2 REG/VTIBY P/67/S /V/
```

---

If `/V/` does NOT exist:

Insert REG and P naturally into the remarks string without creating malformed spacing.

Example:

Before:

```text
PBN/A1B1C1D1L1O1S2 RMK/TCAS
```

After:

```text
PBN/A1B1C1D1L1O1S2 REG/VTIBY P/67/S RMK/TCAS
```

---

# Flight Plan Synchronization Logic

When:

- Registration column edited
- POB column edited
- Flight plan updated
- Aircraft reconnects
- Flight strip refreshes

The plugin must ensure:

- Departure List values remain synchronized
- Flight plan remarks remain synchronized
- No duplicate fields are created

---

# Persistence

Registration and POB values must survive:

- Flight plan updates
- Radar refreshes
- Aircraft position updates
- Departure list refreshes

Use callsign as the primary key.

Maintain an internal cache:

```cpp
struct AircraftExtraData
{
    std::string registration;
    int pob;
};
```

Store by callsign.

---

# Plugin Architecture

Implement:

### Flight Plan Event Handlers

Monitor:

- Flight plan creation
- Flight plan update
- Flight plan amendment
- Flight plan disconnect

Use official EuroScope SDK callbacks.

---

### Departure List Integration

Create custom list columns:

```text
REG
POB
```

Support:

- Display
- Editing
- Validation
- Sorting

---

### Remarks Parser

Create a dedicated parser module:

```cpp
RemarkParser.h
RemarkParser.cpp
```

Functions:

```cpp
std::string ExtractRegistration(...);

int ExtractPOB(...);

std::string UpdateRemarks(
    const std::string& remarks,
    const std::string& registration,
    int pob
);
```

Use regex or token-based parsing.

Never use fragile string replacement logic.

---

# Error Handling

Prevent:

- Duplicate REG fields
- Duplicate P fields
- Invalid registration values
- Invalid POB values
- Broken spacing
- Broken ICAO remark formatting

Gracefully handle malformed remarks.

---

# Code Quality Requirements

- Modern C++17
- Clean architecture
- Separation of concerns
- No hardcoded callsigns
- No memory leaks
- Thread-safe where required
- Extensive comments
- Production-ready

---

# Deliverables

Implement:

1. Plugin source code
2. Registration column
3. POB column
4. Remarks parser module
5. Flight plan synchronization engine
6. Validation layer
7. Internal cache system
8. Build instructions
9. README explaining usage

The final result should allow a controller to type Registration and POB directly in the Departure List and have the aircraft's flight plan remarks automatically updated to contain:

```text
REG/XXXXX
P/XX/S
```

with correct replacement, insertion, spacing, and `/V/` handling in all scenarios.