# Update clip filter's field map to work on any field type

The previous implementation of the map field in the clip filters
(`ClipWithField` and `ClipWithImplicitFunction`) checked for common field
types and interpolated those. If the field value type did not match, it
would either convert the field to floats (which is at odds with what VTK
does) or fail outright if the `Vec` length is not supported.

The map field function for clip has been changed to support all possible
types. It does this by using the extract component functionality to get
data from any type of array.
