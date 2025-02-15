# Find the pigpio library and headers
find_path(pigpio_INCLUDE_DIR NAMES pigpio.h)
find_library(pigpio_LIBRARY NAMES pigpio)

if (pigpio_INCLUDE_DIR AND pigpio_LIBRARY)
    set(pigpio_FOUND TRUE)
endif()

if (pigpio_FOUND)
    message(STATUS "Found pigpio: ${PIGPIO_LIBRARY}")
else()
    message(FATAL_ERROR "Could not find pigpio")
endif()

mark_as_advanced(pigpio_INCLUDE_DIR pigpio_LIBRARY)
