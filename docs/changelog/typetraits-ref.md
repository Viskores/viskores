## Support TypeTraits for reference types

Viskores now supports passing reference types to `viskores::TypeTraits`.
Previously, if you used `viskores::TypeTraits` with a reference type (such as
`viskores::TypeTraits<viskores::Id&>`) you would get the "default"
implementation, which would state that the type was unknown.

Now when you use `viskores::TypeTraits` with a reference type, you get the
traits for the type without the reference. For example, if you use

```cpp
viskores::TypeTraits<T&>
```

you will get the same traits as

```cpp
viskores::TypeTraits<T>
```

This feature is important when implementing templated methods where a templated
type can be a reference. This avoids having to remove references from templated
types.
