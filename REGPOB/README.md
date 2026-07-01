# REGPOB EuroScope Plugin

EuroScope plugin that adds editable **REG** (registration) and **POB** (persons on board) columns to the Departure List and keeps them synchronized with the flight plan remarks field.

## Features

- **REG column** — alphanumeric registration, auto-uppercased, spaces and punctuation stripped
- **POB column** — positive integer only
- **Remarks sync** — writes `REG/XXXXX` and `P/XX/S` into the ICAO remarks string
- **Smart parser** — replaces existing tokens, preserves unrelated fields, handles `/V/` placement
- **Persistence** — in-memory cache keyed by callsign survives list refreshes and reconnects

## Example

Sample flight plan remarks (from VATSIM):

```text
PBN/A1B1C1D1L1O1S2 DOF/260623 REG/VT-IBY EET/VABF0038 OPR/IGO PER/C RMK/TCAS P/67/S /V/
```

The plugin displays:

| REG   | POB |
|-------|-----|
| VTIBY | 67  |

Editing REG to `VTIBY` and POB to `67` normalizes remarks to:

```text
PBN/A1B1C1D1L1O1S2 DOF/260623 REG/VTIBY EET/VABF0038 OPR/IGO PER/C RMK/TCAS P/67/S /V/
```

## Build Requirements

- Windows 10 or later
- Visual Studio 2022 (or 2019 with v143 toolset) with **Desktop development with C++**
- **32-bit (Win32/x86)** target — EuroScope is a 32-bit application
- `lib/EuroScopePlugInDll.lib` — EuroScope import library (included; from the [VFPC](https://github.com/VFPC/VFPC) plugin SDK bundle)

## Build Instructions

1. Open `RegPob.sln` in Visual Studio.
2. Set the solution platform to **x86** (not x64).
3. Select **Release | Win32**.
4. Build the solution (`Build` → `Build Solution`).
5. Output DLL: `bin\Win32\Release\REGPOB.dll`

### Run unit tests

Build and run the `RemarkParserTests` project, or from a Developer Command Prompt:

```bat
msbuild RegPob.sln /p:Configuration=Release /p:Platform=Win32
bin\Win32\Release\RemarkParserTests.exe
```

## Installation

1. Copy `REGPOB.dll` into your EuroScope plugin folder (same location as other `.dll` plugins).
2. Start EuroScope — the plugin loads automatically when the DLL is present.
3. Open **Other → Plug-ins** and confirm **REGPOB** appears in the list.

## Departure List Setup

SPAD (scratchpad) is editable because EuroScope wires **display + edit** together for built-in columns. Plugin columns need the same pairing on each row.

### Option A — Use the pre-wired list (recommended)

On load, the plugin opens **REGPOB Departures** with callsign, REG, and POB columns. REG and POB are click-to-edit (left button is already bound). If that list was saved with wrong columns from an older build, type `.regpob resetlist` in EuroScope to rebuild it.

### Option B — Standard Departure List

1. Open the Departure List.
2. Click **S** (column setup) on the list header.
3. Add/configure two columns on **the same row** as each field:

| Column | Tag item type | Header | Left button function |
|--------|---------------|--------|----------------------|
| Registration | `REGPOB/REG` | `REG` | **`REGPOB/REG`** |
| POB | `REGPOB/POB` | `POB` | **`REGPOB/POB`** |

4. Enable both columns via the **F** menu if needed.
5. Click a cell to edit (white inline box, same as SPAD).

Type `.regpob` in EuroScope for an in-client reminder, or `.regpob resetlist` to rebuild the plugin departure list.

**If Left button is left blank or set to NO, the column is display-only** — that is the most common reason REG/POB appear read-only while SPAD still works.

## Remarks Format

| Field | Remarks token | Example |
|-------|---------------|---------|
| Registration | `REG/<value>` | `REG/VTIBY` |
| Persons on board | `P/<n>/S` | `P/67/S` |

When `/V/` is present in remarks, REG and POB are inserted immediately before it.

## Project Structure

```text
ESPlugin/
├── RegPob.sln              Visual Studio solution
├── RegPob.vcxproj          Plugin DLL project
├── lib/include/
│   └── EuroScopePlugIn.h   EuroScope SDK header
├── src/
│   ├── RegPobPlugin.*      Main plugin (tags, events, sync)
│   ├── RemarkParser.*      ICAO remarks parse/update logic
│   ├── AircraftCache.*     Callsign-keyed persistence
│   └── Validation.*        REG/POB input validation
└── tests/
    └── RemarkParserTests.* Unit tests for remark cases
```

## Validation Rules

**REG**

- Letters A–Z and digits 0–9 only in stored value
- Input such as `VT-IBY` or `VT IBY` is normalized to `VTIBY`
- Empty or all-invalid input is rejected

**POB**

- Positive integer only (`1`, `67`, `180`)
- Letters and symbols are rejected

## License

MIT License
