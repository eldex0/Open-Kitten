cmake_minimum_required(VERSION 3.24)
project(OpenKitten VERSION 2.0.0 LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)

# Keep the Windows portable package flat so the executable can resolve Qt and
# the Rust security DLL without relying on a machine-wide PATH.
if(WIN32)
    set(CMAKE_INSTALL_BINDIR "." CACHE STRING "Executable install directory" FORCE)
endif()

if(MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
    include(InstallRequiredSystemLibraries)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(native)
