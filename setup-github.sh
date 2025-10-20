#!/bin/bash

# ExamGuard GitHub Repository Setup Script
# This script helps set up the GitHub repository and initial release

set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}🚀 ExamGuard GitHub Setup${NC}"
echo "=================================="

# Check if git is initialized
if [ ! -d ".git" ]; then
    echo -e "${YELLOW}Initializing git repository...${NC}"
    git init
    git add .
    git commit -m "Initial commit: ExamGuard C++/Qt5 exam monitoring system

- Complete C++/Qt5 GUI application
- AES-256 encryption for logs and screenshots  
- File integrity checking with SHA-256
- Administrator privileges required
- Network interface control (WiFi/Bluetooth/Ethernet)
- USB device monitoring
- Application and window focus tracking
- Clipboard monitoring
- Resource usage monitoring (<10% CPU, <100MB RAM)
- Custom file extensions (.eglog, .egimg, .egbak, .egint)
- Automated .deb package building with GitHub Actions
- AppImage support
- Security scanning and CI/CD pipeline
- Comprehensive documentation and README"
fi

# Check if remote is set
if ! git remote get-url origin &>/dev/null; then
    echo -e "${YELLOW}Please set up your GitHub repository remote:${NC}"
    echo "1. Create a new repository on GitHub"
    echo "2. Run: git remote add origin https://github.com/YOUR_USERNAME/examguard.git"
    echo "3. Run this script again"
    exit 1
fi

echo -e "${GREEN}✅ Repository setup complete${NC}"

# Build the project to ensure it works
echo -e "${YELLOW}Building project to verify everything works...${NC}"
if [ ! -d "build" ]; then
    mkdir build
fi

cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..

echo -e "${GREEN}✅ Build successful${NC}"

# Push to GitHub
echo -e "${YELLOW}Pushing to GitHub...${NC}"
git push -u origin main

echo -e "${GREEN}✅ Repository pushed to GitHub${NC}"

# Create initial tag for first release
echo -e "${YELLOW}Creating initial release tag...${NC}"
git tag -a v1.0.0 -m "ExamGuard v1.0.0 - Initial Release

🎉 First stable release of ExamGuard!

Features:
- Complete Qt5 C++ GUI application
- Advanced security with AES-256 encryption
- File integrity checking
- Comprehensive monitoring (USB, network, apps, clipboard)
- Real-time alerts and session summaries
- Minimal resource usage
- Automated .deb package building
- Professional installation system

Requirements:
- Ubuntu 20.04 or newer
- Qt5 development libraries
- Root/administrator privileges

Installation:
- Download the .deb package from releases
- Run: sudo dpkg -i examguard-*.deb
- Launch: sudo examguard

Security Features:
- AES-256-CBC encryption for all data
- SHA-256 file integrity verification
- Root privilege enforcement
- Secure key management
- Custom encrypted file formats"

git push origin v1.0.0

echo ""
echo -e "${GREEN}🎉 ExamGuard Repository Setup Complete!${NC}"
echo "=================================="
echo ""
echo -e "${GREEN}Next steps:${NC}"
echo "1. 📋 Your GitHub repository is ready"
echo "2. 🏗️  GitHub Actions will automatically build .deb packages"
echo "3. 📦 Check the 'Actions' tab for build status"
echo "4. 🚀 Release packages will be available in the 'Releases' section"
echo "5. 🔧 Customize the workflows in .github/workflows/ if needed"
echo ""
echo -e "${GREEN}Repository Features:${NC}"
echo "• ✅ Automated .deb package building"
echo "• ✅ AppImage generation"
echo "• ✅ Multi-platform CI/CD (Ubuntu 20.04, 22.04, 24.04)"
echo "• ✅ Security scanning with CodeQL and Trivy"
echo "• ✅ Code quality checks with clang-format and cppcheck"
echo "• ✅ Documentation generation"
echo "• ✅ Release automation on version tags"
echo ""
echo -e "${GREEN}Development Workflow:${NC}"
echo "1. 🔧 Make changes to the code"
echo "2. 📝 Commit and push: git add . && git commit -m 'Description' && git push"
echo "3. 🏷️  Create release: git tag v1.0.1 && git push origin v1.0.1"
echo "4. 📦 GitHub Actions will build and publish packages automatically"
echo ""
echo -e "${YELLOW}Important:${NC}"
echo "• The application requires root privileges to function"
echo "• Ensure proper security practices when deploying"
echo "• Review the LICENSE file for usage terms"
echo "• Check README.md for detailed installation and usage instructions"
echo ""
echo -e "${GREEN}Happy monitoring! 🔒📊${NC}"