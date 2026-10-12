## Fix CleanGrid degenerate 3D cell removal

CleanGrid now checks each face of 3D cells when removing degenerate cells. This
prevents collapsed 3D cells from being kept just because the full cell still has
enough unique point ids overall.

The CleanGrid unit test also covers more edge cases for unused point compaction,
point merging, tolerance modes, fast merge behavior, and active coordinate
system selection.
