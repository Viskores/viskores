# Make field names from sources more descriptive

The VTK-m sources (like `Oscillator`, `Tangle`, and `Wavelet`) were all
creating fields with very generic names like `pointvar` or `scalars`. These
are very unhelpful names as it is impossible for downstream processes to
identify the meaning of these fields. Imagine having these data saved to a
file and then a different person trying to identify what they mean. Or
imagine dealing with more than one such source at a time and trying to
manage fields with similar or overlapping names.

The following renames happened:

  * `Oscillator`: `scalars` -> `oscillating`
  * `Tangle`: `pointvar` -> `tangle`
  * `Wavelet`: `scalars` -> `RTData` (matches VTK source)

