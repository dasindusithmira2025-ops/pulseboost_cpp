# Development Scripts

## Backend Scripts
- `debug_backend.ps1` - Debug backend with verbose logging
- `debug_frontend.ps1` - Debug frontend with dev tools
- `debug_app.ps1` - Debug full application

## Testing Scripts
- `check_app.py` - Check application health
- `check_root_page.py` - Verify root page loads

## App Entrypoints
- `..\serve_app.py` - Root FastAPI/static entrypoint used by Electron and local preview

## Usage

```powershell
# Debug backend
.\scripts\debug_backend.ps1

# Check app health
python .\scripts\check_app.py
```
