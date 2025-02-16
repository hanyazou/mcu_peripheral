# FindLibMPSSE.cmake
# This module finds the libmpsse library and headers.
# It defines the following variables:
#   libmpsse_FOUND        - True if libmpsse is found
#   libmpsse_INCLUDE_DIRS - Path to the libmpsse include directory
#   libmpsse_LIBRARY_DIRS - Path to the libmpsse library directory

find_path(libmpsse_INCLUDE_DIRS
    NAMES mpsse.h
    PATHS
        /usr/include
        /usr/local/include
        $ENV{LIBMPSSE_INCLUDE_DIRS}
)

find_path(libmpsse_LIBRARY_DIRS
    NAMES libmpsse.a
    PATHS
        /usr/lib
        /usr/local/lib
        $ENV{LIBMPSSE_LIBRARY_DIRS}
)

# Check if both include directory and library are found
if (libmpsse_INCLUDE_DIRS AND libmpsse_LIBRARY_DIRS)
    set(libmpsse_FOUND TRUE)
else()
    set(libmpsse_FOUND FALSE)
endif()

# Make the variables available to the parent scope
if (libmpsse_FOUND)
    message(STATUS "Found libmpsse libraries: ${libmpsse_LIBRARY_DIRS}")
    message(STATUS "Found libmpsse headers: ${libmpsse_INCLUDE_DIRS}")
else()
    message(WARNING "libmpsse not found. Please install it.")
endif()

mark_as_advanced(
    libmpsse_INCLUDE_DIRS
    libmpsse_LIBRARY_DIRS
)
