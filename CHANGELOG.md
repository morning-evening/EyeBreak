# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [1.0.0] - 2026-04-30

### Added
- Initial stable release
- 20-minute eye rest reminder with system tray notification
- Lock screen detection: pause timer on lock, resume on unlock
- Auto-start on Windows boot (via registry)
- Single instance enforcement (named mutex)
- Bilingual UI: Chinese / English, switchable from tray menu
- Tray tooltip showing countdown timer (MM:SS)
- Prebuilt binary available via GitHub Releases

### Fixed
- Fix timer not properly stopped/started when toggling reminder
- Fix timer not paused on workstation lock
- Fix tray icon not restored after Explorer restart

### Changed
- Improve code documentation with Doxygen-style comments
- Update project documentation and download links

## [0.9.0] - 2026-04-21

### Added
- Initial commit
- Basic reminder functionality
- System tray integration
- Build scripts (build.bat, Makefile)
