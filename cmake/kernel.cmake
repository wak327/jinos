# Flags for code that runs inside Siraj. See docs/design.md section 7.8.

set(JINOS_KERNEL_TARGET x86_64-unknown-none-elf)

set(JINOS_KERNEL_COMMON_FLAGS
  --target=${JINOS_KERNEL_TARGET}
  -ffreestanding -nostdlib -nostdinc++
  -fno-pic -fno-pie
  -fno-omit-frame-pointer
  -mno-red-zone -mcmodel=kernel -mcx16
  # No floating point or SIMD in the kernel; user threads own those registers.
  -mno-sse -mno-sse2 -mno-mmx -mno-avx -mno-80387 -msoft-float
  -fstack-protector-strong -mstack-protector-guard=global
  -ffunction-sections -fdata-sections
  -fno-asynchronous-unwind-tables -fno-unwind-tables
  -Wall -Wextra -Wshadow -Wconversion -Wsign-conversion
  -Werror=return-type -Werror=implicit-fallthrough
  -g -O2
)

set(JINOS_KERNEL_CXX_FLAGS
  -std=c++20
  -fno-exceptions -fno-rtti -fno-threadsafe-statics
  -fno-use-cxa-atexit
  -Werror=global-constructors -Werror=exit-time-destructors
  -Wno-unused-const-variable
)

set(JINOS_KERNEL_LINK_FLAGS
  --target=${JINOS_KERNEL_TARGET}
  -fuse-ld=lld -nostdlib -static
  -Wl,--gc-sections -Wl,-z,max-page-size=0x1000 -Wl,-z,noexecstack
  -Wl,--build-id=sha1
)

# Applies the kernel flags to a target. Every kernel target (and every kernel
# library) goes through this so flags live in one place.
function(jinos_kernel_target name)
  target_compile_options(${name} PRIVATE
    ${JINOS_KERNEL_COMMON_FLAGS}
    $<$<COMPILE_LANGUAGE:CXX>:${JINOS_KERNEL_CXX_FLAGS}>)
  target_include_directories(${name} PRIVATE
    "${CMAKE_SOURCE_DIR}/lib"
    "${CMAKE_SOURCE_DIR}/kernel"
    "${CMAKE_SOURCE_DIR}/third_party/limine-protocol"
    "${CMAKE_SOURCE_DIR}/third_party/fonts")
  target_compile_definitions(${name} PRIVATE JINOS_KERNEL=1)
endfunction()
