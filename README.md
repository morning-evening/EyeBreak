# EyeBreak

**A minimal Windows tray tool that reminds you to rest your eyes every 20 minutes.**

[![Platform](https://img.shields.io/badge/Platform-Windows_7%2B-blue)](https://www.microsoft.com/windows)
[![Language](https://img.shields.io/badge/C-Win32_API-purple)](https://docs.microsoft.com/cpp/)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)
[![Size](https://img.shields.io/badge/Size-144KB-orange)](#)

---

## Overview

| | |
|---|---|
| **Size** | **~144 KB** — single executable, no installer, no dependencies |
| **Memory** | ~2 MB RAM at runtime |
| **Language** | C (Win32 API) |
| **OS** | Windows 7 SP1+ (x64) |

Double-click to run. Right-click the tray icon to control.

**[中文文档 →](README_CN.md)**

---

## Why

### The 20-20-20 Rule

The American Academy of Ophthalmology (AAO) recommends the **20-20-20 rule** for digital eye strain prevention:

> Every **20 minutes**, look at something **20 feet (6 meters)** away for **20 seconds**.

### Research

| Study | Finding | Source |
|-------|---------|--------|
| Blehm et al. (2005) | Extended near-work reduces blink rate from ~15/min to ~5/min. Regular breaks reduce symptoms by 50–65%. | [*Optometry and Vision Science*](https://doi.org/10.1097/01.OPX.0000168706.96613.C2) |
| Sheppard & Wolffsohn (2018) | Meta-analysis of 1,454 participants: the 20-20 rule significantly reduces asthenopia and dry eye symptoms. | [*BMJ Open*](https://doi.org/10.1136/bmjopen-2017-020189) |
| Jeon et al. (2021) | **Partial blink rate** correlates strongly with meibomian gland loss (r=0.811) and dry eye symptom severity (r=0.596). Incomplete blinking is a key driver of evaporative dry eye. | [*International Ophthalmology* / PMC7993415](https://doi.org/10.1007/s10792-020-03600-w) |
| Kim et al. (2021) | **Blink exercises** (every 20 min, **10 s per session** including firm eyelid closure): reduced partial blink ratio from 54% → 34% (p<0.001), improved NIBUT from 6.5s → 8.1s (n=41). | [*Cont Lens Anterior Eye*](https://doi.org/10.1016/j.clae.2020.04.014) |

> **The full recommendation:** Every 20 minutes, look 6 meters (20 ft) away for **20 seconds**, then **close eyes firmly and squeeze shut for 10 seconds** (Kim et al., *Cont Lens Anterior Eye*, 2021) — the "squeeze" step ensures complete eyelid closure, redistributes tear film and expresses meibomian lipids.

EyeBreak implements exactly this — one timer, one notification, nothing else.

---

## Features

- **20-min timer** — balloon notification when time's up
- **Lock-screen aware** — pauses and resets timer on lock, resumes on unlock via WTS API
- **Tray tooltip** — hover shows MM:SS countdown
- **Auto-start** — optional boot via HKCU\Run registry
- **Bilingual UI** — Chinese / English, switch from tray menu, persists across restarts
- **Single instance** — mutex-enforced, no duplicates
- **No trace uninstall** — just delete the exe

---

## Download

Get the latest release from **[GitHub Releases](https://github.com/morning-evening/EyeBreak/releases)**.

- Single file: `EyeBreak.exe` (~146 KB)
- No installer, no dependencies
- Just download and double-click to run

---

## Usage

### Quick Start

1. Download `EyeBreak.exe` from the link above
2. Double-click to run
3. The icon appears in the system tray
4. Right-click for menu:

```
┌──────────────────────┐
│ Test Notification    │
│ Stop Reminder        │
│ * Auto-start (ON)    │
│ ────────────────── │
│ 切换到中文            │   ← Switch language (persists)
│ ────────────────── │
│ Exit                 │
└──────────────────────┘
```

Default UI language is Chinese. Language preference persists across restarts via registry.

Right-click the tray icon to open this menu.

---

## Uninstall

Completely traceless — no installer, minimal registry footprint (`HKCU\Software\EyeBreak` for language pref only), no files outside its directory:

1. Right-click tray icon → **Exit**
2. Delete `EyeBreak.exe`
3. Done.

If auto-start was enabled, toggle it off before exiting to clean the registry key automatically. If you skip this step, the orphaned entry points to a non-existent path and Windows silently ignores it.

---

## Roadmap

- [ ] Configurable interval (not limited to 20 min)
- [ ] Custom notification text
- [ ] Quiet hours (e.g., no alerts 23:00–08:00)
- [ ] Max pause duration (force reminder after N hours)
- [ ] Custom notification sound
- [ ] About dialog

---

## License

[MIT](LICENSE)
