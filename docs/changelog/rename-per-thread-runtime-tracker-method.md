# Renamed RuntimeDeviceTrackers to use the term Global

The `GetGlobalRuntimeDeviceTracker` never actually returned a process wide
runtime device tracker but always a unique one for each control side thread.
This was the design as it would allow for different threads to have different
runtime device settings.

By removing the term Global from the name it becomes more clear what scope this
class has.
