Building Talvos
===============

Talvos is straightforward to build, and supports Linux, macOS, and Windows.
Continuous integration is in place to ensure that the latest source available
on GitHub builds and passes the internal test suite on each of these platforms.
Prebuilt binaries for releases are provided on the `GitHub releases page
<https://github.com/talvos/talvos/releases>`_.

Talvos requires a compiler that supports C++17.
The following compilers/platforms are frequently tested via continuous
integration (Travis CI and AppVeyor):

- GCC 7 (Ubuntu 14.04)
- GCC 9 (Ubuntu 14.04)
- Clang 5.0 (Ubuntu 14.04)
- Clang 8.0 (Ubuntu 14.04)
- AppleClang 9.1 (macOS 10.13 / Xcode 9.4)
- Visual Studio 2017 (x64)

Building Talvos as a 32-bit library is not currently tested.


Dependencies
------------

Talvos has two required dependencies, SPIRV-Headers and SPIRV-Tools.

SPIRV-Headers can be obtained by simply cloning `the GitHub repository
<https://github.com/KhronosGroup/SPIRV-Headers>`_.

Prebuilt binaries for SPIRV-Tools are available on `its GitHub releases page
<https://github.com/KhronosGroup/SPIRV-Tools/releases>`_.
The continuous integration platforms listed above all use the latest
``master-tot`` binaries.
Alternatively, it can be built from source following the instructions provided
in the SPIRV-Tools README.

For Unix systems, it is recommended to have GNU readline available in order for
the interactive debugger to provide a command history and other common shell
keyboard shortcuts.
On Linux the ``libreadline-dev`` (Debian) or ``readline-devel`` (RPM) packages
provide the necessary development files.


CMake
-----

Configuration is performed via CMake (2.8.12 or newer required).

The following cache variables are used to inform Talvos of the location of the
above dependencies, if necessary:

- ``SPIRV_INCLUDE_DIR``
  - the directory containing ``spirv/unified1/spirv.h``
- ``SPIRV_TOOLS_INCLUDE_DIR``
  - the directory containing ``spirv-tools/libspirv.hpp``
- ``SPIRV_TOOLS_LIBRARY_DIR``
  - the directory containing ``libSPIRV-Tools.a`` (Unix) or ``SPIRV-Tools.lib`` (Windows)

Use ``CMAKE_INSTALL_PREFIX`` to set the target installation directory.

While Talvos is still in the early stages of development, it is recommended to
set ``CMAKE_BUILD_TYPE`` to ``Debug`` to enable assertions.

A typical CMake command line might look like this:
::

  cmake <path_to_talvos_source>                           \
        -DCMAKE_BUILD_TYPE=Debug                          \
        -DCMAKE_INSTALL_PREFIX=<target_install_directory> \
        -DSPIRV_INCLUDE_DIR=<...>                         \
        -DSPIRV_TOOLS_INCLUDE_DIR=<...>                   \
        -DSPIRV_TOOLS_LIBRARY_DIR=<...>

For Visual Studio pass ``-G "Visual Studio 15 2017 Win64"`` to select the
64-bit version of Visual Studio 2017.

To use Ninja instead of GNU Make on Unix pass ``-G Ninja``, and replace ``make``
with ``ninja`` for the commands in the next section.


Building
--------

Once configured, build by typing ``make`` or using the ``ALL_BUILD`` project
in Visual Studio.

After building, run ``make test`` or use the ``RUN_TESTS`` project in
Visual Studio to run the test suite.
If you experience test failures with a compiler/platform configuration that you
expect to work, please open a `GitHub issue
<https:/github.com/talvos/talvos/issues>`_.

Finally, use ``make install`` or the ``INSTALL`` project in Visual Studio to
install Talvos if desired.
