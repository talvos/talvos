# Talvos

This project provides a SPIR-V interpreter and Vulkan device emulator, with the
aim of providing an extensible dynamic analysis framework and debugger for
SPIR-V shaders.

Talvos is distributed under a three-clause BSD license. For full license
terms please see the LICENSE file distributed with this source code.


## Status

Talvos is still in the early stages of development.
Compute shaders are the current focus, and Talvos is currently capable of
executing various SPIR-V shaders generated from OpenCL kernels compiled with
[Clspv](https://github.com/google/clspv).

A simple file format is used to load and execute SPIR-V shaders.
There is a lightweight interactive debugging interface that enables stepping
through SPIR-V instructions and printing instruction results.

The codebase is changing relatively quickly as new features are added, so the
internal and external APIs are all subject to change until we reach the 1.0
release (suggestions for improvements always welcome).

Future work may involve extending the emulator to support vertex and fragment
shaders, and adding a Vulkan runtime interface.
Contributions in these (or other) areas would be extremely welcome.


## Building

Building Talvos requires a compiler that supports C++14, and Python to enable
the internal test suite.
Talvos depends on
[SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers) and
[SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools), and uses GNU
readline to enhance the interactive debugger.

Configure with CMake, indicating where SPIRV-Headers and SPIRV-Tools are
installed if necessary:

    cmake <path_to_talvos_source>                           \
          -G Ninja                                          \
          -DCMAKE_BUILD_TYPE=Debug                          \
          -DCMAKE_INSTALL_PREFIX=<target_install_directory> \
          -DSPIRV_INCLUDE_DIR=<...>                         \
          -DSPIRV_TOOLS_INCLUDE_DIR=<...>                   \
          -DSPIRV_TOOLS_LIBRARY_DIR=<...>

Using the `Debug` build type is strongly recommended while Talvos is still in
the early stages of development, to enable assertions that guard unimplemented
features.

Once configured, use `ninja` (or your preferred build tool) to build and
install. Run `ninja test` to run the internal test suite.


## Usage

The `talvos-cmd` command provides a simple interface to the emulator.
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

[Doxygen documentation](https://talvos.github.io/doxygen/html) for the source
code is also available.
