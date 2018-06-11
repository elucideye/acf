if(DEFINED APP_COMMON_SUGAR_CMAKE_)
  return()
else()
  set(APP_COMMON_SUGAR_CMAKE_ 1)
endif()

include(sugar_files)

sugar_files(ACF_COMMON_HDRS
  LazyParallelResource.h
  Line.h
  Logger.h
  ScopeTimeLogger.h
  cli.h
  acf_common.h
  make_unique.h
)

sugar_files(ACF_COMMON_SRCS
  Logger.cpp
)
