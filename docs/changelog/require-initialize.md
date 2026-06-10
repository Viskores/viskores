## Viskores now requires initialization

It is now required to call `viskores::cont::Initialize` before using other
features in Viskores. This change has come about to ensure that resources are
managed correctly.

A new function, `viskores::cont::IsInitialized` has also been added to help
client code ensure `Initialize` is called exactly once.
