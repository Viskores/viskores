# Added modules to the build system

VTK-m libraries and other targets can now be built as modules. The
advantage of modules is that you can selectively choose which
modules/libraries will be built. This makes it easy to create a more
stripped down compile of VTK-m. For example, you might want a reduced set
of libraries to save memory or you might want to turn off certain libraries
to save compile time.

The module system will automatically determine dependencies among the
modules. It is capable of weakly turning off a module where it will still
be compiled if needed. Likewise, it is capabile of weakly turning on a
module where the build will still work if it cannot be created.

The use of modules is described in the `Modules.md` file in the `docs`
directory of the VTK-m source.
