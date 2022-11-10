# Attach compressed ZFP data as WholeDatSet field

Previously, point fields compressed by ZFP were attached as point fields
on the output. However, using them as a point field would cause
problems. So, instead attache them as `WholeDataSet` fields.

Also fixed a problem where the 1D decompressor created an output of the
wrong size.
