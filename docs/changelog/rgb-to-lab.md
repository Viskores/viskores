## RGB to Lab color conversion filter

Viskores now provides `viskores::filter::field_transform::RGBToLab` to convert
RGB color fields to CIE L*a*b* color fields. The filter accepts byte RGB values
stored as `viskores::Vec3ui_8` and normalized floating-point RGB values stored
as `viskores::Vec3f_32` or `viskores::Vec3f_64`.

The generated Lab field preserves the association of the active input field and
uses the active field name with `_lab` appended when no output field name is
specified.
