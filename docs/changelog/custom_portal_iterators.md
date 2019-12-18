# Portals may advertise custom iterators

The `ArrayPortalToIterator` utilities are used to produce STL-style iterators 
from vtk-m's `ArrayHandle` portals. By default, a facade class is constructed 
around the portal API, adapting it to an iterator interface. 

However, some portals use iterators internally, or may be able to construct a
lightweight iterator easily. For these, it is preferable to directly use the
specialized iterators instead of going through the generic facade. A portal may
now declare the following optional API to advertise that it has custom 
iterators:

```
struct MyPortal
{
  using IteratorType = ...; // alias to the portal's specialized iterator type
  IteratorType GetIteratorBegin(); // Return the begin iterator
  IteratorType GetIteratorEnd(); // Return the end iterator

  // ...rest of ArrayPortal API...
};
```

If these members are present, `ArrayPortalToIterators` will forward the portal's
specialized iterators instead of constructing a facade. This works when using 
the `ArrayPortalToIterators` class directly, and also with the
`ArrayPortalToIteratorBegin` and `ArrayPortalToIteratorEnd` convenience 
functions.
