# Rename NewFilter base classes to Filter

During the VTK-m 1.8 and 1.9 development, the filter infrastructure was
overhauled. Part of this created a completely new set of base classes. To
avoid confusion with the original filter base classes and ease transition,
the new filter base classes were named `NewFilter*`. Eventually after all
filters were transitioned, the old filter base classes were deprecated.

With the release of VTK-m 2.0, the old filter base classes are removed. The
"new" filter base classes are no longer new. Thus, they have been renamed
simply `Filter` (and `FilterField`).
