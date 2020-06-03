# Enable setting invalid value in probe filter

Initially, the probe filter would simply not set a value if a sample was
outside the input `DataSet`. This is not great as the memory could be
left uninitalized and lead to unpredictable results. The testing
compared these invalid results to 0, which seemed to work but is
probably unstable.

This was partially fixed by a previous change that consolidated to
mapping of cell data with a general routine that permuted data. However,
the fix did not extend to point data in the input, and it was not
possible to specify a particular invalid value.

This change specifically updates the probe filter so that invalid values
are set to a user-specified value.
