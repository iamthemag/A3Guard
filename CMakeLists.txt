cmake_minimum_required(VERSION 3.16)

project(A3Guard 
    VERSION 1.0.0
    DESCRIPTION "Advanced Assessment Application"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type to Release by default for minimal resource usage
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler optimizations for minimal resource usage
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# Find required packages
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Network)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)

# Find system libraries
pkg_check_modules(UDEV REQUIRED libudev)
pkg_check_modules(X11 REQUIRED x11)
pkg_check_modules(XFIXES REQUIRED xfixes)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${UDEV_INCLUDE_DIRS})
include_directories(${X11_INCLUDE_DIRS})
include_directories(${XFIXES_INCLUDE_DIRS})

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/SecurityManager.cpp
    src/MonitoringEngine.cpp
    src/NetworkManager.cpp
    src/SystemController.cpp
    src/ConfigManager.cpp
    src/Logger.cpp
    src/AlertManager.cpp
    src/ResourceMonitor.cpp
    src/SessionManager.cpp
    src/PrivilegeDialog.cpp
    src/UpdateChecker.cpp
)

# Header files
set(HEADERS
    include/MainWindow.h
    include/SecurityManager.h
    include/MonitoringEngine.h
    include/NetworkManager.h
    include/SystemController.h
    include/ConfigManager.h
    include/Logger.h
    include/AlertManager.h
    include/ResourceMonitor.h
    include/SessionManager.h
    include/PrivilegeDialog.h
    include/Common.h
    include/UpdateChecker.h
)

# Qt MOC files
qt5_wrap_cpp(MOC_SOURCES ${HEADERS})

# Qt resources
qt5_add_resources(RESOURCE_SOURCES resources/a3guard.qrc)

# Create main executable
add_executable(${PROJECT_NAME} 
    ${SOURCES} 
    ${MOC_SOURCES} 
    ${RESOURCE_SOURCES}
)

# Create update checker helper executable
qt5_wrap_cpp(UPDATE_CHECKER_MOC include/UpdateCheckerDialog.h)
add_executable(a3guard-update-checker
    src/a3guard-update-checker.cpp
    ${UPDATE_CHECKER_MOC}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::Network
    OpenSSL::SSL
    OpenSSL::Crypto
    ${UDEV_LIBRARIES}
    ${X11_LIBRARIES}
    ${XFIXES_LIBRARIES}
    pthread
)

# Link update checker helper
target_link_libraries(a3guard-update-checker
    Qt5::Core
    Qt5::Gui
    Qt5::Network
)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE 
    ${UDEV_CFLAGS_OTHER}
    ${X11_CFLAGS_OTHER}
    ${XFIXES_CFLAGS_OTHER}
)

# Set RPATH for installed binary
set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "/usr/local/lib"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

# Installation
install(TARGETS ${PROJECT_NAME} a3guard-update-checker
    RUNTIME DESTINATION /opt/a3guard/bin
)

# Install update checker helper to user bin for non-root execution
install(TARGETS a3guard-update-checker
    RUNTIME DESTINATION /usr/local/bin
)

install(FILES config/a3guard.conf
    DESTINATION /etc/a3guard
)

# Install launcher script
install(PROGRAMS a3guard-launcher
    DESTINATION /usr/local/bin
    RENAME a3guard
)

# Install desktop entry
install(FILES A3Guard.desktop
    DESTINATION /usr/share/applications
)

# Install application icon
install(FILES A3Guard.png
    DESTINATION /usr/share/pixmaps
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)


# Package configuration
set(CPACK_PACKAGE_NAME "a3guard")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_CONTACT "admin@example.com")
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/A3Guard.png")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5gui5, libqt5core5a, libqt5widgets5, libqt5network5, libssl3, libudev1, libx11-6, libxfixes3")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SECTION "education")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/iamthemag/A3Guard")

include(CPack)