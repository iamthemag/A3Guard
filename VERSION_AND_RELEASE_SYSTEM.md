# A3Guard Version and Release System

## System Overview

A3Guard now has a complete, automated version management and release system that handles:
- Version updates across the entire codebase
- Automated DEB package creation on tag push
- GitHub release management
- Security scanning
- Semantic versioning support

## Components

### 1. Version Update Script (`scripts/update-version.sh`)

**Purpose:** Automatically update all version instances in the codebase

**Files Updated:**
- CMakeLists.txt (2 instances)
- include/Common.h (1 instance)
- src/main.cpp (1 instance)
- .github/workflows/build-deb.yml (multiple instances)
- Documentation files (README.md, README_DETAILED.md, WARP.md)

**Usage:**
```bash
./scripts/update-version.sh <old-version> <new-version>
./scripts/update-version.sh 1.0.0 1.1.0
```

**Features:**
- ✅ Validates version format (X.Y.Z)
- ✅ Checks git repository status
- ✅ Creates .bak backups of modified files
- ✅ Updates all 5+ version locations
- ✅ Shows git diff preview
- ✅ Provides next steps
- ✅ Color-coded output (INFO, ✓, WARN, ERROR)

### 2. Release Workflow (`.github/workflows/release.yml`)

**Purpose:** Automatically build and release DEB packages when tags are pushed

**Triggers:**
- Manual: Run from GitHub Actions UI (workflow_dispatch)
- Automatic: On tag push matching pattern `v*`

**Jobs:**

#### Job 1: Validate Tag
- Extracts version from tag name (v1.1.0 → 1.1.0)
- Validates format
- Outputs version for other jobs

#### Job 2: Build Release
- Runs on Ubuntu 20.04
- Installs Qt5, OpenSSL, X11, udev dependencies
- Configures CMake with version from tag
- Builds optimized Release binary
- Creates DEB package
- Verifies package integrity
- Creates GitHub Release
- Attaches DEB file
- Generates release notes automatically

#### Job 3: Security Scan
- Runs Trivy vulnerability scanner
- Uploads results to GitHub Security tab
- Non-blocking (doesn't fail release if vulnerabilities found)

#### Job 4: Notify Success
- Prints success message
- Provides direct link to release page

### 3. Release Documentation (`RELEASE.md`)

Comprehensive guide including:
- Version management workflow
- Release process (5 steps)
- GitHub Actions integration details
- Manual release process
- Semantic versioning guidelines
- Troubleshooting section
- File reference guide

## Complete Release Workflow

### Step-by-Step Guide

#### 1. Update Version
```bash
# Update all version instances
./scripts/update-version.sh 1.0.0 1.1.0

# Script output:
# [INFO] Updating version...
# [✓] Updated: CMakeLists.txt
# [✓] Updated: include/Common.h
# [✓] Updated: src/main.cpp
# [✓] Updated: .github/workflows/build-deb.yml
```

#### 2. Review Changes
```bash
# See all changes
git diff

# Stage changes
git add .
```

#### 3. Commit Changes
```bash
git commit -m "Bump version: 1.0.0 → 1.1.0"
```

#### 4. Create Release Tag
```bash
git tag -a v1.1.0 -m "Release v1.1.0 - New features and improvements"
```

#### 5. Push to GitHub
```bash
# Push version commit
git push origin main

# Push tag (triggers release workflow)
git push origin v1.1.0
```

#### 6. Monitor GitHub Actions
- Go to GitHub → Actions tab
- Watch "Release - Build DEB Package" workflow
- Workflow automatically:
  - Builds project
  - Creates DEB package
  - Runs security scan
  - Creates GitHub release
  - Attaches DEB file

#### 7. Download Release
DEB package automatically available at:
```
https://github.com/yourorg/a3guard/releases/tag/v1.1.0
```

## Version Files Map

### CMakeLists.txt
```cmake
project(A3Guard 
    VERSION 1.0.0      # Line 4 - Project version
    ...
)

set(CPACK_PACKAGE_VERSION "1.0.0")  # Line 154 - CPack version
```

### include/Common.h
```cpp
#define A3GUARD_VERSION "1.0.0"  # Line 10 - App version macro
```

### src/main.cpp
```cpp
"A3Guard - Version" << A3GUARD_VERSION  # Uses macro from Common.h
```

### .github/workflows/build-deb.yml
```yaml
VERSION="1.0.0-$(git rev-parse --short HEAD)"  # Auto-extracts from tag
```

## Automation Features

### What Happens When You Push a Tag

1. **GitHub detects tag push** (v1.1.0)
2. **Release workflow triggers** (100% automatic)
3. **Version validation** - Tag format checked
4. **Build process starts** - Ubuntu 20.04 VM spins up
5. **Dependencies installed** - All build tools ready
6. **Project builds** - Complete optimization (-O3 -march=native)
7. **DEB package created** - Binary packaged
8. **Package verified** - Contents checked
9. **GitHub release created** - With version tag
10. **DEB attached** - Ready for download
11. **Release notes generated** - Installation instructions auto-added
12. **Security scan runs** - Vulnerability check
13. **Success notification** - Workflow complete

All fully automated - you only need to push the tag!

## Version Numbering Strategy

### Semantic Versioning (SemVer)

Format: **MAJOR.MINOR.PATCH**

- **MAJOR**: Breaking changes, major feature releases
- **MINOR**: New features, backwards compatible
- **PATCH**: Bug fixes, patch releases

### Examples

| From | To | Reason |
|------|-----|--------|
| 1.0.0 | 1.1.0 | New feature (minor bump) |
| 1.1.0 | 1.1.1 | Bug fix (patch bump) |
| 1.1.1 | 2.0.0 | Breaking change (major bump) |

## Release Checklist

Before releasing a new version:

- [ ] All code committed and pushed
- [ ] All tests passing (or manual testing done)
- [ ] Documentation updated
- [ ] CHANGELOG.md entry created (optional but recommended)
- [ ] Version checked: `./scripts/update-version.sh`
- [ ] Code review completed
- [ ] Backwards compatibility verified

## Troubleshooting

### Version Script Won't Run
```bash
# Solution: Make executable
chmod +x scripts/update-version.sh
./scripts/update-version.sh 1.0.0 1.1.0
```

### Script Says "Repository has uncommitted changes"
```bash
# Solution: Commit everything first
git add .
git commit -m "Latest changes"
./scripts/update-version.sh 1.0.0 1.1.0
```

### Release Workflow Didn't Trigger
```bash
# Verify tag format - must match v*
git tag -l
# Should show: v1.0.0, v1.1.0, etc.

# Re-push tag if needed
git push origin --tags
```

### DEB Package Not Created
- Check GitHub Actions logs
- Verify Qt5 dependencies installed
- Check CMake configuration
- Review workflow file syntax

## Manual Release (if needed)

For manual DEB creation without GitHub Actions:

```bash
# Configure build
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/opt/a3guard \
      -DCPACK_GENERATOR=DEB \
      -DCPACK_PACKAGE_VERSION=1.1.0 \
      ..

# Build
make -j$(nproc)

# Create package
make package

# Result: a3guard_1.1.0_amd64.deb
```

## Security Features

All releases include:
- ✅ Trivy vulnerability scanning
- ✅ DEB package integrity checks
- ✅ Git tag signature support (optional)
- ✅ GitHub release verification
- ✅ Automated dependency scanning

## Continuous Integration

The release system integrates with:
- **GitHub Actions** - Workflow automation
- **Ubuntu 20.04** - Build environment
- **CMake** - Build system
- **CPack** - Package creation
- **Trivy** - Security scanning
- **GitHub Releases** - Asset hosting

## Future Enhancements

Potential improvements:
- [ ] Automated CHANGELOG.md generation
- [ ] GPG signing of releases
- [ ] Multi-platform builds (AppImage, Snap)
- [ ] Automated version bumping
- [ ] Pre-release (alpha/beta) support
- [ ] Release notes from git commits
- [ ] Performance benchmarks per release

## Quick Reference

### Update Version
```bash
./scripts/update-version.sh 1.0.0 1.1.0
```

### Create Release
```bash
git add .
git commit -m "Bump version: 1.0.0 → 1.1.0"
git tag -a v1.1.0 -m "Release v1.1.0"
git push origin main --tags
```

### View Release
```
https://github.com/yourorg/a3guard/releases/tag/v1.1.0
```

## Files Reference

| File | Purpose |
|------|---------|
| `scripts/update-version.sh` | Version management script |
| `.github/workflows/release.yml` | Release automation |
| `RELEASE.md` | Release documentation |
| `CMakeLists.txt` | Build configuration |
| `include/Common.h` | Version definition |
| `src/main.cpp` | Version usage |

## Support

For questions or issues:
1. Read RELEASE.md for detailed guide
2. Check workflow logs: GitHub → Actions
3. Review this document
4. Verify git tag format: `git tag -l`

## Summary

A3Guard now has a **production-ready release system** that:
- ✅ Automates version updates
- ✅ Builds DEB packages on tag push
- ✅ Creates GitHub releases automatically
- ✅ Runs security scans
- ✅ Generates release notes
- ✅ Supports semantic versioning
- ✅ Includes comprehensive documentation

Ready for professional open-source development! 🚀
