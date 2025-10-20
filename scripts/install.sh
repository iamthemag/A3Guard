#!/bin/bash

# A3Guard Installation Script for C++/Qt5 Version
# Requires root privileges

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    log_error "This script must be run as root (use sudo)"
    exit 1
fi

log_info "Installing A3Guard C++/Qt5 Version..."

# Define paths
INSTALL_DIR="/opt/a3guard"
SERVICE_DIR="/etc/systemd/system"
CONFIG_DIR="/etc/a3guard"
LOG_DIR="/var/log/a3guard"
DATA_DIR="/var/lib/a3guard"
BUILD_DIR="$(dirname "$0")/../build"

# Check if build exists
if [ ! -f "$BUILD_DIR/A3Guard" ]; then
    log_error "A3Guard binary not found. Please build the project first:"
    log_error "  mkdir -p build && cd build"
    log_error "  cmake .. && make"
    exit 1
fi

# Install system dependencies
log_info "Installing system dependencies..."
apt-get update
apt-get install -y \
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

# Create system user
if ! id "a3guard" &>/dev/null; then
    log_info "Creating a3guard system user..."
    useradd -r -s /bin/false -d "$DATA_DIR" a3guard
fi

# Create directories
log_info "Creating directories..."
mkdir -p "$INSTALL_DIR/bin"
mkdir -p "$INSTALL_DIR/scripts"
mkdir -p "$CONFIG_DIR"
mkdir -p "$LOG_DIR"
mkdir -p "$DATA_DIR/screenshots"
mkdir -p "$DATA_DIR/backup"
mkdir -p "$DATA_DIR/integrity"

# Copy application files
log_info "Installing application files..."
cp "$BUILD_DIR/A3Guard" "$INSTALL_DIR/bin/"
cp "$(dirname "$0")/../config/a3guard.conf" "$CONFIG_DIR/"
cp "$(dirname "$0")/uninstall.sh" "$INSTALL_DIR/scripts/"

# Set permissions
log_info "Setting permissions..."
chown -R root:root "$INSTALL_DIR"
chmod 755 "$INSTALL_DIR/bin/A3Guard"
chmod 755 "$INSTALL_DIR/scripts/uninstall.sh"

# Set data directory permissions
chown -R a3guard:a3guard "$LOG_DIR" "$DATA_DIR"
chmod 700 "$LOG_DIR" "$DATA_DIR"
chmod 755 "$CONFIG_DIR"
chmod 644 "$CONFIG_DIR/a3guard.conf"

# Create systemd service
log_info "Creating systemd service..."
cat > "$SERVICE_DIR/a3guard.service" << 'EOF'
[Unit]
Description=A3Guard Monitoring Service
After=network.target graphical-session.target
Wants=graphical-session.target

[Service]
Type=simple
User=root
Environment=DISPLAY=:0
WorkingDirectory=/opt/a3guard/bin
ExecStart=/opt/a3guard/bin/A3Guard --daemon
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal
TimeoutStopSec=30

# Security settings
NoNewPrivileges=false
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/log/a3guard /var/lib/a3guard /etc/a3guard
PrivateTmp=true

[Install]
WantedBy=multi-user.target
EOF

# Create desktop entry
log_info "Creating desktop entry..."
cat > "/usr/share/applications/a3guard.desktop" << EOF
[Desktop Entry]
Name=A3Guard
Comment=Advanced Assessment Application
Exec=gksu /opt/a3guard/bin/A3Guard
Icon=/opt/a3guard/bin/a3guard.png
Terminal=false
Type=Application
Categories=Education;Security;
StartupNotify=true
EOF

# Create policy for authentication
log_info "Creating authentication policy..."
cat > "/usr/share/polkit-1/actions/com.a3guard.policy" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE policyconfig PUBLIC
 "-//freedesktop//DTD PolicyKit Policy Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/PolicyKit/1.0/policyconfig.dtd">
<policyconfig>
  <action id="com.a3guard.run">
    <description>Run A3Guard</description>
    <message>Authentication is required to run A3Guard</message>
    <defaults>
      <allow_any>auth_admin</allow_any>
      <allow_inactive>auth_admin</allow_inactive>
      <allow_active>auth_admin</allow_active>
    </defaults>
    <annotate key="org.freedesktop.policykit.exec.path">/opt/a3guard/bin/A3Guard</annotate>
    <annotate key="org.freedesktop.policykit.exec.allow_gui">true</annotate>
  </action>
</policyconfig>
EOF

# Create launcher script
log_info "Creating launcher script..."
cat > "/usr/local/bin/a3guard" << 'EOF'
#!/bin/bash
pkexec /opt/a3guard/bin/A3Guard "$@"
EOF
chmod +x "/usr/local/bin/a3guard"

# Reload systemd
systemctl daemon-reload

# Create build script for users
cat > "$INSTALL_DIR/scripts/build.sh" << 'EOF'
#!/bin/bash
# Build script for A3Guard

if [ "$EUID" -ne 0 ]; then
    echo "Error: This script must be run as root (use sudo)"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

echo "Building A3Guard..."

# Install build dependencies
apt-get update
apt-get install -y \
    build-essential \
    cmake \
    qtbase5-dev \
    qtbase5-dev-tools \
    libssl-dev \
    libudev-dev \
    libx11-dev \
    libxfixes-dev \
    pkg-config

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure and build
cmake ..
make -j$(nproc)

echo "Build completed successfully!"
echo "Binary location: $BUILD_DIR/A3Guard"
EOF

chmod +x "$INSTALL_DIR/scripts/build.sh"

# Create AppArmor profile (optional security layer)
log_info "Creating AppArmor profile..."
cat > "/etc/apparmor.d/opt.a3guard.bin.A3Guard" << 'EOF'
#include <tunables/global>

/opt/a3guard/bin/A3Guard {
  #include <abstractions/base>
  #include <abstractions/X>
  #include <abstractions/qt5>

  capability dac_override,
  capability setgid,
  capability setuid,
  capability sys_admin,
  capability net_admin,

  /opt/a3guard/bin/A3Guard mr,
  /etc/a3guard/** r,
  /var/log/a3guard/** rw,
  /var/lib/a3guard/** rw,
  
  /proc/sys/net/** rw,
  /sys/class/net/** r,
  /dev/udev r,
  
  # Allow screenshot functionality
  /dev/fb* r,
  /tmp/.X11-unix/* rw,
  
  # Network control
  /usr/bin/nmcli Px,
  /sbin/ip Px,
  /sbin/iptables Px,
  
  deny network,
  deny /home/** w,
  deny /root/** w,
}
EOF

# Create fail2ban configuration (optional)
if command -v fail2ban-client &> /dev/null; then
    log_info "Creating fail2ban configuration..."
    cat > "/etc/fail2ban/filter.d/a3guard.conf" << 'EOF'
[Definition]
failregex = ^.*\[ERROR\].*integrity_violation.*<HOST>.*$
            ^.*\[ERROR\].*network_violation.*<HOST>.*$
            ^.*\[ERROR\].*unauthorized_access.*<HOST>.*$
ignoreregex =
EOF

    cat > "/etc/fail2ban/jail.d/a3guard.conf" << 'EOF'
[a3guard]
enabled = true
port = all
filter = a3guard
logpath = /var/log/a3guard/*.eglog
maxretry = 3
bantime = 3600
EOF
fi

# Final setup
log_info "Performing final setup..."

# Generate initial key
"$INSTALL_DIR/bin/A3Guard" --generate-key 2>/dev/null || true

# Set final permissions
chown root:a3guard "$CONFIG_DIR/a3guard.key" 2>/dev/null || true
chmod 640 "$CONFIG_DIR/a3guard.key" 2>/dev/null || true

log_info "A3Guard installed successfully!"
echo
log_info "Usage:"
log_info "  GUI: Run 'a3guard' command or use desktop launcher"
log_info "  Service: sudo systemctl start a3guard"
log_info "  Build from source: sudo $INSTALL_DIR/scripts/build.sh"
log_info "  Uninstall: sudo $INSTALL_DIR/scripts/uninstall.sh"
echo
log_warn "Important: A3Guard requires root privileges to function properly."
log_warn "Always run integrity checks after system updates."

# Optional: Enable service
read -p "Enable A3Guard service to start on boot? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    systemctl enable a3guard.service
    log_info "A3Guard service enabled for automatic startup"
fi

log_info "Installation completed successfully!"