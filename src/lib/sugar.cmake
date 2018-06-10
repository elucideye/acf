sugar_include(acf)

# NOTE: The util directory contains utility classes/functions used by the acf
# library.  In this context they are effectively non-exported == private
# headers.  They are also used as a support lib in the various console apps
# and unit tests within the repository.
#
# The util directory sits parallel with the root acf folder so that the
# target_include_directories() calls can include util headers without
# automatically including acf headers from the repository tree, as we want
# to make sure the acf folder includes can be provided exclusively from the
# the installed/managed acf package, using:
#
#  hunter_config(acf GIT_SELF ...)
#
# Layout:
#
#     lib
#      +-util  : utility function
#      +-io    : io/serialization functions
#      +-acf
#         +-acf : acf implementation

#
# Example src/app/acf/acf.cpp:
#
# #include <acf/ACF.h>      // from ${HOME}/.hunter/<SNIP>/Install/acf
# #include <util/Logger.h>  // provided from acf/src/lib/util (in-repo)
#
# int main(int argc, char **argv)
# {
#   // snip
# }
#

sugar_include(util)
sugar_include(io)

