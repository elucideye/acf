# modularize options for reuse
option(ACF_BUILD_SHARED_SDK "Build ACF as a shared library" OFF)
option(ACF_BUILD_APPS "Build applications" ON)
option(ACF_SERIALIZE_WITH_CEREAL "Serialize w/ cereal" ON) # always on
option(ACF_SERIALIZE_WITH_CVMATIO "Build with CVMATIO" ON)
option(ACF_BUILD_OGLES_GPGPU "Build with OGLES_GPGPU" ON)
option(ACF_BUILD_TESTS "Build tests" OFF)
option(ACF_BUILD_EXAMPLES "Build examples" ON)
option(ACF_OPENGL_ES2 "Use OpenGL ES 2.0 (and compatible)" OFF)
option(ACF_OPENGL_ES3 "Use OpenGL ES 3.0 (and compatible)" OFF)
option(ACF_HAS_GPU "Drishti has GPU" ON)
option(ACF_USE_EGL "Use EGL context" OFF)
