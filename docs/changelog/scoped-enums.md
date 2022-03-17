# Favor scoped `enum`s

Several `enum` declarations were changed from a standard `enum` to a
"scoped" `enum` (i.e. `enum struct`). The advantage of a scoped enum is
that they provide better type safety because they won't be converted
willy-nilly to other types. They also prevent the names they define from
being accessible on the inner scope.

There are some cases where you do want the `enum` to convert to other types
(but still want the scope of the symbols to be contained in the `enum`
type). In this case, we worked around the problem by placing an unscoped
`enum` inside of a standard `struct`.
