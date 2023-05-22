# ComputeMoments filter now operates on any scalar field

Previously, the `ComputeMoments` filter only operated on a finite set of
array types as its input field. This included a prescribed list of `Vec`
sizes for the input. The filter has been updated to use more generic
interfaces to the field's array (and float fallback) to enable the
computation of moments on any type of scalar field.
