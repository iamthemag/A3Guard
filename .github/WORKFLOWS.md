# A3Guard GitHub Actions Workflows

This document describes the automated CI/CD workflows for A3Guard.

## Overview

Two main workflows are configured:
1. **CI Workflow** (`ci.yml`) - Continuous integration for every push/PR
2. **Build DEB Workflow** (`build-deb.yml`) - Debian package creation and release

## CI Workflow (`ci.yml`)

### Triggers
- Push to `main` or `develop` branches
- Pull requests to `main` or `develop`
- Manual trigger via workflow_dispatch

### Platform
- **OS:** Ubuntu 20.04 LTS
- **Architecture:** x86_64

### Jobs

#### 1. Build Job
Builds A3Guard in both Debug and Release modes.

**Steps:**
- Checkout code
- Install Qt5, OpenSSL, X11, udev dependencies
- Configure CMake
- Compile with `make -j$(nproc)`
- Verify binary creation
- Upload artifacts (7-day retention)

**Artifacts:**
- `a3guard-build-Release-ubuntu-20.04`
- `a3guard-build-Debug-ubuntu-20.04`

#### 2. Code Quality Job
Static analysis and code checking.

**Steps:**
- Install cppcheck
- Run `cppcheck` on src/ and include/
- Generate code quality report

#### 3. Security Job
Security vulnerability scanning.

**Steps:**
- Check for hardcoded credentials
- Verify no passwords/secrets in code
- Scan dependencies

#### 4. Documentation Job
Documentation validation.

**Steps:**
- Check existence of README.md
- Check existence of README_DETAILED.md
- Verify LICENSE file

#### 5. Summary Job
Aggregates all job results.

## Build DEB Workflow (`build-deb.yml`)

### Triggers
- Push with tags `v*` (e.g., `v1.0.0`)
- Push to `main` or `develop`
- Pull requests to `main`
- Manual trigger

### Platform
- **OS:** Ubuntu 20.04 LTS
- **Architecture:** x86_64

### Jobs

#### Build and Package Job
Creates Release and Debug builds with Debian packaging.

**Build Configurations:**
- Release (O3 optimization)
- Debug (full symbols)

**Steps:**

1. **Checkout** - Get full repository history
2. **Install Dependencies** - Qt5, build tools, packaging tools
3. **Get Version** - Extract version from git tag or use `1.0.0-<commit>`
4. **Configure CMake** - Set release mode, install prefix `/opt/a3guard`
5. **Build Project** - Compile with `make -j$(nproc)`
6. **Create Package** (Release only)
   - Run `make package` to generate `.deb` file
   - Creates Debian package ready for installation
7. **Verify Package** (Release only)
   - Extract package metadata
   - Verify contents
   - Display package size
8. **Upload Artifacts** (Release only)
   - Upload `.deb` file to GitHub (30-day retention)
9. **Create Release** (when tagged)
   - Attach `.deb` files to GitHub release
   - Include installation instructions
   - List features and requirements
10. **Print Summary** - Display build stats

### Debian Package Details

**Package Name:** `a3guard`
**Version:** 1.0.0 (or git-based)
**Architecture:** amd64
**Install Path:** `/opt/a3guard/`
**Config Path:** `/etc/a3guard/`
**Data Path:** `/var/lib/a3guard/`
**Log Path:** `/var/log/a3guard/`

**Dependencies:**
- `libqt5core5a` (>= 5.12.0)
- `libqt5gui5` (>= 5.12.0)
- `libqt5widgets5` (>= 5.12.0)
- `libssl1.1`
- `libudev1`
- `libx11-6`
- `libxfixes3`
- `libstdc++6` (>= 9)

**Suggested Dependencies:**
- `policykit-1` (for privilege elevation)
- `network-manager` (for network control)
- `xdotool` (for window monitoring)

### Release Process

To create a release with DEB package:

```bash
# 1. Tag a release
git tag -a v1.0.0 -m "Initial A3Guard release"

# 2. Push tag to GitHub
git push origin --tags

# 3. GitHub Actions will:
#    - Build the project
#    - Create DEB package
#    - Attach to GitHub release
#    - Generate release notes
```

### Icon Usage

- **Icon Path:** `A3Guard.png` (root directory)
- **Usage:** Referenced in CI/CD workflows
- **Resolution:** Used for application menu and system integration

## Environment Variables

Both workflows use:
```
BUILD_TYPE: Release
PROJECT_NAME: A3Guard
ICON_PATH: A3Guard.png
CMAKE_BUILD_TYPE: Release
```

## Artifact Retention

- **CI Artifacts:** 7 days (binary builds)
- **DEB Artifacts:** 30 days (packaged releases)
- **Release Artifacts:** Permanent (GitHub releases)

## Failed Builds

If a build fails:
1. Check workflow logs in GitHub Actions tab
2. Common issues:
   - Missing dependencies (install Ubuntu 20.04 packages)
   - CMake configuration errors
   - Qt5 version mismatch
   - Compilation errors in source code

## Local Testing

To test builds locally matching CI:

### Ubuntu 20.04
```bash
# Install dependencies
sudo apt-get install -y \
  build-essential cmake \
  qt5-default qtbase5-dev qtbase5-dev-tools \
  libssl-dev libudev-dev libx11-dev libxfixes-dev \
  pkg-config

# Build
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Package (creates .deb)
make package
```

## Troubleshooting

### Build fails: "Qt5 not found"
```bash
# Install Qt5 development files
sudo apt-get install qt5-default qtbase5-dev
```

### Package creation fails
```bash
# Ensure CMake finds all libraries
cd build
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON
make VERBOSE=1
```

### DEB package won't install
```bash
# Check dependencies
dpkg-deb --info a3guard_*.deb

# Install missing dependencies first
sudo apt-get install -f
```

## Future Enhancements

Potential workflow improvements:
- [ ] Unit testing with ctest
- [ ] Integration testing
- [ ] AppImage creation
- [ ] Snap package support
- [ ] RPM package for Fedora/RHEL
- [ ] Docker image building
- [ ] Code coverage reporting
- [ ] Performance benchmarks
- [ ] Automated changelog generation

## References

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [CMake Documentation](https://cmake.org/documentation/)
- [Debian Packaging Guide](https://wiki.debian.org/DebianPackagingGuide)
- [A3Guard README](../README_DETAILED.md)
