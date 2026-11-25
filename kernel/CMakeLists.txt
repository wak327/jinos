set(SIRAJ_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/arch/x86_64/linker.ld")

add_executable(siraj
  arch/x86_64/start.S
  arch/x86_64/mem.S
  arch/x86_64/limine.cpp
  arch/x86_64/serial.cpp
  core/main.cpp
  core/print.cpp
  core/fbcon.cpp
  core/font.S
  core/panic.cpp
  core/runtime.cpp
  ../lib/kstd/string.cpp
)
jinos_kernel_target(siraj)
set_target_properties(siraj PROPERTIES
  OUTPUT_NAME siraj.elf
  LINK_DEPENDS "${SIRAJ_LINKER_SCRIPT}")
target_link_options(siraj PRIVATE
  ${JINOS_KERNEL_LINK_FLAGS}
  "-Wl,-T,${SIRAJ_LINKER_SCRIPT}")
