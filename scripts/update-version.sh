#!/bin/bash
# A3Guard Version Update Script
# Updates all version instances across the codebase and git tags
# Usage: ./scripts/update-version.sh <old-version> <new-version>
# Example: ./scripts/update-version.sh 1.0.0 1.1.0

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Functions
print_help() {
    echo "Usage: $0 <old-version> <new-version>"
    echo ""
    echo "Examples:"
    echo "  $0 1.0.0 1.1.0"
    echo "  $0 1.1.0 2.0.0"
    echo ""
    echo "This script will:"
    echo "  1. Update version in CMakeLists.txt"
    echo "  2. Update version in include/Common.h"
    echo "  3. Update version in src/main.cpp"
    echo "  4. Update version in CMakeLists.txt (CPACK)"
    echo "  5. Create git tag with version"
    echo "  6. Generate CHANGELOG entry"
}

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

validate_version() {
    local version=$1
    if [[ ! $version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        log_error "Invalid version format: $version"
        echo "Version must be in format: X.Y.Z (e.g., 1.0.0)"
        return 1
    fi
    return 0
}

update_file() {
    local file=$1
    local old_version=$2
    local new_version=$3
    
    if [ ! -f "$file" ]; then
        log_warn "File not found: $file"
        return 1
    fi
    
    if ! grep -q "$old_version" "$file"; then
        log_warn "Version $old_version not found in: $file"
        return 1
    fi
    
    # Create backup
    cp "$file" "$file.bak"
    
    # Update version
    sed -i "s/$old_version/$new_version/g" "$file"
    
    log_success "Updated: $file"
    return 0
}

# Main script
main() {
    if [ $# -ne 2 ]; then
        print_help
        exit 1
    fi
    
    OLD_VERSION=$1
    NEW_VERSION=$2
    
    log_info "A3Guard Version Update"
    log_info "Old version: $OLD_VERSION"
    log_info "New version: $NEW_VERSION"
    echo ""
    
    # Validate versions
    validate_version "$OLD_VERSION" || exit 1
    validate_version "$NEW_VERSION" || exit 1
    
    # Check git status
    if ! git -C "$PROJECT_ROOT" rev-parse --git-dir > /dev/null 2>&1; then
        log_error "Not a git repository"
        exit 1
    fi
    
    if ! git -C "$PROJECT_ROOT" diff --quiet; then
        log_warn "Repository has uncommitted changes. Commit them first:"
        git -C "$PROJECT_ROOT" status --short
        exit 1
    fi
    
    echo ""
    log_info "Starting version update..."
    echo ""
    
    # Update CMakeLists.txt
    update_file "$PROJECT_ROOT/CMakeLists.txt" "$OLD_VERSION" "$NEW_VERSION"
    
    # Update Common.h
    update_file "$PROJECT_ROOT/include/Common.h" "$OLD_VERSION" "$NEW_VERSION"
    
    # Update main.cpp
    update_file "$PROJECT_ROOT/src/main.cpp" "$OLD_VERSION" "$NEW_VERSION"
    
    # Update build-deb.yml
    update_file "$PROJECT_ROOT/.github/workflows/build-deb.yml" "$OLD_VERSION" "$NEW_VERSION"
    
    # Update documentation files
    log_info "Updating documentation..."
    update_file "$PROJECT_ROOT/README.md" "$OLD_VERSION" "$NEW_VERSION" || true
    update_file "$PROJECT_ROOT/README_DETAILED.md" "$OLD_VERSION" "$NEW_VERSION" || true
    update_file "$PROJECT_ROOT/WARP.md" "$OLD_VERSION" "$NEW_VERSION" || true
    
    echo ""
    log_success "All version instances updated"
    echo ""
    
    # Show summary of changes
    log_info "Changed files:"
    git -C "$PROJECT_ROOT" diff --name-only
    
    echo ""
    log_info "Changes preview:"
    git -C "$PROJECT_ROOT" diff --no-color | head -50
    
    echo ""
    echo "════════════════════════════════════════════════════════════"
    echo -e "${YELLOW}Next steps:${NC}"
    echo "════════════════════════════════════════════════════════════"
    echo ""
    echo "1. Review changes:"
    echo "   git diff"
    echo ""
    echo "2. Stage changes:"
    echo "   git add ."
    echo ""
    echo "3. Commit changes:"
    echo "   git commit -m \"Bump version: $OLD_VERSION → $NEW_VERSION\""
    echo ""
    echo "4. Create tag:"
    echo "   git tag -a v$NEW_VERSION -m \"Release v$NEW_VERSION\""
    echo ""
    echo "5. Push to GitHub:"
    echo "   git push origin main"
    echo "   git push origin v$NEW_VERSION"
    echo ""
    echo "GitHub Actions will automatically:"
    echo "  • Build the project"
    echo "  • Create Debian package"
    echo "  • Attach to GitHub release"
    echo "  • Run security scans"
    echo ""
    echo "════════════════════════════════════════════════════════════"
}

# Run main
main "$@"
