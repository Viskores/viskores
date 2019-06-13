# VariantArrayHandle::AsVirtual<T>() performs casting

The AsVirtual<T> method of VariantArrayHandle now works for any arithmetic type,
not just the actual type of the underlying array. This works by inserting an
ArrayHandleCast between the underlying concrete array and the new
ArrayHandleVirtual when needed.
