# Contributing to A3Guard

Thank you for your interest in contributing to A3Guard! This document provides guidelines for contributing to the project.

## 🚀 Quick Start

1. **Fork the repository** on GitHub
2. **Clone your fork** locally
3. **Create a feature branch** from `develop`
4. **Make your changes** following our coding standards
5. **Test your changes** thoroughly
6. **Submit a pull request** to the `develop` branch

## 📋 Development Setup

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install -y \
    build-essential \
    cmake \
    qtbase5-dev \
    qtbase5-dev-tools \
    libssl-dev \
    libudev-dev \
    libx11-dev \
    libxfixes-dev \
    pkg-config \
    clang-format \
    clang-tidy \
    cppcheck
```

### Building

```bash
# Clone your fork
git clone https://github.com/YOUR_USERNAME/a3guard.git
cd a3guard

# Create build directory
mkdir build && cd build

# Configure and build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Run tests (when available)
ctest
```

## 🎯 Contributing Areas

We welcome contributions in the following areas:

### 🐛 Bug Fixes
- Security vulnerabilities
- Memory leaks or crashes  
- UI/UX issues
- Performance problems
- Cross-platform compatibility

### ✨ New Features
- Additional monitoring capabilities
- New export formats
- Enhanced encryption methods
- Improved alert systems
- Better configuration options

### 📚 Documentation
- Code documentation
- User guides
- Installation instructions
- API documentation
- Security best practices

### 🧪 Testing
- Unit tests
- Integration tests
- Security tests
- Performance benchmarks
- Cross-platform testing

## 📝 Coding Standards

### C++ Style Guide

We follow a consistent C++ coding style:

```cpp
// Use meaningful names
class SecurityManager {
private:
    std::shared_ptr<ConfigManager> m_config;  // Member variables with m_ prefix
    bool m_initialized;
    
public:
    // Clear, descriptive function names
    bool initialize();
    QByteArray encryptData(const QByteArray& data);
};

// Use const correctness
QString formatDuration(const QDateTime& start) const;

// RAII and smart pointers
auto config = std::make_shared<ConfigManager>();

// Error handling
if (!security->initialize()) {
    LOG_ERROR("Failed to initialize security manager");
    return false;
}
```

### Code Formatting

We use `clang-format` with the following style:

```bash
# Format all source files
find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Check formatting in CI
find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -style=file -dry-run -Werror
```

### Qt Conventions

- Use Qt naming conventions for Qt-specific code
- Prefer Qt containers and types when interfacing with Qt APIs
- Use signals/slots for event handling
- Follow Qt's memory management patterns

```cpp
// Qt style
connect(m_timer, &QTimer::timeout, this, &MainWindow::updateUI);

// Prefer Qt types for Qt APIs
QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"));
```

## 🔒 Security Guidelines

A3Guard handles sensitive monitoring data. Please follow these security guidelines:

### Secure Coding Practices

1. **Input Validation**: Always validate and sanitize input
2. **Error Handling**: Don't leak sensitive information in error messages
3. **Memory Management**: Use RAII and smart pointers
4. **Cryptography**: Use proven algorithms (AES-256, SHA-256)
5. **Privilege Escalation**: Minimize privileged code paths

```cpp
// Good: Validate input
if (filePath.isEmpty() || !QFile::exists(filePath)) {
    LOG_ERROR("Invalid file path provided");
    return false;
}

// Good: Secure memory handling
class SecurityManager {
private:
    QByteArray m_key;  // Automatically cleared when destroyed
    
    void clearSensitiveData() {
        m_key.fill(0);  // Clear sensitive data explicitly
    }
};
```

### Security Review Process

- All security-related changes require review by maintainers
- Cryptographic code must be reviewed by security experts
- New dependencies must be vetted for security
- Security test coverage is mandatory

## 🧪 Testing Requirements

### Unit Tests

```cpp
// Example test structure (when implemented)
TEST(SecurityManagerTest, EncryptionDecryption) {
    auto config = std::make_shared<MockConfigManager>();
    auto logger = std::make_shared<MockLogger>();
    
    SecurityManager security(config, logger);
    ASSERT_TRUE(security.initialize());
    
    QByteArray testData = "test data";
    QByteArray encrypted = security.encrypt(testData);
    QByteArray decrypted = security.decrypt(encrypted);
    
    EXPECT_EQ(testData, decrypted);
}
```

### Integration Tests

Test the full application workflow:
- Start monitoring → Take screenshots → Stop monitoring
- Network control → Verify interface states
- File integrity → Modify file → Detect violation

### Performance Tests

Ensure minimal resource usage:
- CPU usage < 10%
- Memory usage < 100MB
- Fast startup time
- Responsive UI

## 📊 Pull Request Process

### Before Submitting

1. **Fork and branch** from `develop`
2. **Write clear commit messages**:
   ```
   feat: add clipboard monitoring with hash verification
   
   - Implement clipboard change detection
   - Add SHA-256 hashing of clipboard content
   - Update configuration for clipboard monitoring interval
   - Add unit tests for clipboard monitoring
   
   Fixes #123
   ```

3. **Update documentation** if needed
4. **Add or update tests**
5. **Run local checks**:
   ```bash
   # Build and test
   cmake --build build
   ctest
   
   # Format check
   clang-format --dry-run --Werror src/*.cpp include/*.h
   
   # Static analysis
   cppcheck src/ include/
   ```

### Pull Request Template

```markdown
## Description
Brief description of changes made.

## Type of Change
- [ ] Bug fix (non-breaking change)
- [ ] New feature (non-breaking change)  
- [ ] Breaking change (fix or feature that would cause existing functionality to not work)
- [ ] Documentation update
- [ ] Security fix

## Testing
- [ ] Unit tests pass
- [ ] Integration tests pass
- [ ] Manual testing completed
- [ ] Performance impact assessed

## Security Impact
- [ ] No security impact
- [ ] Security enhancement
- [ ] Potential security risk (requires security review)

## Checklist
- [ ] Code follows style guidelines
- [ ] Self-review completed
- [ ] Documentation updated
- [ ] Changes are backwards compatible
```

### Review Process

1. **Automated checks** must pass (CI/CD pipeline)
2. **Code review** by at least one maintainer
3. **Security review** for sensitive changes
4. **Testing** on multiple platforms if applicable
5. **Documentation review** if docs are updated

## 🏷️ Release Process

### Version Numbering

We follow [Semantic Versioning](https://semver.org/):
- `MAJOR.MINOR.PATCH` (e.g., 1.2.3)
- Major: Breaking changes
- Minor: New features, backwards compatible
- Patch: Bug fixes, backwards compatible

### Release Steps

1. **Update version** in CMakeLists.txt and Common.h
2. **Update CHANGELOG.md**
3. **Create release PR** to `main`
4. **Tag release** after merge: `git tag v1.2.3`
5. **Push tag**: `git push origin v1.2.3`
6. **GitHub Actions** will build and publish packages

## 🤝 Code of Conduct

### Our Standards

- **Respectful communication**: Be kind and professional
- **Constructive feedback**: Focus on the code, not the person
- **Inclusive environment**: Welcome contributors of all backgrounds
- **Educational purpose**: Remember this is for educational use

### Unacceptable Behavior

- Harassment, discrimination, or offensive language
- Personal attacks or trolling
- Sharing private information without permission
- Any conduct inappropriate for an educational environment

## 📞 Getting Help

### Community Support

- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: Questions and general discussion
- **Code Review**: PR comments and feedback

### Documentation

- **README.md**: Installation and usage
- **API Documentation**: Generated with Doxygen
- **Security Guide**: Security best practices
- **Architecture**: System design documentation

### Contact Maintainers

For security issues, please email maintainers directly rather than opening public issues.

## 🎉 Recognition

Contributors will be recognized in:
- **CONTRIBUTORS.md** file
- **GitHub contributors** section
- **Release notes** for significant contributions
- **Documentation** credits

## 📄 Legal

By contributing to A3Guard, you agree that your contributions will be licensed under the same license as the project (MIT License with additional educational terms).

---

**Thank you for contributing to A3Guard! Together we can build better, more secure educational assessment tools. 🚀🔒**