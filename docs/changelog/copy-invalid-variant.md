# Fix bug with copying invalid variants

There was a bug where if you attempted to copy a `Variant` that was not
valid (i.e. did not hold an object); a seg fault could happen. This has
been changed to set the target variant to also be invalid.
