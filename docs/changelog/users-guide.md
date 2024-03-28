# Added VTK-m's user guide into the source code

The VTK-m User's Guide is being transitioned into the VTK-m source code.
The implementation of the guide is being converted from LaTeX to
ReStructuredText text to be built by Sphinx. There are several goals of
this change.

1. Integrate the documentation into the source code better to better
   keep the code up to date.
2. Move the documentation over to Sphinx so that it can be posted online
   and be more easily linked.
3. Incoporate Doxygen into the guide to keep the documentation
   consistent.
4. Build the user guide examples as part of the VTK-m CI to catch
   compatibility changes quickly.
