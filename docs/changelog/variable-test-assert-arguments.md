# Allow variable arguments to `VTKM_TEST_ASSERT`

The `VTKM_TEST_ASSERT` macro is a very useful tool for performing checks
in tests. However, it is rather annoying to have to always specify a
message for the assert. Often the failure is self evident from the
condition (which is already printed out), and specifying a message is
both repetative and annoying.

Also, it is often equally annoying to print out additional information
in the case of an assertion failure. In that case, you have to either
attach a debugger or add a printf, see the problem, and remove the
printf.

This change solves both of these problems. `VTKM_TEST_ASSERT` now takes a
condition and a variable number of message arguments. If no message
arguments are given, then a default message (along with the condition)
are output. If multiple message arguments are given, they are appended
together in the result. The messages do not have to be strings. Any
object that can be sent to a stream will be printed correctly. This
allows you to print out the values that caused the issue.

So the old behavior of `VTKM_TEST_ASSERT` still works. So you can have a
statement like

```cpp
VTKM_TEST_ASSERT(array.GetNumberOfValues() != 0, "Array is empty");
```

As before, if this assertion failed, you would get the following error
message.

```
Array is empty (array.GetNumberOfValues() != 0)
```

However, in the statement above, you may feel that it is self evident that
`array.GetNumberOfValues() == 0` means the array is empty and you have to
type this into your test, like, 20 times. You can save yourself some work
by dropping the message.

```cpp
VTKM_TEST_ASSERT(array.GetNumberOfValues() != 0);
```

In this case if the assertion fails, you will get a message like this.

```
Test assertion failed (array.GetNumberOfValues() != 0)
```

But perhaps you have the opposite problem. Perhaps you need to output more
information. Let's say that you expected a particular operation to half the
length of an array. If the operation fails, it could be helpful to know how
big the array actually is. You can now actually output that on failure by
adding more message arguments.

```cpp
VTKM_TEST_ARRAY(outarray.GetNumberOfValues() == inarrayGetNumberOfValues()/2,
                "Expected array size ",
				inarrayGetNumberOfValues()/2,
				" but got ",
				outarray.GetNumberOfValues());
```

In this case, if the test failed, you might get an error like this.

```
Expected array size 5 but got 6 (outarray.GetNumberOfValues() == inarrayGetNumberOfValues()/2)
```
