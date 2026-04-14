## Support skipping tests

Viskores tests can now skip themselves. Sometimes it is not possible to
determine whether a particular test is supported until the test is run and the
system can be introspected. In this case, the test can now return by invoking
the `VISKORES_TEST_SKIP`. In this case, the test will immediately end, and CTest
will report that the test was skipped rather than pass or fail.
