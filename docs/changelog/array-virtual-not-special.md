# Make ArrayHandleVirtual conform with other ArrayHandle structure

Previously, ArrayHandleVirtual was defined as a specialization of
ArrayHandle with the virtual storage tag. This was because the storage
object was polymorphic and needed to be handled special. These changes
moved the existing storage definition to an internal class, and then
managed the pointer to that implementation class in a Storage object that
can be managed like any other storage object.
    
Also moved the implementation of StorageAny into the implementation of the
internal storage object.
