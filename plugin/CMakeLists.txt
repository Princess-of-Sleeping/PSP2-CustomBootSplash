cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(custom_boot_splash)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

add_executable(custom_boot_splash
  main.c
)

target_link_libraries(custom_boot_splash
  SceSysmemForDriver_stub
  SceSysrootForDriver_stub
  SceKernelUtilsForDriver_stub
  SceCpuForDriver_stub
  SceThreadmgrForDriver_stub
  SceIofilemgrForDriver_stub
  SceDisplayForDriver_stub
)

vita_create_self(cbs.skprx custom_boot_splash
  CONFIG exports.yml
  UNSELF
)
