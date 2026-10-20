## ANARI device keeps primitive attributes on triangle geometry

The Viskores ANARI device dropped the `primitive.color` and
`primitive.attributeN` arrays of `triangle` geometry, because it reset the
geometry's data set after adding them. A sampler that read a primitive
attribute of a triangle then found nothing. Triangle geometry now adds them
the same way quad and cylinder geometry do.
