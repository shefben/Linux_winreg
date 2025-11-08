cmake_minimum_required(VERSION 3.10)
project(Linux_WinReg VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_EXAMPLES "Build example programs" ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/winreg.c
    src/db_backend.c
    src/registry_utils.c
)

# Header files
set(HEADERS
    include/winreg.h
    include/winnt.h
    include/winbase.h
    include/winerror.h
)

# Create static library
add_library(winreg_static STATIC ${SOURCES})
target_link_libraries(winreg_static PUBLIC
    SQLite::SQLite3
    Threads::Threads
)
set_target_properties(winreg_static PROPERTIES OUTPUT_NAME winreg)

# Create shared library if enabled
if(BUILD_SHARED_LIBS)
    add_library(winreg_shared SHARED ${SOURCES})
    target_link_libraries(winreg_shared PUBLIC
        SQLite::SQLite3
        Threads::Threads
    )
    set_target_properties(winreg_shared PROPERTIES
        OUTPUT_NAME winreg
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
    )
endif()

# Build examples if enabled
if(BUILD_EXAMPLES)
    add_executable(test_registry examples/test_registry.c)
    target_link_libraries(test_registry PRIVATE winreg_static)
endif()

# Installation rules
install(TARGETS winreg_static
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

if(BUILD_SHARED_LIBS)
    install(TARGETS winreg_shared
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

install(FILES ${HEADERS} DESTINATION include/winreg)

# Add test target
if(BUILD_EXAMPLES)
    enable_testing()
    add_test(NAME registry_test COMMAND test_registry)
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "Linux Windows Registry API Configuration:")
message(STATUS "  Version:          ${PROJECT_VERSION}")
message(STATUS "  Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared libs:      ${BUILD_SHARED_LIBS}")
message(STATUS "  Build examples:   ${BUILD_EXAMPLES}")
message(STATUS "  Install prefix:   ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
