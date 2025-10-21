# A3Guard Release Management

## Overview

This document describes the automated release system for A3Guard, including version management and DEB package generation.

## Version Management

### Files That Contain Version

The following files contain version numbers and must be updated when releasing:

1. **CMakeLists.txt** (Line 4)
   - Contains: `VERSION 1.0.0`
   - Also: `CPACK_PACKAGE_VERSION` (Line 154)

2. **include/Common.h** (Line 10)
   - Contains: `#define A3GUARD_VERSION "1.0.0"`

3. **src/main.cpp** (Line 24)
   - Contains: `"A3Guard - Version"` display

4. **.github/workflows/build-deb.yml**
   - Reads version from git tag automatically

5. **README.md** & **README_DETAILED.md**
   - Documentation examples

### Using the Version Update Script

The automated version update script handles all version changes:

```bash
# Make script executable (first time only)
chmod +x scripts/update-version.sh

# Update version from 1.0.0 to 1.1.0
./scripts/update-version.sh 1.0.0 1.1.0
```

#### Script Features
- ✅ Validates version format (X.Y.Z)
- ✅ Updates all source files
- ✅ Updates CMakeLists.txt
- ✅ Updates Common.h
- ✅ Updates main.cpp
- ✅ Updates documentation
- ✅ Creates backups (.bak files)
- ✅ Shows git diff preview
- ✅ Provides next steps

#### Script Output Example
```
[INFO] A3Guard Version Update
[INFO] Old version: 1.0.0
[INFO] New version: 1.1.0

[✓] Updated: CMakeLists.txt
[✓] Updated: include/Common.h
[✓] Updated: src/main.cpp
[✓] Updated: .github/workflows/build-deb.yml
[INFO] Updating documentation...

[✓] All version instances updated

[INFO] Changed files:
CMakeLists.txt
include/Common.h
src/main.cpp
.github/workflows/build-deb.yml

Next steps:
1. Review changes: git diff
2. Stage changes: git add .
3. Commit: git commit -m "Bump version: 1.0.0 → 1.1.0"
4. Create tag: git tag -a v1.1.0 -m "Release v1.1.0"
5. Push: git push origin main && git push origin v1.1.0
```

## Release Workflow

### Release Process

#### Step 1: Update Version
```bash
./scripts/update-version.sh 1.0.0 1.1.0
```

#### Step 2: Review Changes
```bash
git diff
```

#### Step 3: Commit Version Update
```bash
git add .
git commit -m "Bump version: 1.0.0 → 1.1.0"
```

#### Step 4: Create Release Tag
```bash
git tag -a v1.1.0 -m "Release v1.1.0"
```

#### Step 5: Push to GitHub
```bash
git push origin main
git push origin v1.1.0
```

### GitHub Actions Workflow

When you push a tag matching `v*` (e.g., `v1.1.0`), GitHub Actions automatically:

1. **Validate Tag** (Job 1)
   - Extracts version from tag name
   - Validates format (v-prefix required)

2. **Build Release** (Job 2)
   - Checks out code on Ubuntu 20.04
   - Installs all dependencies
   - Configures CMake with version from tag
   - Builds optimized Release binary
   - Creates DEB package
   - Verifies package contents
   - Creates GitHub Release
   - Attaches DEB file to release
   - Generates release notes

3. **Security Scan** (Job 3)
   - Runs Trivy vulnerability scanner
   - Uploads results to GitHub Security
   - Non-blocking (continues on error)

4. **Notify Success** (Job 4)
   - Prints success message
   - Provides release link

### Release Workflow File

Location: `.github/workflows/release.yml`

**Triggers:**
- Manual: Via GitHub Actions UI (workflow_dispatch)
- Automatic: When tag is pushed (push tags matching v*)

**Jobs:**
- `validate-tag` - Extract and validate version
- `build-release` - Build and package DEB
- `security-scan` - Run security checks
- `notify-success` - Notify completion

## Release Checklist

Before creating a release:

- [ ] All code changes committed
- [ ] All tests passing
- [ ] Documentation updated
- [ ] CHANGELOG.md updated (optional but recommended)
- [ ] Version numbers updated (or use script)
- [ ] Changes reviewed and approved

## GitHub Release Notes

The workflow automatically generates release notes with:
- Installation instructions
- Running instructions
- System requirements
- Feature list
- Security information

## Manual Release Process (if needed)

If you need to manually build a DEB:

```bash
# Build with specific version
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/opt/a3guard \
      -DCPACK_GENERATOR=DEB \
      -DCPACK_PACKAGE_VERSION=1.1.0 \
      ..
make -j$(nproc)
make package

# DEB file created at: a3guard_1.1.0_amd64.deb
```

## Version Numbering

A3Guard uses Semantic Versioning (SemVer):

- **Major.Minor.Patch** (e.g., 1.2.3)
  - **Major**: Major feature release or breaking change
  - **Minor**: New features, backwards compatible
  - **Patch**: Bug fixes, backwards compatible

### Examples:
- `v1.0.0` → `v1.1.0` (New features)
- `v1.1.0` → `v1.1.1` (Bug fixes)
- `v1.1.1` → `v2.0.0` (Major update)

## Troubleshooting

### Version Script Issues

**Issue:** Script says "Repository has uncommitted changes"
```bash
# Solution: Commit all changes first
git add .
git commit -m "Latest changes"
./scripts/update-version.sh 1.0.0 1.1.0
```

**Issue:** Version not updated in a file
```bash
# Solution: File may not exist or version not found
# Check which files need updates:
grep -r "1.0.0" .github src include config/
```

### Release Workflow Issues

**Issue:** DEB not created in workflow
- Check build logs in GitHub Actions
- Verify CMake configuration
- Ensure all dependencies installed

**Issue:** Release not attached to GitHub
- Verify tag matches `v*` pattern
- Check GitHub token permissions
- Verify workflow has `contents: write` permission

**Issue:** Workflow not triggering on tag push
- Ensure you're pushing to main branch
- Verify tag name matches `v*`
- Check workflow file syntax

## Files Reference

### Build Configuration
- `CMakeLists.txt` - Defines version, dependencies, build options
- `.github/workflows/release.yml` - Release automation

### Source Code
- `include/Common.h` - Version macro definition
- `src/main.cpp` - Version display

### Scripts
- `scripts/update-version.sh` - Version management script
- `scripts/install.sh` - Installation script

## Next Release Example

To release version 1.1.0:

```bash
# 1. Update version
./scripts/update-version.sh 1.0.0 1.1.0

# 2. Review
git diff

# 3. Commit
git add .
git commit -m "Bump version: 1.0.0 → 1.1.0"

# 4. Tag
git tag -a v1.1.0 -m "Release v1.1.0 - New monitoring features"

# 5. Push
git push origin main
git push origin v1.1.0

# GitHub Actions will:
# - Build on Ubuntu 20.04
# - Create DEB package
# - Attach to GitHub Release
# - Run security scan
```

The DEB package will be available at:
```
https://github.com/yourorg/a3guard/releases/tag/v1.1.0
```

## CI/CD Integration

The release workflow integrates with:
- GitHub Actions for automated builds
- Ubuntu 20.04 for build environment
- Trivy for security scanning
- GitHub Releases for asset hosting
- DEB package management

## Security & Integrity

All releases include:
- ✅ Security scanning with Trivy
- ✅ Integrity verification via DEB checksums
- ✅ Signed git tags (recommended)
- ✅ GitHub release verification

## Support & Issues

For release-related issues:
1. Check workflow logs: GitHub → Actions → Release workflow
2. Verify git tag format: `git tag -l`
3. Check version script: `./scripts/update-version.sh`
4. Review this documentation

## References

- [Semantic Versioning](https://semver.org/)
- [GitHub Actions Workflows](https://docs.github.com/en/actions)
- [Debian Package Guide](https://wiki.debian.org/DebianPackagingGuide)
- [CMake CPack Guide](https://cmake.org/cmake/help/latest/module/CPack.html)
