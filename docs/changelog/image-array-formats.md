## Add the ability to write images from multiple color formats

Previously, the image writers only wrote color fields that were basic arrays of
value type `viskores::Vec4f_32`. This is the format provided by `Canvas`, but
there may be reason to have other types of colors. Expand the abilities of the
image writers by converting color fields when necessary. The writer now supports
32- and 64-bit floats as well as unsigned bytes for the channel. It also
supports either 3 or 4 component colors.
