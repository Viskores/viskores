# Removed old `ArrayHandle` transfer mechanism

Deleted the default implementation of `ArrayTransfer`. `ArrayTransfer` is
used with the old `ArrayHandle` style to move data between host and device.
The new version of `ArrayHandle` does not use `ArrayTransfer` at all
because this functionality is wrapped in `Buffer` (where it can exist in a
precompiled library).

Once all the old `ArrayHandle` classes are gone, this class will be removed
completely. Although all the remaining `ArrayHandle` classes provide their
own versions of `ArrayTransfer`, they still need the prototype to be
defined to specialize. Thus, the guts of the default `ArrayTransfer` are
removed and replaced with a compile error if you try to compile it.

Also removed `ArrayManagerExecution`. This class was used indirectly by the
old `ArrayHandle`, through `ArrayHandleTransfer`, to move data to and from
a device. This functionality has been replaced in the new `ArrayHandle`s
through the `Buffer` class (which can be compiled into libraries rather
than make every translation unit compile their own template).
