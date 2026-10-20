## Fix ANARI array leaks and stale parameters in the ANARI volume mapper

`ANARIMapperVolume` leaked its unstructured `vertex.data`, `index`, `cell.index`
and `cell.type` arrays every time `SetActor` rebuilt the spatial field. Those
handles were also never cleared, so switching from an unstructured actor to a
structured one set the old unstructured arrays on the new `structuredRegular`
spatial field. The mapper now releases and clears every array it creates when
it rebuilds the spatial field.
