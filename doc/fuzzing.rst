Fuzzing Wakaama Code using libFuzzer
====================================

Wakaama code can be fuzzed with the use of ``clang``'s `libFuzzer <https://llvm.org/docs/LibFuzzer.html>`__.

Adding a Fuzzing Test
---------------------

Create a subfolder in ``fuzzing``. There add a ``CMakeLists.txt`` where
the fuzzing test is added.

.. code-block:: cmake
   :caption: fuzzing/my_fuzzing_test/CMakeLists.txt

   add_fuzzing_test(TARGET_NAME fuz_my_fuzzing_test SOURCE_FILES fuz_my_fuzzing_test.c)

Implement the fuzzing test:

.. code-block:: c
   :caption: fuzzing/my_fuzzing_test/my_fuzz_target.c

   int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
      // do something here with the generated test data
      return 0;
   }

In the main CMake file in ``fuzzing`` add the new test
.. code-block:: cmake
   :caption: fuzzing/my_fuzzing_test/CMakeLists.txt

.. code-block:: cmake
   :caption: fuzzing/CMakeLists.txt

   cmake_minimum_required(VERSION 3.13)

   project(fuzzing_tests C)

   include(Fuzzing.cmake)

   # ...
   add_subdirectory(my_fuzzing_test)


Running a fuzzing Test
----------------------

Build and run the fuzzing test from the project root directory.

.. code-block:: sh
   cmake -DCMAKE_C_COMPILER=clang -S fuzzing -B build-fuzzing
   cmake --build build-fuzzing --target run_my_fuzz_target

This will run the test for three minutes.

Regression Tests
----------------

If a bug is found by a fuzzing test, the provided input is saved
as a file. It's possible to run a test with such a file. This
helps in debugging the code and to provide a regression test.

Move the generated file(s) to an appropriate folder. E.g.
``fuzzing/my_fuzzing_test/crash_files`` and add that folder in
the ``CMakeLists.txt``.

.. code-block:: cmake
   :caption: fuzzing/my_fuzzing_test/CMakeLists.txt

   add_fuzzing_test(TARGET_NAME my_fuzz_target
      SOURCE_FILES my_fuzz_target.c
      CRASH_FILES_DIR crash_files/)

Run the regression test:

.. code-block:: sh
   cmake -DCMAKE_C_COMPILER=clang -S fuzzing -B build-fuzzing
   cmake --build build-fuzzing --target run_my_fuzz_target_fuzzing_reg_test

The fuzzing regression tests are also executed as part of the CI.
