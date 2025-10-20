# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Development Commands

### Build Commands
```bash
# Quick development build
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Optimized release build (default)
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Clean build
rm -rf build && mkdir build && cd build
cmake .. && make -j$(nproc)
```

### Installation Commands
```bash
# Install system dependencies first
sudo apt-get install -y build-essential cmake qtbase5-dev qtbase5-dev-tools \
    libssl-dev libudev-dev libx11-dev libxfixes-dev pkg-config

# Build and install (requires root)
mkdir -p build && cd build && cmake .. && make
cd .. && sudo ./scripts/install.sh

# Create Debian package
cd build && cpack -G DEB
sudo dpkg -i a3guard-*.deb
```

### Testing and Verification
```bash
# Run binary directly (requires root)
sudo ./build/A3Guard --help
sudo ./build/A3Guard --verify-integrity
sudo ./build/A3Guard --generate-key

# Test service installation
sudo systemctl start a3guard
sudo systemctl status a3guard
sudo journalctl -u a3guard -f

# Debug mode
sudo ./build/A3Guard --verbose
```

### Code Quality
```bash
# Format code (install clang-format first)
find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Static analysis (install cppcheck first)
cppcheck src/ include/

# Check formatting
find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -style=file -dry-run -Werror
```

## Project Architecture

### System Overview
A3Guard is a C++17/Qt5 desktop application for exam monitoring with the following core architecture:

**Security-First Design**: AES-256 encryption, SHA-256 integrity checking, and privilege separation
**Modular Architecture**: Component-based design with dependency injection via shared_ptr
**Resource-Conscious**: Designed for <10% CPU and <100MB RAM usage during monitoring
**Cross-Platform**: Built on Qt5 with Linux-specific system monitoring via X11/udev

### Core Components

#### `SecurityManager` (src/SecurityManager.cpp)
- **Purpose**: Handles all encryption, decryption, and file integrity operations
- **Key Features**: AES-256-CBC encryption, SHA-256 hashing, secure file deletion
- **Dependencies**: OpenSSL for crypto operations, Qt for file operations
- **Critical Files**: Uses custom `.a3log`, `.a3img`, `.a3bak`, `.a3int` extensions

#### `MonitoringEngine` (src/MonitoringEngine.cpp)
- **Purpose**: Core monitoring logic for applications, windows, clipboard, USB devices
- **Architecture**: Event-driven with configurable polling intervals
- **System Integration**: Uses X11 for window monitoring, udev for USB detection
- **Thread Safety**: Designed for multi-threaded operation with Qt signals/slots

#### `NetworkManager` (src/NetworkManager.cpp)
- **Purpose**: Controls network interfaces (WiFi, Bluetooth, Ethernet)
- **Implementation**: Uses NetworkManager D-Bus API and iptables for traffic control
- **Security**: Can enable "airplane mode" and block all network traffic

#### `MainWindow` (src/MainWindow.cpp)
- **Purpose**: Qt5-based GUI with dashboard, logs, screenshots, and statistics tabs
- **Architecture**: Tab-based interface with real-time updates via QTimer
- **System Tray**: Supports minimization to system tray for unobtrusive monitoring

#### Configuration System
- **File**: `/etc/a3guard/a3guard.conf` (INI format)
- **Sections**: `[monitoring]`, `[network]`, `[security]`, `[alerts]`, `[ui]`, `[resources]`
- **Runtime**: Managed by `ConfigManager` with real-time config reloading

### Data Flow Architecture

1. **Initialization Phase**: SecurityManager loads/generates keys → ConfigManager reads settings → GUI initializes
2. **Monitoring Phase**: MonitoringEngine polls system → Events encrypted by SecurityManager → Logged by Logger
3. **Screenshot Capture**: Periodic encrypted screenshots saved to `/var/lib/a3guard/screenshots/`
4. **Integrity Checking**: QFileSystemWatcher detects changes → SHA-256 verification → Violations logged
5. **Network Control**: NetworkManager enforces restrictions → Traffic blocked via iptables

### Security Architecture

#### Encryption Pipeline
- **Algorithm**: AES-256-CBC with PKCS#7 padding
- **Key Management**: 256-bit keys stored in `/etc/a3guard/a3guard.key` with 600 permissions
- **IV Generation**: Random 128-bit IVs per encryption operation
- **Data Types**: All logs, screenshots, and backups encrypted with custom extensions

#### Privilege Model
- **Service User**: Dedicated `a3guard` user for data directory ownership
- **Root Privileges**: Required for system-level monitoring (X11, udev, network control)
- **File Permissions**: Strict 700 permissions on data directories, 600 on key files

#### Integrity System
- **Real-time Monitoring**: QFileSystemWatcher for immediate tamper detection  
- **Periodic Verification**: Configurable intervals for batch integrity checks
- **Hash Storage**: SHA-256 hashes stored in encrypted `.a3int` files
- **Violation Response**: Immediate logging and GUI alerts for any integrity failures

### Installation Architecture
- **System Integration**: Systemd service, PolicyKit authentication, desktop entry
- **Directory Structure**: `/opt/a3guard/` (binaries), `/etc/a3guard/` (config), `/var/lib/a3guard/` (data)
- **Dependencies**: Qt5 runtime, OpenSSL, X11/XFixes, libudev for hardware monitoring
- **Package Management**: Debian package creation via CPack with dependency management

### Resource Management
- **CPU Monitoring**: Self-monitoring with configurable limits (default 10%)
- **Memory Limits**: Resource tracking with alerts for excessive usage (default 100MB)
- **Log Rotation**: Automatic log rotation when files exceed configured size limits
- **Screenshot Cleanup**: Automatic cleanup of old screenshots based on retention policy

## Configuration Notes

### Build Types
- **Debug**: Full symbols, no optimization, verbose logging enabled
- **Release**: O3 optimization with `-march=native`, NDEBUG defined for minimal overhead
- **Profile**: Profile-guided optimization available for performance-critical deployments

### Key File Locations
- **Source**: `src/` (implementation), `include/` (headers)  
- **Config**: `config/a3guard.conf` (default settings)
- **Scripts**: `scripts/install.sh` (installation), `scripts/uninstall.sh` (removal)
- **Build**: `build/` directory (CMake out-of-source builds)

### Development Workflow
1. Make changes to source files in `src/` or `include/`
2. Build with `cmake --build build` or `make -C build`
3. Test with `sudo ./build/A3Guard` (requires root for system monitoring)
4. Install with `sudo ./scripts/install.sh` for full system integration testing
5. Use `sudo systemctl status a3guard` to verify service functionality