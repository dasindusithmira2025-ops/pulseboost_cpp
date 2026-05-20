# PresentMon Manual Testing

PulseBoost supports benchmark frame-time evidence through the `PRESENTMON_CSV_PATH` environment variable.

This path tells PulseBoost where to read a real PresentMon-compatible CSV stream. PulseBoost does not launch, manage, or configure PresentMon automatically. You must run PresentMon separately and point PulseBoost at the CSV it is actively writing.

## Prepare A Folder

Create a dedicated folder for manual testing output, for example:

```powershell
New-Item -ItemType Directory -Force -Path D:\PulseBoostData
```

Do not hardcode this path into the app. It is only a convenient example for local testing.

## Start PresentMon Separately

Run PresentMon yourself against a real game or rendering process. Use placeholders for your environment:

```powershell
PresentMon.exe --process_name <game.exe> --output_file <path-to-csv>
```

Example shape only:

```powershell
PresentMon.exe --process_name <game.exe> --output_file D:\PulseBoostData\<session-name>.csv
```

Important:
- PresentMon must be writing to a real CSV file.
- The target process must actually be rendering frames.
- PulseBoost currently does not launch or manage PresentMon automatically.

## Check The CSV Before Launching PulseBoost

Use the helper script to inspect the file without inventing any benchmark values:

```powershell
.\scripts\check_presentmon_csv.ps1 -CsvPath <path-to-csv>
```

The script prints:
- file size
- last modified time
- number of lines
- detected header columns
- whether common PresentMon-compatible frame-time columns are present

Warnings to pay attention to:
- only headers or no rows
- file appears stale
- expected frame-time columns are missing
- process-name / PID columns are missing

## Launch PulseBoost With PRESENTMON_CSV_PATH

Use the process-scoped launcher helper:

```powershell
.\scripts\run_with_presentmon_csv.ps1 -CsvPath <path-to-csv>
```

If you want a process reminder in the console:

```powershell
.\scripts\run_with_presentmon_csv.ps1 -CsvPath <path-to-csv> -GameProcessName <game.exe>
```

This script:
- validates the CSV path
- sets `PRESENTMON_CSV_PATH` for the current PowerShell process only
- does not modify `.env`
- launches PulseBoost through the existing app launcher

## Expected UI Behavior

When the CSV is real, supported, and actively receiving rows for the game:
- Benchmark frame-time evidence can become available
- `FPS average` can become available
- `1% low FPS` can become available
- `P95 frame-time` can become available

When the CSV is missing, stale, mismatched, or unsupported:
- PulseBoost should keep frame-time evidence unavailable
- The Benchmark page should show an honest unavailable reason instead of fabricated values

## Manual Test Flow

1. Create a writable output folder such as `D:\PulseBoostData`.
2. Start PresentMon separately for the real game/process.
3. Confirm the CSV is being appended with:

```powershell
.\scripts\check_presentmon_csv.ps1 -CsvPath <path-to-csv>
```

4. Launch PulseBoost with:

```powershell
.\scripts\run_with_presentmon_csv.ps1 -CsvPath <path-to-csv> -GameProcessName <game.exe>
```

5. Run the game and render real frames.
6. Use Benchmark Mode inside PulseBoost.
7. Verify whether frame-time evidence becomes available or remains unavailable with a specific reason.

## Troubleshooting

### Missing File

If the CSV path does not exist:
- confirm the path is correct
- confirm PresentMon created the file
- rerun `check_presentmon_csv.ps1`

### Stale CSV

If the file exists but is not changing:
- confirm PresentMon is still running
- confirm it is writing to the same file
- confirm the game is actively rendering frames

### Headers-Only CSV

If the CSV has only headers:
- PresentMon may have started, but no frame rows have been written yet
- PulseBoost will keep benchmark frame-time evidence unavailable until real rows appear

### Wrong Process Name

If PresentMon is writing rows for the wrong process:
- verify the actual game executable name
- confirm the benchmarked foreground process matches the process PresentMon is capturing

### PresentMon Permission / Admin Issue

If PresentMon cannot capture the target process:
- confirm PresentMon has the required permissions for that game/runtime
- some games or anti-cheat environments may require elevated execution or may block capture

### Game Not Rendering Frames

If the game is paused, minimized, sitting in a launcher, or otherwise not rendering:
- the CSV may stay empty or may not receive useful rows
- PulseBoost should continue to show frame-time evidence as unavailable

## Current Limitation

PulseBoost reads a configured PresentMon-compatible CSV source, but it does not launch or manage PresentMon automatically yet.
