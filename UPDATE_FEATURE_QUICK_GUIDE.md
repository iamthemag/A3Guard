# A3Guard Check for Updates - Quick Reference Guide

## ⚡ Quick Start

### Testing the Feature

```bash
# Build
cd /home/test/A3Guard/build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Run
sudo ./A3Guard  # or pkexec ./A3Guard

# Test
1. Click "Help" menu
2. Click "Check for Updates..."
3. Watch the magic happen!
```

## 📋 Feature Checklist

- ✅ GitHub API integration (no auth needed)
- ✅ HTTPS-only communication
- ✅ Semantic version comparison
- ✅ Download management
- ✅ User-friendly dialogs
- ✅ Rate limiting (1 hour)
- ✅ Error handling with retry
- ✅ Status bar progress
- ✅ Installation instructions
- ✅ Full production-ready code

## 🎯 How It Works (In 30 Seconds)

```
User clicks "Check for Updates"
         ↓
Query GitHub API (HTTPS)
         ↓
Parse JSON response
         ↓
Compare versions
         ↓
If newer: Show download dialog
If same: Show "up to date"
If error: Show retry option
         ↓
User clicks Download
         ↓
Download to ~/.cache/a3guard/
         ↓
Show installation instructions
         ↓
User installs: sudo dpkg -i ~/.cache/a3guard/a3guard_*.deb
```

## 📁 Files Created

| File | Lines | Purpose |
|------|-------|---------|
| `include/UpdateChecker.h` | 60 | Class definition |
| `src/UpdateChecker.cpp` | 320 | Implementation |

## ✏️ Files Modified

| File | Changes | Purpose |
|------|---------|---------|
| `include/MainWindow.h` | +10 lines | Add UpdateChecker member |
| `src/MainWindow.cpp` | +133 lines | UI integration |
| `CMakeLists.txt` | +2 lines | Build configuration |

## 🔧 Key Components

### UpdateChecker Class
```cpp
public:
  void checkForUpdates()                    // Start checking
  QString getCurrentVersion() const         // Get app version
  QString getLatestVersion() const          // Get GitHub version
  QString getDownloadUrl() const            // Get DEB URL
  QString getReleaseNotes() const           // Get release notes
  bool isUpdateAvailable() const            // Is update newer?

signals:
  void checkStarted()                       // Check started
  void updateAvailable(...)                 // New version found
  void noUpdateAvailable()                  // Already up to date
  void checkFailed(QString)                 // Check failed
  void downloadProgress(qint64, qint64)     // Download progress
  void downloadFinished(QString)            // Download complete
```

### GitHub API Endpoint

```
GET https://api.github.com/repos/iamthemag/A3Guard/releases/latest
Headers: User-Agent: A3Guard/1.0.0

Response:
{
  "tag_name": "v1.1.0",
  "body": "Release notes...",
  "assets": [
    {
      "name": "a3guard_1.1.0_amd64.deb",
      "browser_download_url": "https://github.com/.../a3guard_1.1.0_amd64.deb"
    }
  ]
}
```

## 🛡️ Security Features

| Feature | Implementation |
|---------|-----------------|
| HTTPS Only | ✅ All requests encrypted |
| No Auth Needed | ✅ Public GitHub API |
| User Consent | ✅ Manual checks only |
| File Isolation | ✅ ~/.cache/a3guard/ |
| Error Safe | ✅ Graceful degradation |

## 🚀 Release Workflow

```bash
# 1. Update version
./scripts/update-version.sh 1.0.0 1.1.0

# 2. Commit
git add .
git commit -m "Bump version: 1.0.0 → 1.1.0"

# 3. Tag & Push
git tag -a v1.1.0 -m "Release v1.1.0"
git push origin main && git push origin v1.1.0

# 4. GitHub Actions builds DEB automatically
# 5. Release appears on GitHub
# 6. Users can now: Help → Check for Updates → Download → Install
```

## 🐛 Troubleshooting

| Problem | Solution |
|---------|----------|
| "Failed to check for updates" | Check internet connection |
| "Already checked recently" | Wait 1 hour (rate limit) |
| Download doesn't start | Check ~/.cache/ permissions |
| Can't find DEB in release | Ensure workflow created DEB file |

## 📊 Build Status

```
Build: ✅ SUCCESS
Errors: 0
Warnings: 2 (non-critical deprecation warnings)
Binary: 537 KB (Release build)
Type: ELF 64-bit x86-64
Location: /home/test/A3Guard/build/A3Guard
```

## 💡 Implementation Highlights

### Version Comparison
```cpp
// Semantic versioning: major.minor.patch
isNewerVersion("1.0.0", "1.1.0") → true
isNewerVersion("1.1.0", "1.0.0") → false
```

### Rate Limiting
```cpp
// Max 1 check per hour
if (m_lastCheckTime.secsTo(now) < 3600) {
    emit checkFailed("Already checked recently");
}
```

### Error Handling
```cpp
// Timeout after 30 seconds
QTimer *timer = new QTimer(this);
timer->start(30000);  // 30 seconds
```

## 🎨 UI Elements

### Menu Item
```
Help
├─ Check for Updates...  ← NEW
├─ ──────────
└─ About
```

### Dialogs
1. **Update Available**: Shows version comparison + Download button
2. **Download Progress**: Status bar with MB/speed
3. **Installation**: Shows command to run
4. **Already Up to Date**: "You have the latest version"
5. **Error**: "Check internet" + Retry button

## 📝 Code Statistics

- **Total New Code**: ~525 lines
- **Files Created**: 2
- **Files Modified**: 3
- **Build Time**: ~5 seconds
- **Binary Size**: 537 KB

## 🔍 Key Functions

### Check for Updates
```cpp
m_updateChecker->checkForUpdates();
// Queries GitHub API every hour max
```

### Parse Response
```cpp
void UpdateChecker::parseGitHubResponse(const QByteArray &data)
// Extracts: version, release notes, DEB URL
```

### Version Compare
```cpp
bool UpdateChecker::isNewerVersion(const QString &current, const QString &latest)
// Compares semantic versions
```

### Download Handler
```cpp
void MainWindow::onDownloadFinished(QString filePath)
// Shows installation instructions
```

## ✨ Features

✅ Check latest version from GitHub  
✅ Compare semantic versions  
✅ Download DEB package  
✅ Show progress bar  
✅ Installation instructions  
✅ Error handling & retry  
✅ Rate limiting (1 hour)  
✅ HTTPS only  
✅ User consent required  
✅ Production-ready  

## 🎓 How to Use

### For Users
1. Open A3Guard
2. Click: Help → Check for Updates...
3. Wait for result
4. If update available:
   - Click "Download"
   - Wait for download
   - Run: `sudo dpkg -i ~/.cache/a3guard/a3guard_*.deb`
5. Done!

### For Developers
1. Make code changes
2. Update version: `./scripts/update-version.sh 1.0.0 1.1.0`
3. Commit & tag
4. Push to GitHub
5. GitHub Actions builds DEB
6. Release appears automatically
7. Users get notified on next "Check for Updates"

## 📞 Support

**Issue:** Check fails repeatedly  
**Solution:** Restart app, check internet connection, check GitHub status

**Issue:** Can't write to cache directory  
**Solution:** `chmod 755 ~/.cache/` and verify permissions

**Issue:** Want to disable rate limiting for testing  
**Solution:** Edit `RATE_LIMIT_SECONDS` in `UpdateChecker.h` (default: 3600)

---

**Status: ✅ Implementation Complete**  
**Binary: Ready at `/home/test/A3Guard/build/A3Guard`**  
**Documentation: Complete**
