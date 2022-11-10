# Fix VTKM_LOG_SCOPE

The `VTKM_LOG_SCOPE` macro was not working as intended. It was supposed to
print a log message immediately and then print a second log message when
leaving the scope along with the number of seconds that elapsed between the
two messages.

This was not what was happening. The second log message was being printed
immediately after the first. This is because the scope was taken inside of
the `LogScope` method. The macro has been rewritten to put the tracking in
the right scope.
