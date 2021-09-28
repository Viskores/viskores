# Support scatter/mask for CellSetExtrude

Scheduling topology map workets for `CellSetExtrude` always worked, but the
there were indexing problems when a `Scatter` or a `Mask` was used. This
has been corrected, and now `Scatter`s and `Mask`s are supported on
topology maps on `CellSetExtrude`.
