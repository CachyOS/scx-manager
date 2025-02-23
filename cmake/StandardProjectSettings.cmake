# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE
      RelWithDebInfo
      CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui, ccmake
  set_property(
    CACHE CMAKE_BUILD_TYPE
    PROPERTY STRINGS
             "Debug"
             "Release"
             "MinSizeRel"
             "RelWithDebInfo")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050F00)

# Generate compile_commands.json to make it easier to work with clang based tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build with PIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  #add_compile_options(-nostdlib++ -stdlib=libc++ -nodefaultlibs -fexperimental-library)
  #add_link_options(-stdlib=libc++)

  add_compile_options(-fstrict-vtable-pointers)

  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
    # Set new experimental pass manager, it's a performance, build time and binary size win.
    # Can be removed after https://reviews.llvm.org/D66490 merged and released to at least two versions of clang.
    add_compile_options(-fexperimental-new-pass-manager)
  endif()
endif()

option(ENABLE_IPO "Enable Interprocedural Optimization, aka Link Time Optimization (LTO)" OFF)

if(ENABLE_IPO)
  include(CheckIPOSupported)
  check_ipo_supported(
    RESULT
    result
    OUTPUT
    output)
  if(result)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  else()
    message(SEND_ERROR "IPO is not supported: ${output}")
  endif()
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  add_compile_options(-fcolor-diagnostics)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fdiagnostics-color=always)
else()
  message(STATUS "No colored compiler diagnostic set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
endif()

# Enables STL container checker if not building a release.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-D_GLIBCXX_ASSERTIONS)
  add_definitions(-D_LIBCPP_ENABLE_THREAD_SAFETY_ANNOTATIONS=1)
  add_definitions(-D_LIBCPP_ENABLE_ASSERTIONS=1)
endif()

# Prevent shared library to export unintended symbols
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

# cmake defines for packaging
set(SCXCTLUI_TARGET_NAME                scxctl-ui)
set(SCXCTLUI_CONFIG_INSTALL_DIR         "${CMAKE_INSTALL_LIBDIR}/cmake/${SCXCTLUI_TARGET_NAME}" CACHE INTERNAL "")
set(SCXCTLUI_INCLUDE_BUILD_DIR          "${PROJECT_SOURCE_DIR}/include")
set(SCXCTLUI_INCLUDE_INSTALL_DIR        "${CMAKE_INSTALL_INCLUDEDIR}")
set(SCXCTLUI_TARGETS_EXPORT_NAME        "${SCXCTLUI_TARGET_NAME}-targets")
set(SCXCTLUI_CMAKE_CONFIG_TEMPLATE      "${PROJECT_SOURCE_DIR}/cmake/scxctl-ui-config.cmake.in")
set(SCXCTLUI_CMAKE_CONFIG_DIR           "${CMAKE_CURRENT_BINARY_DIR}")
set(SCXCTLUI_CMAKE_VERSION_CONFIG_FILE  "${SCXCTLUI_CMAKE_CONFIG_DIR}/${SCXCTLUI_TARGET_NAME}-config-version.cmake")
set(SCXCTLUI_CMAKE_PROJECT_CONFIG_FILE  "${SCXCTLUI_CMAKE_CONFIG_DIR}/${SCXCTLUI_TARGET_NAME}-config.cmake")
set(SCXCTLUI_CMAKE_PROJECT_TARGETS_FILE "${SCXCTLUI_CMAKE_CONFIG_DIR}/${SCXCTLUI_TARGET_NAME}-targets.cmake")
