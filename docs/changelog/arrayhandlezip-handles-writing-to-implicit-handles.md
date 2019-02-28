# vtkm::cont::ArrayHandleZip provides a consistent API even with non-writable handles

Previously ArrayHandleZip could not wrap an implicit handle and provide a consistent experience.
The primary issue was that if you tried to use the PortalType returned by GetPortalControl() you
would get a compile failure. This would occur as the PortalType returned would try to call `Set`
on an ImplicitPortal which doesn't have a set method. 

Now with this change, the `ZipPortal` use SFINAE to determine if `Set` and `Get` should call the
underlying zipped portals.
