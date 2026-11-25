cmake_minimum_required(VERSION 3.25)

# Siraj and its host tools are one CMake project. Clang compiles both: host
# tools with the default target, the kernel with --target=x86_64-unknown-none
# and the freestanding flags in cmake/kernel.cmake.
find_program(JINOS_CLANG NAMES clang-21 clang REQUIRED)
find_program(JINOS_CLANGXX NAMES clang++-21 clang++ REQUIRED)
set(CMAKE_C_COMPILER "${JINOS_CLANG}" CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "${JINOS_CLANGXX}" CACHE FILEPATH "" FORCE)
set(CMAKE_ASM_COMPILER "${JINOS_CLANG}" CACHE FILEPATH "" FORCE)

project(jinos LANGUAGES C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

find_program(JINOS_QEMU qemu-system-x86_64 REQUIRED)
find_program(JINOS_XORRISO xorriso REQUIRED)

set(JINOS_LIMINE_DIR "${CMAKE_SOURCE_DIR}/third_party/limine")

include(cmake/kernel.cmake)
add_subdirectory(kernel)

# --- Host tool: the `limine` executable that installs the BIOS stages -------
add_executable(limine-host "${JINOS_LIMINE_DIR}/limine.c")
set_target_properties(limine-host PROPERTIES OUTPUT_NAME limine)
target_compile_options(limine-host PRIVATE -O2 -w)

# --- Images and runners ----------------------------------------------------
set(JINOS_ISO "${CMAKE_BINARY_DIR}/siraj.iso")
set(JINOS_TEST_ISO "${CMAKE_BINARY_DIR}/siraj-selftest.iso")
set(JINOS_MKIMAGE "${CMAKE_SOURCE_DIR}/tools/mkimage.sh")
set(JINOS_RUN "${CMAKE_SOURCE_DIR}/tools/run-qemu.sh")

add_custom_command(
  OUTPUT "${JINOS_ISO}"
  COMMAND "${JINOS_MKIMAGE}" "$<TARGET_FILE:siraj>" "${CMAKE_SOURCE_DIR}/boot/limine.conf"
          "${JINOS_ISO}" "${JINOS_LIMINE_DIR}" "$<TARGET_FILE:limine-host>"
  DEPENDS siraj limine-host "${JINOS_MKIMAGE}" "${CMAKE_SOURCE_DIR}/boot/limine.conf"
  COMMENT "Building ${JINOS_ISO}")
add_custom_command(
  OUTPUT "${JINOS_TEST_ISO}"
  COMMAND "${JINOS_MKIMAGE}" "$<TARGET_FILE:siraj>" "${CMAKE_SOURCE_DIR}/boot/limine-selftest.conf"
          "${JINOS_TEST_ISO}" "${JINOS_LIMINE_DIR}" "$<TARGET_FILE:limine-host>"
  DEPENDS siraj limine-host "${JINOS_MKIMAGE}" "${CMAKE_SOURCE_DIR}/boot/limine-selftest.conf"
  COMMENT "Building ${JINOS_TEST_ISO}")

add_custom_target(image ALL DEPENDS "${JINOS_ISO}")
add_custom_target(test-image DEPENDS "${JINOS_TEST_ISO}")

add_custom_target(run      COMMAND "${JINOS_RUN}" uefi "${JINOS_ISO}" DEPENDS image USES_TERMINAL)
add_custom_target(run-bios COMMAND "${JINOS_RUN}" bios "${JINOS_ISO}" DEPENDS image USES_TERMINAL)
add_custom_target(debug    COMMAND "${JINOS_RUN}" debug "${JINOS_ISO}" DEPENDS image USES_TERMINAL)
add_custom_target(check
  COMMAND "${CMAKE_SOURCE_DIR}/tests/boot_test.sh" "${JINOS_TEST_ISO}"
  DEPENDS test-image USES_TERMINAL)
