# A3Guard Update System - Complete Explanation

## Overview

The A3Guard update system uses a two-way architecture:

1. **Release Side (GitHub)**: Automatic build and packaging
2. **Client Side (App)**: Detect new versions and download updates

This document explains the complete flow from release to user update.

---

## Part 1: GitHub Release Pipeline

### 1.1 Release Trigger

When you create a release, the workflow is triggered by pushing a git tag:

```bash
# Current version in Common.h: 1.0.0
./scripts/update-version.sh 1.0.0 1.1.0  # Update all version numbers
git add .
git commit -m "Bump version: 1.0.0 → 1.1.0"
git tag -a v1.1.0 -m "Release v1.1.0"
git push origin main && git push origin v1.1.0  # TRIGGERS workflow
```

**File: `.github/workflows/release.yml`** (primary) or **`build-deb.yml`** (triggered on tag)

### 1.2 Build Process

When tag `v1.1.0` is pushed, GitHub Actions automatically:

```
┌─────────────────────────────────────────────────────────┐
│  GitHub Actions - Release Workflow Triggered            │
└─────────────────────────────────────────────────────────┘
                            ↓
         ┌──────────────────────────────────────┐
         │ JOB 1: validate-tag                  │
         │ ├─ Extract version from tag name     │
         │ │  (v1.1.0 → 1.1.0)                 │
         │ └─ Output: version=1.1.0             │
         └──────────────────────────────────────┘
                            ↓
         ┌──────────────────────────────────────┐
         │ JOB 2: build-release                 │
         │ ├─ Checkout code on Ubuntu 20.04     │
         │ ├─ Install dependencies              │
         │ │  (Qt5, OpenSSL, cmake, etc.)      │
         │ ├─ Run CMake with version 1.1.0      │
         │ ├─ Build optimized Release binary    │
         │ ├─ Create DEB package                │
         │ ├─ Verify DEB integrity              │
         │ ├─ Create GitHub Release             │
         │ └─ Attach DEB file to Release        │
         └──────────────────────────────────────┘
                            ↓
         ┌──────────────────────────────────────┐
         │ JOB 3: security-scan                 │
         │ ├─ Run Trivy vulnerability scanner   │
         │ ├─ Upload to GitHub Security         │
         │ └─ Non-blocking (continues if fails) │
         └──────────────────────────────────────┘
                            ↓
         ┌──────────────────────────────────────┐
         │ JOB 4: notify-success                │
         │ ├─ Print success message             │
         │ └─ Show release URL                  │
         └──────────────────────────────────────┘
```

### 1.3 GitHub Release Created

After build completes, GitHub Release contains:

**URL:** `https://github.com/iamthemag/A3Guard/releases/tag/v1.1.0`

**Assets:**
```
📦 a3guard_1.1.0_amd64.deb (≈50-100 MB)
   ├─ Binary: /opt/a3guard/bin/A3Guard
   ├─ Config: /etc/a3guard/a3guard.conf
   ├─ Service: /etc/systemd/system/a3guard.service
   └─ PolicyKit: /usr/share/polkit-1/actions/com.a3guard.policy
```

**Release Info Available:**
- Tag name: `v1.1.0`
- Release date
- DEB download URL
- Release notes
- Build information

---

## Part 2: GitHub API Integration

### 2.1 API Endpoint

The UpdateChecker will query GitHub API:

```
GET https://api.github.com/repos/iamthemag/A3Guard/releases/latest
```

### 2.2 API Response (JSON)

```json
{
  "url": "https://api.github.com/repos/iamthemag/A3Guard/releases/1234567",
  "tag_name": "v1.1.0",
  "name": "A3Guard v1.1.0",
  "draft": false,
  "prerelease": false,
  "created_at": "2024-10-22T10:00:00Z",
  "published_at": "2024-10-22T10:15:00Z",
  "body": "# A3Guard v1.1.0 Release\n\nBug fixes and improvements...",
  "assets": [
    {
      "name": "a3guard_1.1.0_amd64.deb",
      "size": 75000000,
      "download_count": 42,
      "created_at": "2024-10-22T10:15:00Z",
      "browser_download_url": "https://github.com/iamthemag/A3Guard/releases/download/v1.1.0/a3guard_1.1.0_amd64.deb"
    }
  ]
}
```

### 2.3 Key Data Extracted by UpdateChecker

```cpp
QString latestVersion = "1.1.0";        // From tag_name (remove 'v')
QString downloadUrl = "<browser_download_url>";  // Direct DEB link
QString releaseNotes = "<body>";        // Release description
```

---

## Part 3: Client-Side Update System

### 3.1 Architecture

```
A3Guard Application (Current Version: 1.0.0)
│
├─ MainWindow
│  ├─ Help Menu
│  │  └─ "Check for Updates" ← New menu item
│  │     └─ Triggers UpdateChecker::checkForUpdates()
│  │
│  └─ UpdateChecker (NEW COMPONENT)
│     ├─ Network Manager (QNetworkAccessManager)
│     ├─ Version Comparator
│     ├─ Download Manager
│     └─ Signals/Slots for UI feedback
│
└─ Configuration
   └─ Common.h: A3GUARD_VERSION = "1.0.0"
```

### 3.2 Update Check Flow

#### User Action
```
User clicks Help → "Check for Updates"
```

#### UpdateChecker Process

```cpp
UpdateChecker::checkForUpdates()
{
    // 1. Rate limiting (max once per hour)
    if (lastCheckTime < 1 hour ago) {
        emit checkFailed("Already checked recently");
        return;
    }
    
    // 2. Make HTTP request to GitHub API
    QUrl url("https://api.github.com/repos/iamthemag/A3Guard/releases/latest");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "A3Guard/1.0.0");
    m_networkManager->get(request);
}

// 3. Response received (callback)
void UpdateChecker::onGitHubResponseReceived()
{
    // Parse JSON response
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject obj = doc.object();
    
    // Extract version and download URL
    m_latestVersion = obj["tag_name"].toString().remove("v");  // "1.1.0"
    
    // Find DEB asset
    QJsonArray assets = obj["assets"].toArray();
    for (const auto& asset : assets) {
        if (asset["name"].toString().endsWith(".deb")) {
            m_downloadUrl = asset["browser_download_url"].toString();
            break;
        }
    }
    
    // 4. Version comparison
    if (isNewerVersion(getCurrentVersion(), m_latestVersion)) {
        // "1.1.0" > "1.0.0" ✓
        emit updateAvailable(m_latestVersion, m_downloadUrl);
    } else {
        emit noUpdateAvailable();
    }
}
```

### 3.3 Version Comparison Logic

```cpp
bool UpdateChecker::isNewerVersion(
    const QString &current,  // "1.0.0"
    const QString &latest)   // "1.1.0"
{
    // Parse semantic versions
    QStringList currParts = current.split('.');  // ["1", "0", "0"]
    QStringList latestParts = latest.split('.');  // ["1", "1", "0"]
    
    // Compare major.minor.patch
    for (int i = 0; i < 3; ++i) {
        int currNum = currParts[i].toInt();
        int latestNum = latestParts[i].toInt();
        
        if (latestNum > currNum) return true;    // 1 > 1? No, continue
                                                 // 1 > 0? Yes, return true ✓
        if (latestNum < currNum) return false;
    }
    return false;  // Same version
}

// Examples:
// isNewerVersion("1.0.0", "1.1.0") → true  ✓
// isNewerVersion("1.0.0", "2.0.0") → true  ✓
// isNewerVersion("1.0.0", "1.0.1") → true  ✓
// isNewerVersion("1.1.0", "1.0.0") → false ✗
// isNewerVersion("1.0.0", "1.0.0") → false ✗
```

---

## Part 4: UI Flow Examples

### 4.1 Scenario: Update Available

```
┌─────────────────────────────────────────────────────┐
│ MainWindow                                          │
│                                                     │
│ Help Menu                                           │
│ ├─ About                                            │
│ ├─ Settings                                         │
│ └─ Check for Updates ← User clicks              [1]│
└─────────────────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────────┐
│ Progress Dialog                                     │
│ "Checking for updates from GitHub..."         [2]  │
│ [Cancel]                                            │
└─────────────────────────────────────────────────────┘
                      ↓
          (Network request sent)
          (Waiting for response...)
                      ↓
┌─────────────────────────────────────────────────────┐
│ Update Available Dialog                        [3]  │
│                                                     │
│ ✓ New version available!                            │
│                                                     │
│ Current version: 1.0.0                              │
│ Latest version:  1.1.0                              │
│                                                     │
│ Release Notes:                                      │
│ ┌─────────────────────────────────────────────┐    │
│ │ • Bug fixes and improvements                │    │
│ │ • Performance optimization                  │    │
│ │ • Enhanced monitoring features              │    │
│ └─────────────────────────────────────────────┘    │
│                                                     │
│ [Download & Install] [Later]                       │
└─────────────────────────────────────────────────────┘
                      ↓
            User clicks "Download"
                      ↓
┌─────────────────────────────────────────────────────┐
│ Download Progress Dialog                       [4]  │
│                                                     │
│ Downloading a3guard_1.1.0_amd64.deb...              │
│ ████████████████░░░░░░░░░░░░░░  65%                │
│ 48.5 MB / 75 MB (2.3 MB/s)                          │
│ Time remaining: ~12 seconds                         │
│                                                     │
│ [Cancel]                                            │
└─────────────────────────────────────────────────────┘
                      ↓
          Download completes
                      ↓
┌─────────────────────────────────────────────────────┐
│ Installation Instructions Dialog               [5]  │
│                                                     │
│ ✓ Download Complete!                                │
│                                                     │
│ File: a3guard_1.1.0_amd64.deb                       │
│ Location: ~/.cache/a3guard/a3guard_1.1.0_amd64.deb │
│ Size: 75 MB                                         │
│                                                     │
│ To install, run:                                    │
│ ┌─────────────────────────────────────────────┐    │
│ │ sudo dpkg -i ~/.cache/a3guard/...deb    [📋]│    │
│ └─────────────────────────────────────────────┘    │
│                                                     │
│ [Open Installer] [Copy Command] [OK]               │
└─────────────────────────────────────────────────────┘
```

### 4.2 Scenario: Already Up to Date

```
Progress Dialog → GitHub API Response → Version Check
                                      (1.0.0 = 1.0.0)
                                            ↓
                      ┌────────────────────────────────┐
                      │ Up to Date Dialog              │
                      │                                │
                      │ ✓ Already up to date!          │
                      │                                │
                      │ You are running the latest     │
                      │ version (v1.0.0)               │
                      │                                │
                      │ [OK]                           │
                      └────────────────────────────────┘
```

### 4.3 Scenario: Network Error

```
Network Request → Connection Failed
                        ↓
          ┌──────────────────────────────┐
          │ Check Failed Dialog          │
          │                              │
          │ ✗ Unable to check for        │
          │   updates                    │
          │                              │
          │ Check your internet          │
          │ connection and try again.    │
          │                              │
          │ Error: Connection timeout    │
          │                              │
          │ [Retry] [Cancel]             │
          └──────────────────────────────┘
```

---

## Part 5: Implementation Details

### 5.1 UpdateChecker Class (include/UpdateChecker.h)

```cpp
#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QDateTime>

class UpdateChecker : public QObject {
    Q_OBJECT
    
public:
    explicit UpdateChecker(QObject *parent = nullptr);
    void checkForUpdates();
    QString getCurrentVersion() const;
    QString getLatestVersion() const;
    bool isUpdateAvailable() const;
    
signals:
    void updateAvailable(QString latestVersion, QString downloadUrl, QString releaseNotes);
    void noUpdateAvailable();
    void checkFailed(QString errorMessage);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(QString filePath);
    void checkStarted();
    
private slots:
    void onGitHubResponseReceived(QNetworkReply *reply);
    void onNetworkError(QNetworkReply::NetworkError error);
    void onDownloadFinished(QNetworkReply *reply);
    
private:
    void parseGitHubResponse(const QByteArray &data);
    bool isNewerVersion(const QString &current, const QString &latest);
    
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply;
    QString m_latestVersion;
    QString m_downloadUrl;
    QString m_releaseNotes;
    QDateTime m_lastCheckTime;
};

#endif
```

### 5.2 CMakeLists.txt Changes

**Add Qt5::Network dependency:**
```cmake
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Network)  # Added Network

# In add_executable section:
target_link_libraries(${PROJECT_NAME}
    ...existing...
    Qt5::Network  # NEW
)

# Add UpdateChecker to sources:
set(SOURCES
    ...existing...
    src/UpdateChecker.cpp  # NEW
)

set(HEADERS
    ...existing...
    include/UpdateChecker.h  # NEW
)
```

### 5.3 MainWindow Integration

**include/MainWindow.h:**
```cpp
private:
    UpdateChecker *m_updateChecker;  // NEW
    void checkForUpdates();           // NEW
    void onUpdateAvailable(QString version, QString url, QString notes);  // NEW
    void onUpdateCheckFailed(QString error);  // NEW
    void onUpdateCheckStarted();      // NEW
```

**src/MainWindow.cpp setupMenuBar():**
```cpp
QMenu *helpMenu = menuBar()->addMenu(tr("Help"));

// NEW: Add Check for Updates
QAction *checkUpdatesAction = helpMenu->addAction(tr("Check for Updates..."));
connect(checkUpdatesAction, &QAction::triggered, 
        this, &MainWindow::checkForUpdates);

helpMenu->addSeparator();
QAction *aboutAction = helpMenu->addAction(tr("About"));
// existing connections...
```

**src/MainWindow.cpp constructor:**
```cpp
m_updateChecker = new UpdateChecker(this);
connect(m_updateChecker, &UpdateChecker::checkStarted,
        this, &MainWindow::onUpdateCheckStarted);
connect(m_updateChecker, &UpdateChecker::updateAvailable,
        this, &MainWindow::onUpdateAvailable);
connect(m_updateChecker, &UpdateChecker::noUpdateAvailable,
        this, [this]() {
            QMessageBox::information(this, tr("Check for Updates"),
                tr("You are running the latest version (v%1)").arg(m_updateChecker->getCurrentVersion()));
        });
connect(m_updateChecker, &UpdateChecker::checkFailed,
        this, &MainWindow::onUpdateCheckFailed);
```

---

## Part 6: Data Flow Diagram

```
┌─────────────────────────────────────┐
│   Your Local Machine                │
│   A3Guard v1.0.0                    │
│   ┌──────────────────────────────┐  │
│   │ Help Menu                    │  │
│   │ "Check for Updates"          │  │
│   └──────────────────────────────┘  │
└─────────────────────────────────────┘
              ↓ (HTTPS)
┌─────────────────────────────────────┐
│   GitHub API                        │
│   /repos/iamthemag/A3Guard/...      │
│   /releases/latest                  │
└─────────────────────────────────────┘
              ↓ (JSON Response)
         Tag: v1.1.0
         Asset: a3guard_1.1.0_amd64.deb
         URL: github.com/.../releases/download/...
              ↓
┌─────────────────────────────────────┐
│   Local Machine                     │
│   Version Comparison:               │
│   1.0.0 < 1.1.0 → Update Available │
└─────────────────────────────────────┘
              ↓ (User confirms)
         Download URL invoked
              ↓
┌─────────────────────────────────────┐
│   GitHub Release Server             │
│   /releases/download/v1.1.0/        │
│   a3guard_1.1.0_amd64.deb           │
└─────────────────────────────────────┘
              ↓ (Binary stream)
┌─────────────────────────────────────┐
│   Local Machine                     │
│   ~/.cache/a3guard/                 │
│   a3guard_1.1.0_amd64.deb (75 MB)   │
└─────────────────────────────────────┘
              ↓ (User installs manually)
         sudo dpkg -i a3guard_*.deb
              ↓
         Installation complete
              ↓
         A3Guard v1.1.0 installed
```

---

## Part 7: Security & Reliability

### 7.1 Security Measures

✅ **HTTPS Only**
- All GitHub API requests use HTTPS
- SSL certificate verification enforced
- No plaintext data transmission

✅ **Authenticated Requests**
- User-Agent header identifies requests
- Rate limiting prevents abuse (1 check/hour)
- GitHub API public (no token needed)

✅ **File Integrity**
- DEB downloaded from official GitHub releases only
- Debian package manager validates contents
- Future: GPG signature verification

✅ **User Consent**
- Only checks on explicit user action
- No background auto-checking (default)
- User confirms before downloading
- User controls installation

### 7.2 Error Handling

```cpp
// Network errors caught:
- Connection timeout
- DNS resolution failure
- SSL certificate errors
- HTTP errors (404, 500, etc.)
- JSON parse errors
- Invalid version format

// User-friendly messages shown:
- "Unable to check for updates. Check internet connection."
- "Failed to retrieve update information."
- "Unexpected error. Try again later."
```

### 7.3 Reliability Features

- **Caching**: Store last check time (prevents API spam)
- **Retry Logic**: User can click "Retry" on failure
- **Graceful Degradation**: App continues even if update check fails
- **Timeout Protection**: Network request times out after 30 seconds

---

## Part 8: Complete User Journey

### Release Manager Creates Release

```bash
# 1. Update version
./scripts/update-version.sh 1.0.0 1.1.0

# 2. Review changes
git diff

# 3. Commit
git add .
git commit -m "Bump version: 1.0.0 → 1.1.0"

# 4. Tag
git tag -a v1.1.0 -m "Release v1.1.0"

# 5. Push (TRIGGERS workflow)
git push origin main
git push origin v1.1.0
```

### GitHub Workflow Runs

```
[validate-tag] → Extract version (1.1.0) ✓
[build-release] → Build DEB, create release ✓
[security-scan] → Run Trivy ✓
[notify-success] → Print success ✓
```

### Release Available

GitHub Release: https://github.com/iamthemag/A3Guard/releases/tag/v1.1.0
DEB Available: `a3guard_1.1.0_amd64.deb`

### User Workflow

```
1. User clicks: Help → "Check for Updates"
2. Progress dialog shows
3. UpdateChecker queries GitHub API
4. Gets: v1.1.0 available
5. Version comparison: 1.1.0 > 1.0.0
6. Dialog: "Update Available" appears
7. User clicks: "Download"
8. DEB downloads: ~/.cache/a3guard/a3guard_1.1.0_amd64.deb
9. Instructions shown
10. User installs: sudo dpkg -i ~/.cache/a3guard/a3guard_1.1.0_amd64.deb
11. A3Guard updated to v1.1.0
```

---

## Part 9: Testing the System

### 9.1 Manual Testing

```bash
# Build with current version
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Run app
sudo ./A3Guard  # or pkexec ./A3Guard

# Click: Help → Check for Updates
# Should query GitHub API for releases/latest
# Show current version vs latest
```

### 9.2 Mock Testing

To test without real GitHub:

```cpp
// In UpdateChecker_test.cpp
void TestUpdateChecker::testVersionComparison() {
    UpdateChecker checker;
    
    ASSERT_TRUE(checker.isNewerVersion("1.0.0", "1.1.0"));
    ASSERT_TRUE(checker.isNewerVersion("1.0.0", "2.0.0"));
    ASSERT_TRUE(checker.isNewerVersion("1.0.5", "1.1.0"));
    ASSERT_FALSE(checker.isNewerVersion("1.1.0", "1.0.0"));
    ASSERT_FALSE(checker.isNewerVersion("1.0.0", "1.0.0"));
}

void TestUpdateChecker::testJSONParsing() {
    // Mock GitHub response
    QString mockResponse = R"({
        "tag_name": "v1.1.0",
        "assets": [{
            "name": "a3guard_1.1.0_amd64.deb",
            "browser_download_url": "https://..."
        }]
    })";
    
    checker.parseGitHubResponse(mockResponse.toUtf8());
    ASSERT_EQ(checker.getLatestVersion(), "1.1.0");
}
```

---

## Part 10: Future Enhancements

```
Phase 1: ✅ Basic update checking (Current Design)
├─ Check GitHub releases API
├─ Version comparison
├─ Download DEB
└─ Installation instructions

Phase 2: 🔄 Auto-checking (Optional)
├─ Background update checks on startup
├─ Configurable check intervals
├─ Notification system
└─ Tray notifications

Phase 3: 🔄 Advanced features
├─ In-app DEB installation (pkexec)
├─ Changelog display
├─ Update history
├─ Rollback support
└─ Beta version opt-in

Phase 4: 🔄 Security enhancements
├─ GPG signature verification
├─ Checksum validation
├─ Binary fingerprint checking
└─ Signed releases
```

---

## Summary

**The update system works through:**

1. **Release**: Tag pushed → GitHub Actions builds DEB → Release created
2. **Discovery**: User clicks "Check for Updates" → API queries GitHub
3. **Comparison**: Local version (1.0.0) vs GitHub version (1.1.0)
4. **Download**: If newer, user downloads DEB from GitHub
5. **Installation**: User manually installs via dpkg
6. **Complete**: App updated to latest version

This approach is:
- ✅ Simple and reliable
- ✅ Secure (HTTPS only)
- ✅ User-controlled (explicit consent)
- ✅ Professional (clean UI)
- ✅ Maintainable (minimal code)
- ✅ Scalable (future enhancements possible)
