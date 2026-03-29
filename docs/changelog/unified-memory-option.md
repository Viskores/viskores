## Add configuration option for unified memory

A command line option for `--viskores-use-unified-memory` has been added. This
command line option allows you to more easily select whether a device attempts
to use unified memory or not (assuming that it supports that). The option can
likewise be set with the `VISKORES_USE_UNIFIED_MEMORY` environment variable.

Unified memory has also been turned off by default. This is because under at
least some circumstances unified memory causes a performance degredation over
explicit memory management.
