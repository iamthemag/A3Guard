<h1 align="center">
  <br>
  <a href="http://github.com/iamthemag/A3Guard">
    <img src="https://raw.githubusercontent.com/iamthemag/A3Guard/main/A3Guard-logo.jpg" alt="A3Guard" width="250">
  </a>
  <br>
</h1>

<p align="center">
  <img src="https://visitor-badge.laobi.icu/badge?page_id=iamthemag.A3Guard" alt="Visitors" />
  <img src="https://img.shields.io/github/stars/iamthemag/A3Guard" alt="GitHub Stars" />
  <img src="https://img.shields.io/github/forks/iamthemag/A3Guard" alt="GitHub Forks" />
  <img src="https://img.shields.io/github/issues/iamthemag/A3Guard" alt="Issues" />
  <img src="https://img.shields.io/badge/License-GPLv3-blue.svg" alt="GNU GPLv3" />
  <img src="https://img.shields.io/github/v/release/iamthemag/A3Guard" alt="Latest Release" />
  <img src="https://img.shields.io/github/contributors/iamthemag/A3Guard" alt="Contributors" />
</p>

<p align="center">
  <a href="#about-a3guard">About A3Guard</a> •
  <a href="#key-features">Key Features</a> •
  <a href="#installation">Installation</a> •
  <a href="#usage">Usage</a> •
  <a href="#security-features">Security</a> •
  <a href="#contributing">Contributing</a> •
  <a href="#license">License</a>
</p>

## About A3Guard

A3Guard is a comprehensive C++/Qt5-based desktop application for advanced assessment and monitoring for academic environments. It provides robust security features, real-time monitoring, and minimal resource usage.

## Features

### Core Monitoring
- **Application Tracking**: Monitor launched/closed applications
- **Window Focus Tracking**: Track active window changes  
- **Clipboard Monitoring**: Detect copy/paste activities
- **Screenshot Capture**: Periodic encrypted screenshots
- **USB Monitoring**: Detect device insertions/removals
- **Network Control**: Granular network interface management

### Security
- **AES-256 Encryption**: All logs and screenshots encrypted
- **File Integrity Checking**: Detect tampering with SHA-256 hashes
- **Secure Deletion**: Multi-pass file overwriting
- **Administrator Privileges**: Root-level system access required
- **Real-time Alerts**: Visual and audio violation notifications

### Performance
- **Minimal Resource Usage**: <10% CPU, <100MB RAM
- **Optimized Monitoring**: Efficient polling intervals
- **Resource Monitoring**: Self-monitoring with limits
- **Custom File Extensions**: .a3log, .a3img, .a3bak, .a3int

## Requirements

### System Requirements
- Ubuntu 20.04 or newer
- Qt5 development libraries
- OpenSSL
- X11 with XFixes
- Root/sudo access

### Build Dependencies
```bash
sudo apt-get install -y \
    build-essential \
    cmake \
    qtbase5-dev \
    qtbase5-dev-tools \
    libssl-dev \
    libudev-dev \
    libx11-dev \
    libxfixes-dev \
    pkg-config
```

### Runtime Dependencies  
```bash
sudo apt-get install -y \
    qt5-default \
    libqt5widgets5 \
    libqt5core5a \
    libqt5gui5 \
    libqt5network5 \
    libssl1.1 \
    libudev1 \
    libx11-6 \
    libxfixes3 \
    gksu
```

## Building

### Quick Build
```bash
# Clone or extract source
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

### Detailed Build Process
```bash
# Install build dependencies
sudo apt-get update
sudo apt-get install -y build-essential cmake qtbase5-dev qtbase5-dev-tools \
    libssl-dev libudev-dev libx11-dev libxfixes-dev pkg-config

# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build (use all CPU cores)
make -j$(nproc)

# Optional: Build Debian package
cpack -G DEB
```

### Build Options
- `-DCMAKE_BUILD_TYPE=Release`: Optimized release build (default)
- `-DCMAKE_BUILD_TYPE=Debug`: Debug build with symbols

## Installation

### Automated Installation
```bash
# Build first
mkdir -p build && cd build
cmake .. && make

# Install (requires root)
cd ..
sudo ./scripts/install.sh
```

### Manual Installation
```bash
# After building
sudo mkdir -p /opt/a3guard/bin
sudo cp build/A3Guard /opt/a3guard/bin/
sudo cp config/a3guard.conf /etc/a3guard/
sudo cp scripts/install.sh /opt/a3guard/scripts/

# Run installation script
sudo ./scripts/install.sh
```

### Package Installation
```bash
# Build package
cd build
cpack -G DEB

# Install package
sudo dpkg -i a3guard-*.deb
sudo apt-get install -f  # Fix dependencies if needed
```

## Usage

### GUI Mode
```bash
# Launch with GUI (requires root)
sudo a3guard

# Or use desktop launcher (requires authentication)
a3guard
```

### Service Mode
```bash
# Start service
sudo systemctl start a3guard

# Enable auto-start
sudo systemctl enable a3guard

# Check status
sudo systemctl status a3guard

# View logs
sudo journalctl -u a3guard -f
```

### Command Line Options
```bash
A3Guard [options]

Options:
  --daemon          Run as daemon (no GUI)
  --generate-key    Generate new encryption key
  --verify-integrity Verify file integrity
  --config <file>   Use custom config file
  --help           Show help
  --version        Show version
```

## Configuration

Configuration file: `/etc/a3guard/a3guard.conf`

### Key Settings
```ini
[monitoring]
screenshot_interval=120        # Screenshot every 2 minutes
network_check_interval=30      # Check network every 30 seconds
app_monitor_interval=5         # Check apps every 5 seconds

[security]
enable_integrity_check=true    # Enable file integrity checking
encryption_key_file=/etc/a3guard/a3guard.key

[resources]
max_cpu_usage=10              # Maximum 10% CPU usage
max_memory_usage=100          # Maximum 100MB RAM

[network]
disable_interfaces=wifi,bluetooth,ethernet
block_all_traffic=true
```

## Security Features

### Encryption
- **Algorithm**: AES-256-CBC
- **Key Storage**: Secure file with 600 permissions
- **Data Protection**: All logs and screenshots encrypted

### Integrity Checking
- **Hash Algorithm**: SHA-256
- **Real-time Monitoring**: File system watcher
- **Periodic Verification**: Configurable intervals
- **Violation Detection**: Automatic alerts

### Access Control
- **Root Privileges**: Required for system-level monitoring
- **AppArmor Profile**: Optional additional security layer
- **Secure Permissions**: Restricted file access
- **Process Protection**: Systemd service hardening

## File Structure

### Installation Paths
```
/opt/a3guard/          # Application directory
├── bin/
│   └── A3Guard        # Main executable
└── scripts/
    ├── install.sh       # Installation script
    ├── uninstall.sh     # Removal script
    └── build.sh         # Build script

/etc/a3guard/          # Configuration directory
├── a3guard.conf       # Main configuration
└── a3guard.key        # Encryption key (auto-generated)

/var/log/a3guard/      # Log directory
└── *.a3log              # Encrypted log files

/var/lib/a3guard/      # Data directory
├── screenshots/
│   └── *.a3img          # Encrypted screenshots
├── backup/
│   └── *.a3bak          # Backup files
└── integrity/
    └── *.a3int          # Integrity check files
```

### Custom Extensions
- `.a3log` - Encrypted log files
- `.a3img` - Encrypted screenshot files  
- `.a3bak` - Encrypted backup files
- `.a3int` - Encrypted integrity check files

## Monitoring Capabilities

### Application Monitoring
- Process launch/termination detection
- Window title and class tracking
- Active window focus changes
- Resource usage per application

### System Monitoring
- USB device insertion/removal
- Network interface state changes
- Clipboard content changes (hashed)
- System resource usage

### Network Control
- Wi-Fi interface control
- Bluetooth interface control
- Ethernet interface control
- Traffic blocking with iptables
- NetworkManager integration

### Screenshot System
- Encrypted storage
- Configurable intervals
- Thumbnail generation
- Automatic cleanup
- Integrity verification

## Troubleshooting

### Common Issues

**Permission denied errors:**
```bash
# Ensure proper permissions
sudo chown -R root:a3guard /var/log/a3guard /var/lib/a3guard
sudo chmod 700 /var/log/a3guard /var/lib/a3guard
```

**Build failures:**
```bash
# Install all dependencies
sudo apt-get build-dep qt5-default
sudo apt-get install qtbase5-private-dev
```

**Service won't start:**
```bash
# Check systemd logs
sudo journalctl -u a3guard -n 50

# Verify binary permissions
sudo chmod +x /opt/a3guard/bin/A3Guard
```

**GUI won't launch:**
```bash
# Check X11 permissions
sudo xhost +SI:localuser:root

# Use pkexec instead of gksu
pkexec /opt/a3guard/bin/A3Guard
```

### Debug Mode
```bash
# Build debug version
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Run with debug output
sudo ./A3Guard --verbose
```

### Log Analysis
```bash
# View decrypted logs (requires root)
sudo a3guard --decrypt-logs

# Check integrity
sudo a3guard --verify-integrity

# Resource usage
sudo a3guard --resource-report
```

## Uninstallation

```bash
# Using uninstall script
sudo /opt/a3guard/scripts/uninstall.sh

# Manual removal
sudo systemctl stop a3guard
sudo systemctl disable a3guard
sudo rm -rf /opt/a3guard
sudo rm -rf /etc/a3guard
sudo rm -rf /var/log/a3guard
sudo rm -rf /var/lib/a3guard
sudo rm -f /etc/systemd/system/a3guard.service
sudo rm -f /usr/share/applications/a3guard.desktop
sudo rm -f /usr/local/bin/a3guard
sudo userdel a3guard
```

## Development

### Code Structure
- `src/`: Source files (.cpp)
- `include/`: Header files (.h)
- `config/`: Configuration files
- `scripts/`: Installation/build scripts
- `resources/`: Qt resources (icons, etc.)

### Key Classes
- `SecurityManager`: Encryption and integrity
- `MonitoringEngine`: Core monitoring logic
- `NetworkManager`: Network control
- `ConfigManager`: Configuration handling
- `MainWindow`: GUI interface

### Building from Source
```bash
# Development build with debug symbols
mkdir debug-build && cd debug-build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Profile-guided optimization build
mkdir pgo-build && cd pgo-build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fprofile-generate" ..
make && ./A3Guard --benchmark
cmake -DCMAKE_CXX_FLAGS="-fprofile-use" ..
make
```

## License

This project is licensed under a proprietary license for educational use. See LICENSE file for details.

## Support

For support and bug reports:
- Check logs: `/var/log/a3guard/`
- Verify integrity: `sudo a3guard --verify-integrity`
- Review configuration: `/etc/a3guard/a3guard.conf`

## Security Considerations

⚠️ **Important Security Notes:**
- Always run integrity checks after system updates
- Regularly backup encryption keys securely
- Monitor log files for unauthorized access attempts
- Use AppArmor/SELinux for additional security layers
- Keep the system updated with latest security patches

## Performance Tuning

For optimal performance on exam systems:
- Adjust monitoring intervals based on exam duration
- Limit screenshot frequency for longer exams
- Configure resource limits appropriately
- Use SSD storage for better I/O performance
- Disable unnecessary system services during exams