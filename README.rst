=== 
acf
===
Aggregated Channel Feature object detection in C++ and OpenGL ES 2.0

|TravisCI| |Appveyor| |License| |Hunter|

This module is very well suited to running real time object detection on mobile processors, where recent high performing but GPU needy DNN approaches aren't as suitable.  The ACF pyramids can be computed with the OpenGL ES 2.0 shaders and retrieved more or less for free (< 1 frame time with 1 frame of latency).  For selfie video, the pretrained face detectors (see FACE80_ and FACE64_) run in a few milliseconds on an iPhone 7.  TODO: The Locally Decorrelated Channel Feature addition has not yet been added (see LDCF_), but the 5x5 kernels should map well to OpenGL shaders.  That should make performance very competitive (see Piotr's references for comparisons).

.. _FACE80: https://github.com/elucideye/drishti-assets/blob/master/drishti_face_gray_80x80.cpb
.. _FACE64: https://github.com/elucideye/drishti-assets/blob/master/drishti_face_gray_64x64.cpb
.. _LDCF: https://arxiv.org/pdf/1406.1134.pdf

Sample 10 Channel ACF from GPU: LUV + magnitude (locally normalized) + gradient orientation (6 bins):

.. image:: https://cloud.githubusercontent.com/assets/554720/21356618/4decbb4c-c6a0-11e6-8d8a-d1a3fc23c742.jpg

- C++ implementation of `Fast Feature Pyramids for Object Detection`_ (see `Piotr's Matlab Toolbox`_)
- `Hunter`_ package management for cross platform builds: "Organized Freedom!" :)
- Fast OpenGL ES 2.0 ACF Pyramid computation via `ogles_gpgpu`_
- `OpenCV`_ API

.. _OpenCV: https://github.com/opencv/opencv
.. _ogles_gpgpu: https://github.com/hunter-packages/ogles_gpgpu
.. _Hunter: https://github.com/ruslo/hunter
.. _Fast Feature Pyramids for Object Detection: https://pdollar.github.io/files/papers/DollarPAMI14pyramids.pdf 
.. _Piotr's Matlab Toolbox: https://pdollar.github.io/toolbox for mobile friendly object detection

.. |TravisCI| image:: https://img.shields.io/travis/elucideye/acf/master.svg?style=flat-square&label=Linux%20OSX%20Android%20iOS
  :target: https://travis-ci.org/elucideye/acf/builds

.. |Appveyor| image:: https://img.shields.io/appveyor/ci/headupinclouds/acf.svg?style=flat-square&label=Windows
  :target: https://ci.appveyor.com/project/headupinclouds/acf

.. |License| image:: https://img.shields.io/badge/license-BSD%203--Clause-brightgreen.svg?style=flat-square
  :target: http://opensource.org/licenses/BSD-3-Clause
  
.. |Hunter| image:: https://img.shields.io/badge/hunter-v0.19.107-blue.svg
  :target: http://github.com/ruslo/hunter

===========
Quick Start
===========

ACF is a `CMake <https://github.com/kitware/CMake>`__ based project
that uses the `Hunter <https://github.com/ruslo/hunter>`__ package
manager to download and build project dependencies from source as
needed. Hunter contains `detailed
documentation <https://docs.hunter.sh/en/latest>`__, but a few high
level notes and documentation links are provided here to help orient
first time users. In practice, some working knowledge of CMake may also
be required. Hunter itself is written in CMake, and is installed as part
of the build process from a single ``HunterGate()`` macro at the top of
the root ``CMakeLists.txt`` file (typically
``cmake/Hunter/HunterGate.cmake``) (you don't have to build or install
it). Each CMake dependency's ``find_package(FOO)`` call that is paired
with a ``hunter_add_package(FOO CONFIG REQUIRED)`` will be managed by
Hunter. In most cases, the only system requirement for building a Hunter
project is a recent `CMake with
CURL <https://docs.hunter.sh/en/latest/contributing.html#reporting-bugs>`__
support and a working compiler correpsonding to the operative toolchain.
Hunter will maintain all dependencies in a
`versioned <https://docs.hunter.sh/en/latest/overview/customization.html>`__
local
`cache <https://docs.hunter.sh/en/latest/overview/shareable.html>`__ by
default (typically ``${HOME}/.hunter``) where they can be reused in
subsequent builds and shared between different projects. They can also
be stored in a server side `binary
cache <https://docs.hunter.sh/en/latest/overview/binaries.html>`__ --
select `toolchains <#Toolchains>`__ will be backed by a server side
binary cache (https://github.com/elucideye/hunter-cache) and will
produce faster first time builds (use them if you can!).

The
`Travis <https://github.com/elucideye/drishti/blob/master/.travis.yml>`__
(Linux/OSX/iOS/Android) and
`Appveyor <https://github.com/elucideye/drishti/blob/master/appveyor.yml>`__
(Windows) CI scripts in the project's root directory can serve as a
reference for basic setup when building from source. To support cross
platform builds and testing, the CI scripts make use of
`Polly <https://github.com/ruslo/polly>`__: a set of common CMake
toolchains paired with a simple ``polly.py`` CMake build script. Polly
is used here for convenience to generate ``CMake`` command line
invocations -- it is not required for building Hunter projects.

To reproduce the CI builds on a local host, the following setup is
recommended:

-  Install compiler:
   http://cgold.readthedocs.io/en/latest/first-step.html
-  Install `CMake <https://github.com/kitware/CMake>`__ (and add to
   ``PATH``)
-  Install Python (for Polly)
-  Clone `Polly <https://github.com/ruslo/polly>`__ and add
   ``<polly>/bin`` to ``PATH``

Note: Polly is not a build requirement, CMake can always be used
directly, but it is used here for convenience.

The ``bin/hunter_env.{sh,cmd}`` scripts (used in the CI builds) can be
used as a fast shortcut to install these tools for you. You may want to
add the ``PATH`` variables permanently to your ``.bashrc`` file (or
equivalent) for future sessions.

+--------------------------------+--------------------------+
| Linux/OSX/Android/iOS          | Windows                  |
+================================+==========================+
| ``source bin/hunter_env.sh``   | ``bin\hunter_env.cmd``   |
+--------------------------------+--------------------------+

After the environment is configured, you can build for any supported
``Polly`` toolchain (see ``polly.py --help``) with a command like this:

::

    polly.py --toolchain ${TOOLCHAIN} --config-all ${CONFIG} --install --verbose

========
Training
========

To train your own model, you can use Piotr's Toolbox.  This currently requires Matlab until the training code gets ported.  There are a few existing samples for pedestrian detection applications that are well documented and can be modified for your application.  See `acfDemoCal.m <https://github.com/pdollar/toolbox/blob/master/detector/acfDemoCal.m>`__ for a sample training script.

===========
Integration 
===========

If you would like to integrate the library in another project, the easiest thing will be to use `Hunter <http://github.com/ruslo/hunter>`__ to manage and build your application or SDK.  The acf library and all dependencies will then be managed automatically.  Please see the documentation in the above link for more details.  If this isn't an option, it will be easiest to build ACF as a single shared library (without dependencies) that can then be integrated in your project.  In the later case, you can pass `ACF_BUILD_SHARED_SDK=ON` on the command line while generating the project in order to build the ACF library as a shared library such that all dependencies will be compiled as static libraries and "absorbed" by the acf library.  

::

    polly.py --toolchain ${TOOLCHAIN} --config-all ${CONFIG} --fwd ACF_BUILD_SHARED_SDK=ON --install --verbose

For iOS, you can use `polly` to create a dynamic framework from the generated `libacf.dylib` (see command line options) as a post build step, note the additional `--framework` and `--framework-lib` options in the build command below:

::

    polly.py --toolchain ${TOOLCHAIN} --config-all ${CONFIG} --fwd ACF_BUILD_SHARED_SDK=ON --install --verbose --framework --framework-lib libacf.dylib
    
The resulting framework will be generated in the `_framework` directory as shown below:    
   
::

    tree _framework/
    _framework/
    └── ios-11-3-dep-9-0-arm64
        └── acf.framework
            ├── Headers
            │   ├── ACF.h
            │   ├── ACFField.h
            │   ├── GPUACF.h
            │   ├── MatP.h
            │   ├── ObjectDetector.h
            │   ├── acf_common.h
            │   └── acf_export.h
            ├── Info.plist
            ├── _CodeSignature
            │   └── CodeResources
            └── acf

=====
HOWTO
=====

::

    _install/
    └── libcxx
        ├── bin
        │   ├── acf-detect
        │   └── acf-mat2cpb
        ├── include
        │   └── acf
        │       ├── ACF.h
        │       ├── ACFField.h
        │       ├── GPUACF.h
        │       ├── MatP.h
        │       ├── ObjectDetector.h
        │       ├── acf_common.h
        │       └── acf_export.h
        └── lib
             ├── cmake
             │   └── acf
             │       ├── acfConfig.cmake
             │       ├── acfConfigVersion.cmake
             │       ├── acfTargets-release.cmake
             │       └── acfTargets.cmake
             └── libacf.a
             
.. code-block:: bash

   $ cd _install/${TOOLCHAIN}/bin
   $ wget https://github.com/elucideye/drishti-assets/raw/master/drishti_face_gray_80x80.cpb
   $ wget https://github.com/elucideye/drishti-faces/raw/master/lena512color.png
   $ ./acf-detect --input=lena512color.png --output=/tmp/ --model=drishti_face_gray_80x80.cpb --nms --annotate --calibration=0.00001
   
:: 

    [16:56:34.092 | thread:8703967691101883897 | acf-detect | info]: 1/1 /Users/dhirvonen/devel/elucideye//drishti-faces/lena512color.png = 1; score = 26.0038
    
============
Contributors
============

This C++/OpenGL adaptation of the original ACF/toolbox project has benefited from contributions by:

* Ruslan Baratov @ruslo: Numerous CMake and CI contributions, and of course, `Hunter <http://github.com/ruslo/hunter>`__
* @JN-Jones: Several fixes where the C++ didn't match the matlab reference: `#67 <https://github.com/elucideye/acf/issues/67>`__,  `#62 <https://github.com/elucideye/acf/issues/62>`__
