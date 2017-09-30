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


=====
HOWTO
=====

.. code-block:: bash

   
   $ cd ${HOME}
   $ wget https://github.com/ruslo/polly/archive/master.zip
   $ unzip master.zip
   $ POLLY_ROOT="${PWD}/polly-master"
   $ export PATH="${POLLY_ROOT}/bin:${PATH}" # add to .profile
   $ install-ci-dependencies.py
   $ export PATH="${PWD}/_ci/cmake/bin:${PATH}" # add to .profile
   $ mkdir -p git 
   $ cd git
   $ git clone https://github.com/elucideye/acf.git
   $ cd acf
   $ git submodule update --init --recursive --quiet)
   $ polly.py --help # pick a toolchain
   $ polly.py --toolchain libcxx --install --reconfig --verbose --test
   $ ls -R _install/libcxx/
