# Fix setting fields to pass in `Filter` when setting mode

The `Filter` class has several version of the `SetFieldsToPass` method that
works in conjunction with the `FieldSelection` object to specify which
fields are mapped. For example, the user might have code like this to pass
all fields except those named `pointvar` and `cellvar`:

``` cpp
    filter.SetFieldsToPass({ "pointvar", "cellvar" },
                           vtkm::filter::FieldSelection::Mode::Exclude);
```

This previously worked by implicitly creating a `FieldSelection` object
using the `std::initializer_list` filled with the 2 strings. This would
then be passed to the `SetFieldsToPass` method, which would capture the
`FieldSelection` object and change the mode.

This stopped working in a recent change to `FieldSelection` where each
entry is given its own mode. With this new class, the `FieldSelection`
constructor would capture each field in the default mode (`Select`) and
then change the default mode to `Exclude`. However, the already set modes
kept their `Select` status, which is not what is intended.

This behavior is fixed by adding `SetFieldToPass` overloads that capture
both the `initializer_list` and the `Mode` and then constructs the
`FieldSelection` correctly.
