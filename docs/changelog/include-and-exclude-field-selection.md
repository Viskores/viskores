# Allow FieldSelection to simultaneously include and exclude fields

The basic use of `FieldSelection` is to construct the class with a mode
(`None`, `Any`, `Select`, `Exclude`), and then specify particular fields
based off of this mode. This works fine for basic uses where the same code
that constructs a `FieldSelection` sets all the fields.

But what happens, for example, if you have code that takes an existing
`FieldSelection` and wants to exclude the field named `foo`? If the
`FieldSelection` mode happens to be anything other than `Exclude`, the code
would have to go through several hoops to construct a new `FieldSelection`
object with this modified selection.

To make this case easier, `FieldSelection` now has the ability to specify
the mode independently for each field. The `AddField` method now has an
optional mode argument the specifies whether the mode for that field should
be `Select` or `Exclude`.

In the example above, the code can simply add the `foo` field with the
`Exclude` mode. Regardless of whatever state the `FieldSelection` was in
before, it will now report the `foo` field as not selected.
