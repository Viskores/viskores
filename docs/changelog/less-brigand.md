# Remove brigand from List.h

Early in the development of VTK-m, a third party header library named
brigand was incorporated into the source code. This header file contained
the implementation of several complex meta-template programming constructs
that are useful. Furthermore, it is implemented in such a way as to compile
(relatively) quickly.

However, lately we have run into problems with brigand. First, it is not a
very active project so that it is hard to submit fixes back to the project.
Of the activity that is there, the most recent version of brigand now
requires C++17, which is not directly supported by VTK-m. Second, brigand
was added before the thridparty directory was established. This means as we
have added corrections to the brigand source code, they have not been
properly marked up in git to allow us to easily bring in changes from the
main repo. On top of all that, because of the complexity of brigand, we
often run into problems with compilers that fail in corner cases, which
makes it difficult to support.

We have already moved away from brigand quite a bit. This takes another big
step closer to removing our independence on brigand by no longer requiring
it for any of the implementation of `vtkm::List`.

Because brigand.hpp is no longer included in List.h, some uses of the
brigand header have been replaced with the implementation in List.h.
