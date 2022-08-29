# Add Variant::IsType

The `Variant` class was missing a way to check the type. You could do it
indirectly using `variant.GetIndex() == variant.GetIndexOf<T>()`, but
having this convenience function is more clear.
