# A3Guard - Advanced Assessment Application

**Version:** 1.0.0  
**Purpose:** A comprehensive exam monitoring and security system designed to prevent cheating and unauthorized activities during online assessments.

## Table of Contents
1. [Project Overview](#project-overview)
2. [Architecture](#architecture)
3. [File Structure](#file-structure)
4. [Core Components](#core-components)
5. [Building](#building)
6. [Installation](#installation)
7. [Usage](#usage)
8. [Security Features](#security-features)
9. [Development](#development)

---

## Project Overview

A3Guard is a C++17 Qt5-based desktop application for comprehensive exam monitoring with the following key features:

- **Real-time Monitoring:** Application tracking, clipboard monitoring, keystroke detection, USB device monitoring
- **Network Control:** WiFi/Bluetooth/Ethernet disabling, airplane mode toggle
- **Security:** AES-256 encryption, SHA-256 integrity checking, encrypted logs and screenshots
- **System Monitoring:** CPU/memory usage tracking, file integrity verification, privilege enforcement
- **Intuitive UI:** Tabbed interface with dashboard, logs, clipboard data, keylogger, USB, and statistics

**Target Users:** Educational institutions running online exams, proctoring systems

**Security Level:** Enterprise-grade with root privilege enforcement and encrypted data storage

---

## Architecture

### System Design Pattern
```
┌─────────────────────────────────────────────────────────────┐
│                      A3Guard Application                      │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  MainWindow  │  │ AlertManager │  │  Logger      │      │
│  │   (UI)       │  │  (Alerts)    │  │  (Logging)   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│         ▲                                     ▲               │
│         └─────────────────────────────────────┘               │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │         MonitoringEngine (Central Hub)               │   │
│  │  • checkApplications()                               │   │
│  │  • checkWindows()                                    │   │
│  │  • checkClipboard()                                  │   │
│  │  • checkUSBDevices()                                 │   │
│  │  • checkKeystrokes()                                 │   │
│  └──────────────────────────────────────────────────────┘   │
│         ▲           ▲           ▲           ▲                │
│         │           │           │           │                │
│  ┌──────┴──┐  ┌─────┴────┐  ┌──┴──────┐  ┌─┴─────────┐    │
│  │ Network │  │ Resource │  │ Security│  │ Privilege │    │
│  │Manager  │  │ Monitor  │  │Manager  │  │ Dialog    │    │
│  └─────────┘  └──────────┘  └─────────┘  └───────────┘    │
│                                                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ConfigManager │  │ SystemCtrlr  │  │SessionMgr    │      │
│  │  (Config)    │  │ (System)     │  │ (Sessions)   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│                                                               │
└─────────────────────────────────────────────────────────────┘
         │                          │                  │
         ▼                          ▼                  ▼
    ┌─────────────┐            ┌──────────┐     ┌──────────┐
    │  Qt5 Core   │            │ OpenSSL  │     │  X11/   │
    │  Widgets    │            │ Crypto   │     │ udev    │
    └─────────────┘            └──────────┘     └──────────┘
```

### Security-First Design
- **Encryption Pipeline:** All logs, screenshots, and backups encrypted with AES-256-CBC
- **Privilege Separation:** App runs with minimal privileges initially, elevates via pkexec
- **Integrity Verification:** SHA-256 hashing with real-time tamper detection
- **Resource Limits:** Self-monitoring to prevent system abuse (<10% CPU, <100MB RAM)

---

## File Structure

```
A3Guard/
├── CMakeLists.txt                 # CMake build configuration
├── LICENSE                         # Software license
├── WARP.md                        # WARP IDE guidance
├── README.md                      # Original README (quick start)
├── README_DETAILED.md             # This file
│
├── include/                       # C++ Header files
│   ├── Common.h                   # Common enums, structures, constants
│   ├── MainWindow.h               # Main UI window interface
│   ├── SecurityManager.h          # Encryption/decryption interface
│   ├── ConfigManager.h            # Configuration management interface
│   ├── Logger.h                   # Event logging interface
│   ├── MonitoringEngine.h         # Core monitoring interface
│   ├── NetworkManager.h           # Network control interface
│   ├── SystemController.h         # System integration interface
│   ├── AlertManager.h             # Alert/notification interface
│   ├── ResourceMonitor.h          # Resource usage interface
│   ├── SessionManager.h           # Session management interface
│   ├── PrivilegeDialog.h          # Privilege elevation dialog
│   └── KeyLogger.h                # Keystroke logging interface
│
├── src/                           # C++ Implementation files
│   ├── main.cpp                   # Entry point, QApplication setup
│   ├── MainWindow.cpp             # UI implementation, event handlers
│   ├── SecurityManager.cpp        # Encryption/decryption implementation
│   ├── ConfigManager.cpp          # Config file parsing and management
│   ├── Logger.cpp                 # Log file creation and writing
│   ├── MonitoringEngine.cpp       # Core monitoring logic
│   ├── NetworkManager.cpp         # Network interface control via D-Bus
│   ├── SystemController.cpp       # System commands execution
│   ├── AlertManager.cpp           # Alert notifications (tray)
│   ├── ResourceMonitor.cpp        # CPU/memory monitoring from /proc
│   ├── SessionManager.cpp         # Session state and history
│   ├── PrivilegeDialog.cpp        # Privilege elevation UI and logic
│   └── KeyLogger.cpp              # Keystroke detection from /proc/interrupts
│
├── config/                        # Configuration files
│   └── a3guard.conf              # Default configuration (INI format)
│
├── resources/                     # Qt resource files
│   └── a3guard.qrc               # Resource file definitions (icons, etc)
│
├── scripts/                       # Utility scripts
│   └── install.sh                # Installation script
│
└── build/                        # Build output directory (created by cmake)
    └── A3Guard                   # Compiled executable
```

---

## Core Components

### 1. **MainWindow** (`include/MainWindow.h`, `src/MainWindow.cpp`)
**Purpose:** Main GUI window with tabbed interface

**Key Features:**
- Tabbed interface: Dashboard, Clipboard, Keylogger, USB, Logs, Statistics
- Real-time monitoring status display
- System tray integration for minimize/restore
- Session history tracking
- Report generation and export

**Key Methods:**
- `setupUI()` - Initialize all UI components
- `toggleMonitoring()` - Start/stop monitoring
- `updateUI()` - Periodic UI updates
- `checkPrivileges()` - Privilege elevation dialog
- `updateClipboardTabDisplay()` - Display clipboard data
- `updateKeyloggerDisplay()` - Display keystroke data
- `updateUsbDisplay()` - Display USB device info
- `updateLogs()` - Refresh log display

**Signals Emitted:** None directly (receives signals from monitoring engine)

---

### 2. **SecurityManager** (`include/SecurityManager.h`, `src/SecurityManager.cpp`)
**Purpose:** Handles all encryption, decryption, and file integrity operations

**Key Features:**
- AES-256-CBC encryption/decryption
- SHA-256 hashing for integrity checking
- Secure file deletion (overwrite before delete)
- Key generation and management
- Encrypted file format: `.a3log`, `.a3img`, `.a3bak`, `.a3int`

**Key Methods:**
- `initialize()` - Load/generate encryption keys
- `encrypt()` - Encrypt data with AES-256-CBC
- `decrypt()` - Decrypt encrypted data
- `calculateHash()` - Generate SHA-256 hash
- `verifyFileIntegrity()` - Check if file has been tampered
- `verifyDirectoryIntegrity()` - Batch integrity check
- `secureDelete()` - Securely delete file

**File Format:** Custom `.a3*` extensions with header + IV + encrypted data

---

### 3. **MonitoringEngine** (`include/MonitoringEngine.h`, `src/MonitoringEngine.cpp`)
**Purpose:** Core event-driven monitoring system for applications, windows, clipboard, USB, keystrokes

**Key Features:**
- Polls system state at configurable intervals (5s apps, 2s clipboard, 5s USB, 2s keystrokes)
- X11-based window tracking
- USB device detection via `lsblk` and `udevadm`
- Keystroke detection from `/proc/interrupts`
- Clipboard monitoring via Qt signals
- Auto-unmount USB devices on detection

**Key Methods:**
- `startMonitoring()` - Start all monitoring timers
- `stopMonitoring()` - Stop all monitoring
- `checkApplications()` - Scan running processes
- `checkWindows()` - Get current active window
- `checkClipboard()` - Monitor clipboard changes
- `checkUSBDevices()` - Detect USB insertions/removals
- `checkKeystrokes()` - Monitor keyboard activity
- `unmountUSBDevice()` - Auto-unmount USB partitions

**Signals Emitted:**
- `suspiciousActivityDetected(QString)` - General activity alert
- `windowChanged(QString)` - Window focus changed
- `clipboardChanged()` - Clipboard data changed
- `keystrokeDetected(QString)` - Keyboard activity detected
- `usbDeviceDetected(QString)` - USB device connected

---

### 4. **NetworkManager** (`include/NetworkManager.h`, `src/NetworkManager.cpp`)
**Purpose:** Controls network interfaces (WiFi, Bluetooth, Ethernet)

**Key Features:**
- D-Bus integration with NetworkManager
- Enables/disables WiFi, Bluetooth, Ethernet
- "Airplane mode" functionality
- Real-time network state monitoring
- Command execution via `nmcli` or `iptables`

**Key Methods:**
- `enableAirplaneMode()` - Disable all network interfaces
- `disableAirplaneMode()` - Enable network interfaces
- `disableWiFi()` / `enableWiFi()` - Toggle WiFi
- `disableBluetooth()` / `enableBluetooth()` - Toggle Bluetooth
- `disableEthernet()` / `enableEthernet()` - Toggle Ethernet
- `blockAllNetworkTraffic()` - Block traffic via iptables
- `allowNetworkTraffic()` - Allow traffic

**Dependencies:** NetworkManager (D-Bus), nmcli, iptables

---

### 5. **ConfigManager** (`include/ConfigManager.h`, `src/ConfigManager.cpp`)
**Purpose:** Manages configuration file parsing and application settings

**Configuration Sections:**
```ini
[monitoring]
screenshot_interval=120000
network_check_interval=30000
app_monitor_interval=5000
clipboard_interval=2000

[network]
disable_wifi=true
disable_bluetooth=true
disable_ethernet=false
airplane_mode=false

[security]
enable_encryption=true
encrypt_screenshots=true
encrypt_logs=true
enable_integrity_check=true

[alerts]
show_alerts=true
log_violations=true

[ui]
show_dashboard=true
show_clipboard=true
show_keylogger=true
show_usb=true

[resources]
max_cpu_usage=10.0
max_memory_mb=100
log_max_size_mb=10
```

**Key Methods:**
- `initialize()` - Load config from file
- `getScreenshotInterval()` - Get screenshot interval
- `getAllowLocalhost()` - Check if localhost allowed
- `getWhitelistedApplications()` - Get app whitelist

---

### 6. **Logger** (`include/Logger.h`, `src/Logger.cpp`)
**Purpose:** Event logging with encryption support

**Key Features:**
- Encrypted log file writing (`.a3log` format)
- Automatic log rotation based on size
- Timestamp and severity level tracking
- Batch file logging from memory
- Log retrieval by time range

**Key Methods:**
- `initialize()` - Create log files and directories
- `log()` - Write log entry
- `logEvent()` - Log monitoring event
- `rotateLogFiles()` - Manage log size
- `getLogsFromLastHours()` - Retrieve recent logs

**Log Format:** `[TIMESTAMP] [LEVEL] [COMPONENT] Message`

---

### 7. **AlertManager** (`include/AlertManager.h`, `src/AlertManager.cpp`)
**Purpose:** System tray notifications and alerts

**Key Features:**
- Qt system tray integration
- Alert levels: INFO, WARNING, CRITICAL, VIOLATION
- Non-intrusive notifications
- Context menu in tray
- Alert history tracking

**Key Methods:**
- `showAlert()` - Display alert message
- `showTrayNotification()` - Show system notification
- `setAlertLevel()` - Set severity level

---

### 8. **ResourceMonitor** (`include/ResourceMonitor.h`, `src/ResourceMonitor.cpp`)
**Purpose:** Monitors application resource usage to prevent abuse

**Key Features:**
- CPU usage tracking from `/proc/[pid]/stat`
- Memory usage from `/proc/[pid]/status`
- Threshold-based alerts
- Self-resource awareness

**Key Methods:**
- `startMonitoring()` - Start resource tracking
- `stopMonitoring()` - Stop resource tracking
- `getCpuUsage()` - Get current CPU %
- `getMemoryPercentage()` - Get current memory %

---

### 9. **SessionManager** (`include/SessionManager.h`, `src/SessionManager.cpp`)
**Purpose:** Manages exam session state, timing, and history

**Key Features:**
- Session start/stop tracking
- Event aggregation per session
- Session history persistence
- Report generation

**Key Methods:**
- `startSession()` - Initialize new exam session
- `endSession()` - Finalize session
- `addEvent()` - Record event in session
- `getSummary()` - Get session statistics

---

### 10. **PrivilegeDialog** (`include/PrivilegeDialog.h`, `src/PrivilegeDialog.cpp`)
**Purpose:** Handles privilege elevation via pkexec

**Key Features:**
- Modal privilege request dialog
- pkexec-based elevation (no sudo password required)
- X11 display preservation for GUI
- Timeout handling
- User-friendly error messages

**Key Methods:**
- `hasRootPrivileges()` - Check if running as root
- `requestElevation()` - Show dialog and request elevation
- `authenticateWithPkexec()` - Execute elevation via pkexec
- `onElevateClicked()` - Handle button clicks

**Elevation Flow:**
1. Non-elevated app starts
2. Privilege dialog appears
3. User clicks "Request Privileges"
4. pkexec password dialog appears
5. After authentication, elevated app launches
6. Non-elevated instance exits

---

### 11. **SystemController** (`include/SystemController.h`, `src/SystemController.cpp`)
**Purpose:** Execute system commands and manage system integration

**Key Features:**
- Process execution wrapper
- Command timeout handling
- Output capture
- Error handling

**Key Methods:**
- `executeCommand()` - Run system command
- `executeCommandWithOutput()` - Run and capture output
- `killProcess()` - Terminate process

---

### 12. **Common.h** (`include/Common.h`)
**Purpose:** Shared definitions, enums, and constants

**Defines:**
- Version: `A3GUARD_VERSION = "1.0.0"`
- File extensions: `.a3log`, `.a3img`, `.a3bak`, `.a3int`
- Default paths: `/etc/a3guard/`, `/var/lib/a3guard/`, `/var/log/a3guard/`
- Monitoring intervals in milliseconds
- Resource limits (CPU, memory, log size)
- Event types, alert levels
- Structures: `MonitoringEvent`, `SessionSummary`, `ResourceUsage`
- Logging macros: `LOG_DEBUG`, `LOG_INFO`, `LOG_WARNING`, `LOG_ERROR`

---

## Building

### Prerequisites
```bash
# Debian/Ubuntu
sudo apt-get install -y \
    build-essential \
    cmake \
    qtbase5-dev qtbase5-dev-tools \
    libssl-dev \
    libudev-dev \
    libx11-dev libxfixes-dev \
    pkg-config
```

### Build Steps
```bash
# Navigate to project directory
cd /home/test/A3Guard

# Create build directory
mkdir -p build && cd build

# Configure (Debug or Release)
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
make -j$(nproc)

# Binary location
./A3Guard
```

### Build Types
- **Release** (default): Optimized with `-O3 -march=native`, minimal debug info
- **Debug**: Full symbols, no optimization, verbose logging
- **Profile**: Profile-guided optimization for benchmarking

---

## Installation

### System-wide Installation
```bash
cd /home/test/A3Guard/build

# Install (requires root)
sudo make install

# This installs:
# - Binary: /opt/a3guard/bin/A3Guard
# - Config: /etc/a3guard/a3guard.conf
# - Systemd service: /etc/systemd/system/a3guard.service
# - Desktop entry: /usr/share/applications/A3Guard.desktop
# - PolicyKit policy: /usr/share/polkit-1/actions/com.a3guard.policy
```

### Debian Package
```bash
cd build
cpack -G DEB
sudo dpkg -i a3guard-*.deb
```

### Directory Permissions
After installation, set proper permissions:
```bash
sudo mkdir -p /var/lib/a3guard/{screenshots,backup,integrity}
sudo mkdir -p /var/log/a3guard
sudo chown -R a3guard:a3guard /var/lib/a3guard /var/log/a3guard
sudo chmod 700 /var/lib/a3guard /var/log/a3guard
```

---

## Usage

### Running the Application

#### GUI Mode (Recommended)
```bash
# Without elevation (limited features)
./build/A3Guard

# Or with elevation prompt
./build/A3Guard
# → Privilege dialog appears
# → Click "Request Privileges"
# → Enter password when prompted
# → Elevated app launches
```

#### Command Line Mode
```bash
# Generate new encryption key
sudo ./build/A3Guard --generate-key

# Verify file integrity
sudo ./build/A3Guard --verify-integrity

# Enable verbose logging
sudo ./build/A3Guard --verbose

# Use custom config
./build/A3Guard --config /path/to/custom.conf

# View help
./build/A3Guard --help
```

### UI Navigation

1. **Dashboard Tab:** Monitoring status, airplane mode, violations, resource usage
2. **Clipboard Tab:** Recent clipboard entries with detection types
3. **Keylogger Tab:** Keystroke activity summary
4. **USB Tab:** Connected USB devices with model/serial info
5. **Logs Tab:** Event log with severity levels
6. **Statistics Tab:** Session summary, report generation

### Starting/Stopping Monitoring

1. Click "Start Monitoring" button in Dashboard
2. System begins collecting data
3. Click "Stop Monitoring" to pause
4. Generate reports from Statistics tab

---

## Security Features

### 1. Encryption
- **Algorithm:** AES-256-CBC with PKCS#7 padding
- **Key Management:** 256-bit keys in `/etc/a3guard/a3guard.key`
- **IV Generation:** Random 128-bit per operation
- **Applies to:** Logs, screenshots, backups

### 2. Integrity Verification
- **Algorithm:** SHA-256 hashing
- **Detection:** Real-time file monitoring with `QFileSystemWatcher`
- **Storage:** `.a3int` encrypted hash files
- **Response:** Immediate alert and logging

### 3. Privilege Enforcement
- **Model:** Minimal privilege by default, elevation on demand
- **Method:** pkexec (PolicyKit) for user-friendly auth
- **Requirements:** PolicyKit daemon, polkit rules
- **Timeout:** 120 seconds for password entry

### 4. Resource Limits
- **CPU:** Self-limits to 10% (monitored and logged)
- **Memory:** Self-limits to 100MB (monitored and logged)
- **Logs:** Auto-rotate at 10MB
- **Screenshots:** Automatic cleanup based on retention policy

### 5. Network Isolation
- **Airplane Mode:** Disables WiFi, Bluetooth, Ethernet simultaneously
- **Traffic Control:** iptables rules for granular blocking
- **State Enforcement:** Periodic verification that restrictions hold

---

## Development

### Code Organization
- **Headers in `include/`:** Interface definitions
- **Implementation in `src/`:** Core logic
- **Config in `config/`:** Default settings
- **Resources in `resources/`:** Qt resources (icons, etc)

### Adding New Features

1. **Create header in `include/`**
   ```cpp
   class MyNewFeature : public QObject {
       Q_OBJECT
   public slots:
       void onSomething();
   signals:
       void featureEvent(QString);
   };
   ```

2. **Implement in `src/MyNewFeature.cpp`**

3. **Connect to MainWindow:**
   ```cpp
   connect(m_myFeature.get(), &MyNewFeature::featureEvent,
           this, &MainWindow::onViolationDetected);
   ```

4. **Update `CMakeLists.txt`** to include new files

5. **Build and test:**
   ```bash
   cd build && make -j$(nproc)
   ```

### Testing

**No automated tests currently. Manual testing:**

```bash
# Test privilege elevation
./build/A3Guard

# Test USB detection
# → Plug in USB device → Check USB tab

# Test clipboard monitoring
# → Copy text → Check Clipboard tab

# Test keystroke detection
# → Type on keyboard → Check Keylogger tab

# Test network control
# → Check airplane mode toggle

# Test encryption
# → Check log files in /var/log/a3guard/
# → Verify .a3log files are binary (encrypted)
```

### Debugging

Enable debug logging:
```bash
./build/A3Guard --verbose 2>&1 | tee a3guard.log

# Also check system logs:
journalctl -u a3guard -f
```

### Code Style
- C++17 standard
- Qt naming conventions (m_ for members, camelCase for methods)
- Const correctness
- Smart pointers (shared_ptr, unique_ptr)

---

## Configuration

### Default Configuration File
Location: `/etc/a3guard/a3guard.conf` or `~/.config/a3guard/a3guard.conf`

Edit to customize:
- Monitoring intervals
- Resource limits
- Network restrictions
- Whitelisted applications/windows
- Alert settings

### Key Configuration Options

```ini
# Increase screenshot interval (milliseconds)
screenshot_interval=300000

# Enable localhost for local testing
allow_localhost=true

# Disable USB monitoring
check_usb_devices=false

# Set custom logging level
log_level=DEBUG
```

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| "Could not connect to X11 display" | Run with DISPLAY=:0, ensure X11 running |
| "Permission denied for USB unmount" | App must run as root (use pkexec) |
| "pkexec: command not found" | Install policykit package |
| "Blank UI after elevation" | Check X11 authority, run `xhost +local:` |
| "High CPU/memory usage" | Check resource limits in config |

---

## License

See LICENSE file for full license terms.

---

## Contributing

Contributions welcome! Please ensure:
- Code compiles without warnings
- Follows existing code style
- Includes appropriate logging
- Tests manual functionality
- Documents new features

---

## Contact & Support

For issues, questions, or contributions, please use the project's issue tracker or contact the development team.
