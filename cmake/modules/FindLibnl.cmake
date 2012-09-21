# - Find libnl
#
# This module defines
#  LIBNL_FOUND - whether the libnl library was found
#  LIBNL_LIBRARIES - the libnl library
#  LIBNL_INCLUDE_DIR - the include path of the libnl library

find_library (LIBNL_LIBRARY nl-3)
find_library (LIBNL_ROUTE_LIBRARY nl-route-3)

set(LIBNL_LIBRARIES
    ${LIBNL_LIBRARY}
    ${LIBNL_ROUTE_LIBRARY}
)

find_path (LIBNL_INCLUDE_DIR
  NAMES
  netlink/netlink.h
  PATH_SUFFIXES
  libnl3
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libnl  DEFAULT_MSG  LIBNL_LIBRARIES LIBNL_INCLUDE_DIR)

include_directories("${LIBNL_INCLUDE_DIR}")
mark_as_advanced(LIBNL_INCLUDE_DIR LIBNL_LIBRARY LIBNL_ROUTE_LIBRARY)
