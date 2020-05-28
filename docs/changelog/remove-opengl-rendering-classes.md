# Removed OpenGL Rendering Classes

When the rendering library was first built, OpenGL was used to implement
the components (windows, mappers, annotation, etc.). However, as the native
ray casting became viable, the majority of the work has focused on using
that. Since then, the original OpenGL classes have been largely ignored.

It has for many months been determined that it is not work attempting to
maintain two different versions of the rendering libraries as features are
added and changed. Thus, the OpenGL classes have fallen out of date and did
not actually work.

These classes have finally been officially removed.
