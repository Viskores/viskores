## Bug fix for CellLocatorBoundingIntervalHierarchy

Fixes a memory error that was showing up on GPUs.

Fixed one bug where BIH could keep searching after already finding cell 0, which could cause invalid memory access. Fixed a second bug where stale cached leaf data could be reused incorrectly, leading to bad reads in the LastCell fast path.


