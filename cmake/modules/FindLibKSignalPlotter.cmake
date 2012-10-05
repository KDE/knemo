# - Find libksignalplotter
#
# This module defines
#  LIBKSIGNALPLOTTER_FOUND - whether the libksignalplotter library was found
#  LIBKSIGNALPLOTTER_LIBRARY - the libksignalplotter library
#  LIBKSIGNALPLOTTER_INCLUDE_DIR - the include path of the libksignalplotter library

find_library (LIBKSIGNALPLOTTER_LIBRARY ksignalplotter)

find_path (LIBKSIGNALPLOTTER_INCLUDE_DIR
  ksysguard/ksignalplotter.h
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibKSignalPlotter DEFAULT_MSG LIBKSIGNALPLOTTER_LIBRARY LIBKSIGNALPLOTTER_INCLUDE_DIR)

mark_as_advanced(LIBKSIGNALPLOTTER_INCLUDE_DIR LIBKSIGNALPLOTTER_LIBRARY)
