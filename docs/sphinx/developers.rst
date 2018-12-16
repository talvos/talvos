Talvos developer information
============================

Contributions in the form of GitHub pull requests are always welcome.
Pull requests will be reviewed carefully, and changes may be requested if they
do not adhere to the guidelines below.


Coding style
------------

New contributions should always adhere to the Talvos coding style.
It should generally be possible to glean the coding style from the existing
code, but there is also a ``.clang_format`` file which helps automate the
process of checking the style.
The recommendation is to run ``git-clang-format`` after staging changes to
ensure the changes will match the Talvos coding style.

Code should be well commented.
New class and method declarations should use Doxygen ``///`` syntax to allow
documentation to be generated automatically.


Tests
-----
Talvos has an internal test suite that should be run before any pull request is
opened to ensure that there are no regressions.
Changes that add new functionality or fix serious bugs should almost always be
accompanied by new tests (or additions to existing tests where appropriate).

There are Travis CI and AppVeyor configurations that will test that each commit
and pull request builds and passes the test suite for multiple platforms
and compilers.

In addition, Talvos is run through the `Vulkan Conformance Test Suite
<https://github.com/KhronosGroup/VK-GL-CTS>`_ daily, via CircleCI.
The results are stored in the `talvos-vulkan-cts GitHub repository
<https://github.com/talvos/talvos-vulkan-cts>`_.
Regressions in the Vulkan CTS will produce a failed CircleCI build for that
repository.


Other information
-----------------
Commits should be atomic where possible, with `meaningful commit messages
<https://chris.beams.io/posts/git-commit/#seven-rules>`_.

First-time contributors should add their name to the
`AUTHORS file <https://github.com/talvos/talvos/tree/master/AUTHORS>`_.

Doxygen documentation can be found `here <https://talvos.github.io/api>`_.

Significant new features should generally also be accompanied by Sphinx
documentation.

Talvos uses a `semantic version numbering scheme <https://semver.org/>`_.
