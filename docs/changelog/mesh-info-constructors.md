# Constructors for mesh info classes updated to conform with other filters

The `CellMeasures` and `MeshQuality` filters had constructors that took the
metric that the filter should generate. However, this is different than the
iterface of the rest of the filters. To make the interface more consistent,
these filters now have a default (no argument) constructor, and the metric
to compute is selected via a method. This makes it more clear what is being
done.

In addition, the documentation for these two classes is updated.
