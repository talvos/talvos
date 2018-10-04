.. _talvos-cmd:

Talvos command files
=====================

The ``talvos-cmd`` command consumes a Talvos command file, which uses a handful
of simple commands to describe the SPIR-V shader that should be executed.

For a complete example demonstrating several of these commands, see
:ref:`tcf-example`.
The `internal test suite <https://github.com/talvos/talvos/tree/master/test>`_
used by Talvos developers also contains many examples.

Commands
--------

Commands and arguments are case-sensitive.
Using the ``#`` character will mark the rest of that line as a comment.

Where a command takes a ``<type>`` argument, the type can be one of the
following:
::

  INT8
  INT16
  INT32
  INT64
  UINT8
  UINT16
  UINT32
  UINT64
  FLOAT
  DOUBLE


``BUFFER``
~~~~~~~~~~
::

  BUFFER <name> <size> <initializer...>

Allocate a storage buffer that is `<size>` bytes.
The identifier `<name>` can be used to refer to the allocation in
`DESCRIPTOR_SET` and `DUMP` commands.

The initializer can be one of the following:
::

  # Fill the buffer the value <value>
  FILL <type> <value>

  # Fill the buffer with a series starting at <start>, incrementing by <inc>
  SERIES <type> <start> <inc>

  # Fill the buffer with the contents of a file
  BINFILE <filename>

  # Specify values for the contents of the buffer
  DATA <type> <values...>


``DESCRIPTOR_SET``
~~~~~~~~~~~~~~~~~~
::

  DESCRIPTOR_SET <d> <b> <a> <name>

Associate the buffer ``<name>`` with descriptor set ``<d>``, binding ``<b>``,
array element ``<a>`` for the next command dispatched.


``DISPATCH``
~~~~~~~~~~~~
::

  DISPATCH <x> <y> <z>

Dispatch the current SPIR-V shader, with the three integer arguments specifying
the number of groups to launch in each dimension.


``DUMP``
~~~~~~~~~~~~
::

  DUMP <type> <name>

Dump the buffer ``<name>``, interpreting its contents using ``<type>``.
In addition to the types listed above, the command also accepts ``RAW`` to
display raw hexadecimal data.
The scalar data types can also be suffixed with ``vN`` to display the data as
vectors of length ``N``.


``ENTRY``
~~~~~~~~~~~~
::

  ENTRY <name>

Set the name of the SPIR-V entry point to be used for all subsequent dispatches.


``LOOP and ENDLOOP``
~~~~~~~~~~~~~~~~~~~~~~~~
::

  LOOP <n>
  ...
  ENDLOOP

Repeat the commands enclosed in the region ``<n>`` times.


``MODULE``
~~~~~~~~~~~~
::

  MODULE <filename>

Load a SPIR-V module from file and use it for all subsequent dispatches.
The filename provided can either be a SPIR-V binary or a human-readable
disassembled SPIR-V module produced by ``spirv-dis``.


.. _tcf-example:


``SPECIALIZE``
~~~~~~~~~~~~~~
::

  SPECIALIZE <spec_id> <type> <value>

Set the value of the specialization constant with id ``<spec_id>`` to
``<value>``, interpreting it using ``<type>``.
This affects any subsequent ``DISPATCH`` commands.
In addition to the types listed above, the command also accepts ``BOOL`` to
set the value of ``OpSpecConstantTrue`` and ``OpSpecConstantFalse``
instructions, where ``<value>`` should be ``0`` or ``1``.


Example
-------

This example runs several iterations of an N-Body simulation.
The SPIR-V assembly file that works with this example can be found
`here <https://github.com/talvos/talvos/tree/master/test/misc/nbody.spvasm>`_.

::

  MODULE nbody.spvasm
  ENTRY nbody

  # Initialize the positions using specific floating point values.
  BUFFER positionsIn   128   DATA FLOAT
    86.52     0.00   -94.33  1
     4.49  -127.48   -10.59  1
  -103.63   -21.64   -71.95  1
   114.35    34.82    45.79  1
   -27.18   -57.11   111.28  1
   -95.14    85.48     4.97  1
    22.78   -40.85  -119.15  1
   120.63    42.12     7.60  1

  # Initialize the output positions and velocities buffers with zeros.
  BUFFER positionsOut  128   FILL FLOAT 0
  BUFFER velocities    128   FILL FLOAT 0

  # Set other parameters used in the simulation.
  BUFFER numBodies     4     DATA UINT32   8
  BUFFER softening     4     DATA FLOAT  100
  BUFFER delta         4     DATA FLOAT   50

  # Set descriptor set values.
  DESCRIPTOR_SET 0 2 0 velocities
  DESCRIPTOR_SET 0 3 0 numBodies
  DESCRIPTOR_SET 0 4 0 softening
  DESCRIPTOR_SET 0 5 0 delta

  # Run the shader in a loop.
  # Each loop iteration launches the shader twice, swapping the position
  # buffers each time.
  LOOP 4
    DESCRIPTOR_SET 0 0 0 positionsIn
    DESCRIPTOR_SET 0 1 0 positionsOut
    DISPATCH 2 1 1

    DESCRIPTOR_SET 0 0 0 positionsOut
    DESCRIPTOR_SET 0 1 0 positionsIn
    DISPATCH 2 1 1
  ENDLOOP

  # Dump the final positions to stdout.
  DUMP FLOATv4 positionsIn
