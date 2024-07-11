## Added log entry when a cast and call fallback is used

Several places in VTK-m use the `CastAndCallForTypesWithFallback` method in
`UnknownArrayHandle`. The method works well for catching both common and
corner cases. However, there was no way to know if the efficient direct
method or the (supposedly) less likely fallback of copying data to a float
array was used. VTK-m now adds a log event, registered at the "INFO" level,
whenever data is copied to a fallback float array. This helps developers
monitor the eficiency of their code.

