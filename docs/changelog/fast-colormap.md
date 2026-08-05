## Added the Fast colormap

The "Fast" colormap is a redesign of the cool-to-warm colormap used as the
default for products like ParaView. It improves on some of the shortcomings of
the original colormap by extending the perceivable range and introducing more
hues.

![](./Fast.png)

You can now load `Fast` as a preset with `viskores::cont::ColorTable`.

The Fast colormap was first proposed in:

> Francesca Samsel, W. Alan Scott, and Kenneth Moreland.
> **A New Default Colormap for ParaView**.
> _IEEE Computer Graphics and Applications_, 44(4):150–160, July 2024.
> [doi:10.1109/MCG.2024.3383137](https://dx.doi.org/10.1109/MCG.2024.3383137).
