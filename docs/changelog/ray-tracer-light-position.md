## Make the ray-tracing light position configurable

The ray tracer and scalar renderer now place the light at the camera by default.
Applications can override the default with `SetLightPosition` and query the
effective position with `GetLightPosition`. Mappers provide this configuration
through their common base class. This removes the previous hard-coded offset
along the camera's up direction.
