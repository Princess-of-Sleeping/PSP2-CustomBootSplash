cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(cbsm)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

add_executable(cbsm
  main.c
  blit.c
  font.c
)

target_link_libraries(cbsm
  SceSysmemForDriver_stub
  SceSysrootForDriver_stub
  SceKernelUtilsForDriver_stub
  SceCpuForDriver_stub
  SceThreadmgrForDriver_stub
  SceIofilemgrForDriver_stub
  SceDisplayForDriver_stub
  SceSysclibForDriver_stub
  SceSysrootForKernel_stub
  SceDipswForDriver_stub
  k
  gcc
)

set_target_properties(cbsm
  PROPERTIES LINK_FLAGS "-nostdlib"
)

vita_create_self(cbsm.skprx cbsm
  CONFIG exports.yml
  UNSELF
)
