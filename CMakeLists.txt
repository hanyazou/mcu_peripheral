cmake_minimum_required(VERSION 3.0)

project(mcu_peripheral)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

find_package(PkgConfig REQUIRED)

if(MCUPR_IMPL STREQUAL "pigpiod")
  # pigpio (https://github.com/smurfix/pigpio)
  find_package(pigpio REQUIRED)
  if(pigpio_FOUND)
    message("pigpio_src=${pigpio_src}")
      set(pigpio_src "src/impl_pigpiod.c")
  endif()
endif()

if(MCUPR_IMPL STREQUAL "libmpsse")
  # libmpsse (https://github.com/devttys0/libmpsse)
  # ./configure --disable-python
  find_package(libmpsse REQUIRED)
  if(libmpsse_FOUND)
      pkg_check_modules(LIBFTDI REQUIRED libftdi)
      set(libmpsse_src "src/impl_libmpsse.c")
  endif()
endif()

if(NOT DEFINED MCUPR_IMPL OR MCUPR_IMPL STREQUAL "" OR MCUPR_IMPL STREQUAL "linuxdev")
  set(libmpsse_src "src/impl_linuxdev.c")
endif()

add_library(mcupr SHARED
    src/mcu_peripheral.c
    src/error.c
    src/log.c
    ${pigpio_src}
    ${libmpsse_src}
)
target_compile_definitions(mcupr PUBLIC MCUPR_DEBUG)
target_include_directories(mcupr PUBLIC include)

if(pigpio_FOUND)
    target_link_libraries(mcupr PRIVATE pigpiod_if2)
    target_link_directories(mcupr PRIVATE ${pigpio_LIBRARY_DIRS})
endif()
if(libmpsse_FOUND)
    target_include_directories(mcupr PRIVATE ${LIBFTDI_INCLUDE_DIRS})
    target_link_directories(mcupr PRIVATE ${LIBFTDI_LIBRARY_DIRS})
    target_link_libraries(mcupr PRIVATE ftdi)
    target_include_directories(mcupr PRIVATE ${libmpsse_INCLUDE_DIRS})
    target_link_directories(mcupr PRIVATE ${libmpsse_LIBRARY_DIRS})
    target_link_libraries(mcupr PRIVATE mpsse)
endif()

#
# example executables
#
add_executable(tsl2561 examples/tsl2561.c)
target_link_libraries(tsl2561 mcupr)

add_executable(i2cdetect examples/i2cdetect.c)
target_link_libraries(i2cdetect mcupr)

add_executable(gpio examples/gpio.c)
target_link_libraries(gpio mcupr)

install(TARGETS mcupr DESTINATION ${CMAKE_INSTALL_LIBDIR})
