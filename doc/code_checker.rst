Static Code analysis with CodeChecker
=====================================

Wakaama uses ```CodeChecker`` <https://codechecker.readthedocs.io/>`__
for static code analysis.

It’s possible to run ``CodeChecker`` in two different modes: ``full``
and ``diff``

In ``full`` mode all found issues are reported. In ``diff`` mode only
new issues are shown.

The ``diff`` mode compares found issues with a ‘base line’ and shows
only newly found issues.

Running ``CodeChecker``
-----------------------

The ``CodeChecker`` is run as part of the CI GitHub Actions. But it can
be run manually:

To show new issues:

::

   tools/ci/run_ci.sh --run-build --run-code-checker --code-checker diff

To show *all* issues:

::

   tools/ci/run_ci.sh --run-build --run-code-checker --code-checker full

Create new ‘base line’:

::

   tools/ci/run_ci.sh --run-build --run-code-checker --code-checker baseline

