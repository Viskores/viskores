## Clip: Fix cellOffset increment

There was bug in the newly redesigned Clip filter that caused the cellOffset to not be incremented,
cause output cells to be incorrectly placed in the output. This has now been fixed, and the output.

Additionally, all resources are released as soon as possible.

Finally, the ScanExclusive usages within the Clip filter have been improved to operate
only on the batches of points/cells that are actually needed, rather than the entire batch data.
