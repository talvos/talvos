Plugin interface
================
.. highlight:: c++

Talvos provides a simple plugin interface to enable a wide range of dynamic
analysis tools to be created.
This interface provides a callback mechanism by which a Talvos plugin can
receive notifications when various events occur during emulation.

A Talvos plugin should be created as a dynamic library. During initialization,
Talvos will load any libraries listed in the ``TALVOS_PLUGINS`` environment
variable (semicolon separated).
These libraries should each contain an implementation of a class which extends
the ``talvos::Plugin`` base class, overriding any callback methods that are
interesting to the plugin.
See the `documentation for the Plugin class
<https://talvos.github.io/api/classtalvos_1_1_plugin.html>`_ for a full list of
available callback methods.

A Talvos plugin library should also provide a function with the following
signature:
::

  extern "C"
  {
    Plugin *talvosCreatePlugin(talvos::Device *Dev);
  }

This function will be called when a Talvos device is created, and should return
an instance of the plugin's derived ``Plugin`` class.
Additionally, a plugin library can implement the following cleanup function
which is called when a Talvos device is destroyed:
::

  extern "C"
  {
    void talvosDestroyPlugin(talvos::Plugin *P);
  }

If a Plugin is not thread-safe, it should indicate this by overriding the
``isThreadSafe()`` function and returning ``false``.


Example (instruction tracing)
-----------------------------

The following code listing is a complete example of a Talvos plugin that prints
each instruction executed during emulation, along with the global invocation ID.
::

  #include <iostream>

  #include "talvos/Device.h"
  #include "talvos/Instruction.h"
  #include "talvos/Invocation.h"
  #include "talvos/Plugin.h"

  using namespace talvos;

  class Tracer : public Plugin
  {
  public:
    void instructionExecuted(const Invocation *Invoc,
                             const Instruction *Inst) override
    {
      std::cout << Invoc->getGlobalId() << ": ";
      Inst->print(std::cout);
      std::cout << std::endl;
    }
  };

  extern "C"
  {
    Plugin *talvosCreatePlugin(Device *Device)
    {
      return new Tracer;
    }

    void talvosDestroyPlugin(Plugin *P)
    {
      delete P;
    }
  }

On a Unix system, this is compiled with the following command (assuming that
the Talvos `include/` and `lib/` directories are in the relevant search paths):

.. highlight:: bash

::

  c++ -std=c++14 tracer.cpp -shared -o libtracer.so -ltalvos

Emulating a simple vector addition shader with this plugin loaded provides the
following output:
::

  $ TALVOS_PLUGINS=libtracer.so talvos-cmd vecadd.tcf
  (0,0,0):   %24 = OpAccessChain %19 %2 %21
  (0,0,0):   %25 = OpLoad %12 %24
  (0,0,0):   %26 = OpAccessChain %13 %9 %21 %25
  (0,0,0):   %27 = OpLoad %12 %26
  (0,0,0):   %28 = OpAccessChain %13 %10 %21 %25
  (0,0,0):   %29 = OpLoad %12 %28
  (0,0,0):   %30 = OpIAdd %12 %29 %27
  (0,0,0):   %31 = OpAccessChain %13 %11 %21 %25
  (0,0,0):         OpStore %31 %30
  (0,0,0):         OpReturn
  (1,0,0):   %24 = OpAccessChain %19 %2 %21
  (1,0,0):   %25 = OpLoad %12 %24
  (1,0,0):   %26 = OpAccessChain %13 %9 %21 %25
  (1,0,0):   %27 = OpLoad %12 %26
  (1,0,0):   %28 = OpAccessChain %13 %10 %21 %25
  (1,0,0):   %29 = OpLoad %12 %28
  (1,0,0):   %30 = OpIAdd %12 %29 %27
  (1,0,0):   %31 = OpAccessChain %13 %11 %21 %25
  (1,0,0):         OpStore %31 %30
  (1,0,0):         OpReturn
  # etc
