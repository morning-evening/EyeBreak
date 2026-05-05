# Contributing to EyeBreak

Thank you for your interest in contributing to EyeBreak! This document provides guidelines and information for contributors.

## Development Environment

### Requirements

- Windows 7 SP1 or later (64-bit)
- Visual Studio 2022 with C/C++ desktop development workload
- Git

### Setup

1. Clone the repository:
   ```bash
   git clone https://github.com/morning-evening/EyeBreak.git
   cd EyeBreak
   ```

2. Open a Developer Command Prompt for VS 2022

3. Build the project:
   ```bash
   build.bat
   ```
   Or using Make:
   ```bash
   make
   ```

## Project Structure

```
EyeBreak/
├── src/           # Source code
├── include/       # Header files
├── res/           # Resources (icons, etc.)
├── build.bat      # Windows build script
└── Makefile       # Alternative build system
```

## Code Style

- **Indentation**: 4 spaces (no tabs)
- **Brace style**: Allman (opening brace on its own line)
- **Comments**: English, Doxygen-style (`/** ... */`) for public functions, standard (`/* ... */`) for static helpers
- **Naming**: 
  - Functions: `PascalCase`
  - Variables: `g_` prefix for globals, `camelCase` for locals
  - Constants: `UPPER_SNAKE_CASE`

## How to Contribute

### Reporting Bugs

1. Check existing issues to avoid duplicates
2. Create a new issue with:
   - Clear title and description
   - Steps to reproduce
   - Expected vs actual behavior
   - Windows version and architecture

### Suggesting Features

1. Open an issue describing the feature
2. Explain the use case and benefits
3. Discuss implementation approach

### Submitting Changes

1. Fork the repository
2. Create a feature branch:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. Make your changes following the code style
4. Test thoroughly
5. Commit with clear messages:
   ```bash
   git commit -m "feat: add your feature description"
   ```
6. Push and create a Pull Request

### Commit Message Format

We follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation only
- `style`: Code style (formatting, etc.)
- `refactor`: Code refactoring
- `build`: Build system changes
- `chore`: Other changes

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
