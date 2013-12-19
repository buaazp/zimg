# - Try to find the libevhtp
# Once done this will define
#
# LIBEVHTP_FOUND - System has libevhtp
# LIBEVHTP_INCLUDE_DIR - the libevhtp include directory
# LIBEVHTP_LIBRARY 0 The library needed to use libevhtp

FIND_PATH(LIBEVHTP_INCLUDE_DIR NAMES evhtp.h)
FIND_LIBRARY(LIBEVHTP_LIBRARY NAMES evhtp PATH /usr/lib /usr/local/lib)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(libevhtp DEFAULT_MSG LIBEVHTP_LIBRARY LIBEVHTP_INCLUDE_DIR)
MARK_AS_ADVANCED(LIBEVHTP_INCLUDE_DIR LIBEVHTP_LIBRARY)

