## Use id-only block lookup for flow bounds

Flow filters now build an internal block-bounds locator that can identify all
blocks containing a particle position using cell-id-only locator queries. This
provides the foundation for more efficient worklet-based particle routing across
overlapping blocks.
