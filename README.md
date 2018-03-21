# Talvos

This project provides a SPIR-V interpreter and Vulkan device emulator, with the
aim of providing an extensible dynamic analysis framework and debugger for
SPIR-V shaders.
Talvos provides an implementation of the Vulkan API to enable it to execute
real Vulkan applications.

Talvos is distributed under a three-clause BSD license. For full license
terms please see the LICENSE file distributed with this source code.


## Status

Talvos is still in the early stages of development.
Compute shaders are the current focus, and Talvos is currently capable of
executing various SPIR-V shaders generated from OpenCL kernels compiled with
[Clspv](https://github.com/google/clspv).
Linux, macOS, and Windows are supported.

The codebase is changing relatively quickly as new features are added, so the
internal and external APIs are all subject to change until we reach the 1.0
release (suggestions for improvements always welcome).

Future work may involve extending the emulator to support vertex and fragment
shaders, and implementing missing Vulkan API functions to reach conformance.
Contributions in these (or other) areas would be extremely welcome.


## Building

More detailed build instructions are provided
[here](https://talvos.github.io/building.html).

Building Talvos requires a compiler that supports C++14, and Python to enable
the internal test suite.
Talvos depends on
[SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers) and
[SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools), and uses GNU
readline to enhance the interactive debugger.

Configure with CMake, indicating where SPIRV-Headers and SPIRV-Tools are
installed if necessary:

    cmake <path_to_talvos_source>                           \
          -DCMAKE_BUILD_TYPE=Debug                          \
          -DCMAKE_INSTALL_PREFIX=<target_install_directory> \
          -DSPIRV_INCLUDE_DIR=<...>                         \
          -DSPIRV_TOOLS_INCLUDE_DIR=<...>                   \
          -DSPIRV_TOOLS_LIBRARY_DIR=<...>

Using the `Debug` build type is strongly recommended while Talvos is still in
the early stages of development, to enable assertions that guard unimplemented
features.

Once configured, use `make` (or your preferred build tool) to build and
install. Run `make test` to run the internal test suite.


## Usage

More detailed usage information is provided
[here](https://talvos.github.io/usage.html).

Talvos provides an implementation of the Vulkan API which allows existing
Vulkan applications to be executed through the emulator without modification.
Simply linking an application against `libtalvos-vulkan.so` or
`talvos-vulkan.lib` is enough for it to use Talvos.

Alternatively, the `talvos-cmd` command provides a simple interface to the
emulator.
An example that runs a simple N-Body simulation can be found in
`test/misc/nbody.tcf`.

The following command is used to execute a Talvos command file:

    talvos-cmd nbody.tcf

The command file contains commands that allocate and initialize storage buffers
and launch SPIR-V shaders.
There are also commands that dump the output of storage buffers, to verify
that the shader executed as expected.

To enable the interactive debugger, set the environment variable
`TALVOS_INTERACTIVE=1`.
This will drop to a prompt when a shader begins executing.
Use `help` to list the available commands, which include `step` to advance
through the interpreter, `print` to view an instruction result, and `switch` to
change the current invocation.


## More information

Raise issues and ask questions via
[GitHub](https://github.com/talvos/talvos/issues).

The [Talvos documentation](https://talvos.github.io) provides more detailed
information about using Talvos, as well as information for developers.

[Doxygen documentation](https://talvos.github.io/api) for the source code is
also available.
