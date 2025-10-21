# A3Guard - Check for Updates Feature Design

## Executive Summary

This document outlines the implementation plan for a "Check for Updates" feature in the A3Guard application Help menu. The feature will automatically detect new versions from GitHub, notify users, and facilitate downloading the latest DEB package.

## 1. Architecture Overview

### 1.1 Component Structure

```
UpdateChecker (NEW)
├── Async Version Check (QNetworkAccessManager)
├── GitHub API Integration
├── Version Comparison Logic
├── Download Manager
└── UI Integration (MainWindow)
```

### 1.2 Files to be Created/Modified

**New Files:**
- `include/UpdateChecker.h` - Header for update checking component
- `src/UpdateChecker.cpp` - Implementation of update checking logic

**Modified Files:**
- `include/MainWindow.h` - Add UpdateChecker member and update slot
- `src/MainWindow.cpp` - Add "Check for Updates" menu item and handlers
- `CMakeLists.txt` - Add UpdateChecker sources, QNetwork module

## 2. Detailed Implementation Plan

### 2.1 UpdateChecker Class

**Header Location:** `include/UpdateChecker.h`

**Features:**
- Inherits from `QObject` for signal/slot support
- Uses `QNetworkAccessManager` for HTTP requests
- Non-blocking asynchronous operation
- Caches last check time to avoid spam

**Public Methods:**
```cpp
class UpdateChecker : public QObject {
    Q_OBJECT
    
public:
    explicit UpdateChecker(QObject *parent = nullptr);
    void checkForUpdates();
    QString getCurrentVersion() const;
    QString getLatestVersion() const;
    bool isUpdateAvailable() const;
    
signals:
    void updateAvailable(QString latestVersion, QString downloadUrl);
    void noUpdateAvailable();
    void checkFailed(QString errorMessage);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(QString filePath);
    
private slots:
    void onGitHubResponseReceived();
    void onNetworkError(QNetworkReply::NetworkError error);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    
private:
    QNetworkAccessManager *m_networkManager;
    QString m_latestVersion;
    QString m_downloadUrl;
    QDateTime m_lastCheckTime;
    
    void parseGitHubResponse(const QByteArray &data);
    bool isNewerVersion(const QString &current, const QString &latest);
    QString getDownloadUrl(const QString &version);
};
```

### 2.2 GitHub API Integration

**Approach:**
- Use GitHub REST API v3 (no authentication required for public repos)
- Endpoint: `https://api.github.com/repos/yourorg/a3guard/releases/latest`
- Parse JSON response to extract version and download URL

**Data Flow:**
1. Make HTTP GET request to GitHub API
2. Parse JSON response
3. Extract version number from tag_name field
4. Extract DEB file download URL from assets
5. Compare with current version
6. Emit appropriate signal

**JSON Response Example:**
```json
{
  "tag_name": "v1.1.0",
  "assets": [
    {
      "name": "a3guard_1.1.0_amd64.deb",
      "download_count": 5,
      "browser_download_url": "https://github.com/..."
    }
  ]
}
```

### 2.3 Version Comparison Logic

**Semantic Version Comparison:**
```cpp
bool UpdateChecker::isNewerVersion(
    const QString &current,  // e.g., "1.0.0"
    const QString &latest)   // e.g., "1.1.0"
{
    // Parse versions: "1.0.0" -> [1, 0, 0]
    QStringList currParts = current.split('.');
    QStringList latestParts = latest.split('.');
    
    // Compare major.minor.patch
    for (int i = 0; i < 3; ++i) {
        int currNum = currParts[i].toInt();
        int latestNum = latestParts[i].toInt();
        
        if (latestNum > currNum) return true;
        if (latestNum < currNum) return false;
    }
    return false; // Same version
}
```

### 2.4 User Interface Integration

**Location:** Help Menu

**UI Flow:**
```
Help Menu
├── About A3Guard (existing)
├── Settings (existing)
└── Check for Updates (NEW)
    ↓
    Show Progress Dialog (Checking...)
    ↓
    Compare Versions
    ↓
    If Update Available:
       ↓ Show Update Dialog
       ├─ Current: v1.0.0
       ├─ Latest: v1.1.0
       ├─ Changes: [Release notes from GitHub]
       ├─ [Download & Install]
       └─ [Later]
    
    If No Update:
       ↓ Show "Already up to date" message
```

**UI Components:**
- Progress dialog with "Checking for updates..."
- Update available dialog with:
  - Current version
  - Latest version
  - Release notes/changelog
  - Download button
  - Later/Cancel button
- Status bar notification for download progress

### 2.5 Download Management

**Implementation:**
- Download DEB file to `~/.cache/a3guard/` or `/tmp/`
- Show progress in main UI or separate dialog
- After download completes:
  - Notify user
  - Optionally launch installer (`pkexec dpkg -i filename.deb`)
  - Or provide direct link to install instructions

**Download Flow:**
```
File → Network Request → Progress Updates → Local Disk
        ↓                 ↓
        |←─ emitProgress ─|
        
After complete:
        ↓
Show notification with install options
```

## 3. Integration Points

### 3.1 MainWindow Integration

**Changes Required:**

1. **Header Addition** (include/MainWindow.h):
```cpp
private:
    UpdateChecker *m_updateChecker;
    void checkForUpdates();  // Slot
    void onUpdateAvailable(QString version, QString url);
    void onUpdateCheckFailed(QString error);
```

2. **Menu Bar Addition** (src/MainWindow.cpp setupMenuBar):
```cpp
// In Help menu
QAction *checkUpdatesAction = helpMenu->addAction("Check for Updates...");
connect(checkUpdatesAction, &QAction::triggered, 
        this, &MainWindow::checkForUpdates);
```

3. **Signal-Slot Connections** (src/MainWindow.cpp constructor):
```cpp
m_updateChecker = new UpdateChecker(this);
connect(m_updateChecker, &UpdateChecker::updateAvailable,
        this, &MainWindow::onUpdateAvailable);
connect(m_updateChecker, &UpdateChecker::checkFailed,
        this, &MainWindow::onUpdateCheckFailed);
```

### 3.2 CMakeLists.txt Update

**Add to dependencies:**
```cmake
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Network)

# In target_link_libraries:
Qt5::Network
```

**Add source files:**
```cmake
set(SOURCES
    ...existing files...
    src/UpdateChecker.cpp
)

set(HEADERS
    ...existing files...
    include/UpdateChecker.h
)
```

## 4. Implementation Details

### 4.1 UpdateChecker Implementation

**Key Methods:**

```cpp
UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &UpdateChecker::onGitHubResponseReceived);
    m_lastCheckTime = QDateTime::currentDateTime().addSecs(-3600);
}

void UpdateChecker::checkForUpdates()
{
    // Prevent spam checking (max once per hour)
    if (m_lastCheckTime.secsTo(QDateTime::currentDateTime()) < 3600) {
        emit checkFailed("Already checked recently. Try again later.");
        return;
    }
    
    QUrl url("https://api.github.com/repos/yourorg/a3guard/releases/latest");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "A3Guard/1.0.0");
    
    m_networkManager->get(request);
    m_lastCheckTime = QDateTime::currentDateTime();
}

void UpdateChecker::parseGitHubResponse(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    
    m_latestVersion = obj["tag_name"].toString().remove("v");
    
    QJsonArray assets = obj["assets"].toArray();
    for (const QJsonValue &asset : assets) {
        QString name = asset.toObject()["name"].toString();
        if (name.endsWith(".deb")) {
            m_downloadUrl = asset.toObject()["browser_download_url"].toString();
            break;
        }
    }
    
    if (isNewerVersion(getCurrentVersion(), m_latestVersion)) {
        emit updateAvailable(m_latestVersion, m_downloadUrl);
    } else {
        emit noUpdateAvailable();
    }
}
```

### 4.2 Error Handling

**Network Errors:**
- Connection failed → "Unable to check for updates. Check internet connection."
- Invalid response → "Failed to retrieve update information."
- Parse error → "Update check failed. Try again later."

**User Messages:**
- "Already up to date" - Current version is latest
- "New version available: v1.1.0" - Update available
- "Check your internet connection" - Network error

### 4.3 Caching Strategy

**Prevent Excessive Requests:**
- Store last check time in memory
- Minimum interval: 1 hour between checks
- Cache latest version info until next check

**File-Based Caching (Optional):**
```cpp
~/.config/a3guard/update_cache.json
{
    "lastCheck": "2024-10-21T19:38:00",
    "latestVersion": "1.1.0",
    "downloadUrl": "https://..."
}
```

## 5. User Experience Flow

### 5.1 Happy Path (Update Available)

```
User clicks "Help" → "Check for Updates"
    ↓
Progress dialog shows "Checking for updates..."
    ↓
API returns latest version (v1.1.0)
    ↓
Version comparison: 1.1.0 > 1.0.0 ✓
    ↓
Dialog shows:
  "Update Available"
  "Current: v1.0.0"
  "Latest: v1.1.0"
  "Changes: Bug fixes and improvements..."
  [Download] [Later]
    ↓
User clicks "Download"
    ↓
Download progress shown in UI
    ↓
After download:
  "Downloaded: a3guard_1.1.0_amd64.deb"
  "Install with: sudo dpkg -i ~/Downloads/a3guard_1.1.0_amd64.deb"
  [Open Installer] [Copy Command] [OK]
```

### 5.2 No Update Path

```
User clicks "Help" → "Check for Updates"
    ↓
Checking...
    ↓
Version matches: 1.0.0 = 1.0.0
    ↓
Dialog: "✓ Already up to date"
         "You are running the latest version (v1.0.0)"
         [OK]
```

### 5.3 Error Path

```
User clicks "Help" → "Check for Updates"
    ↓
Network request fails
    ↓
Dialog: "✗ Check Failed"
         "Unable to check for updates"
         "Check your internet connection and try again"
         [Retry] [Cancel]
```

## 6. Security Considerations

### 6.1 HTTPS Only
- All requests to GitHub API via HTTPS (enforced)
- Verify SSL certificates
- No plaintext transmission of any data

### 6.2 File Integrity
- Download DEB from official GitHub releases only
- Verify DEB file integrity (optional: GPG signature check)
- Store in isolated cache directory

### 6.3 User Consent
- Only check when user explicitly requests
- No background auto-checking (by default)
- User must confirm before downloading
- User must confirm before installing

## 7. Configuration (Future Enhancement)

**Optional config entries:**
```ini
[updates]
enable_auto_check=false
check_interval=604800  # seconds (1 week)
auto_download=false
notify_on_update=true
```

## 8. Testing Plan

**Unit Tests:**
- Version comparison logic (1.0.0 vs 1.1.0, 2.0.0, etc.)
- JSON parsing with various GitHub API responses
- Error handling for network failures

**Integration Tests:**
- Full update check flow
- Download functionality
- UI signal/slot connections

**Manual Testing:**
- Check for updates with network
- Test without internet connection
- Verify DEB download integrity
- Test installation instructions

## 9. Dependencies

**New Qt Modules:**
- `Qt5::Network` - HTTP requests and responses

**External:**
- GitHub REST API (no auth needed for public repos)
- User's internet connection

**Internal:**
- `Common.h` for version constant
- `MainWindow.h` for UI integration

## 10. Implementation Timeline

**Estimated Effort:**
- UpdateChecker class: 2-3 hours
- UI integration: 1-2 hours
- Testing & fixes: 1-2 hours
- **Total: 4-7 hours**

**Milestones:**
1. Create UpdateChecker class with GitHub API integration
2. Implement version comparison logic
3. Add UI components (dialogs, menu items)
4. Integrate with MainWindow
5. Test all scenarios
6. Add documentation

## 11. Future Enhancements

- Auto-check for updates on startup (configurable)
- Background update checking
- Update history/changelog display
- Direct in-app update installation
- GPG signature verification
- Beta version opt-in
- Update notification settings

## 12. Summary

The "Check for Updates" feature will provide users with:
✅ Easy way to check for new versions
✅ One-click download of latest DEB
✅ Clear update notifications
✅ Simple installation instructions
✅ Secure HTTPS connections
✅ Non-intrusive implementation
✅ Professional user experience
