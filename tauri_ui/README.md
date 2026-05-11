# PulseBoost Tauri UI

This folder contains the full Tauri + React frontend rebuild for PulseBoost AI.

## Stack
- React 18 + TypeScript + Vite
- Tauri window shell (`src-tauri/`)
- Zustand state stores
- Custom glassmorphism design system (no component library)

## Run
```powershell
cd tauri_ui
npm install
npm run dev
```

## Build
```powershell
cd tauri_ui
npm install
npm run build
```

## Notes
- The frontend is fully functional with mock fallback when Tauri commands/events are unavailable.
- In a real Tauri backend integration, expose commands/events referenced in `src/hooks/useCommands.ts`, `src/hooks/useSnapshot.ts`, and `src/stores/chatStore.ts`.
