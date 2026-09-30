## Make the ray-tracing light position configurable

Mappers own the point-light position and pass it to the ray tracer when
rendering. If no position is configured, the mapper uses the camera position.
The ray tracer no longer stores a light position, so its direct `Render` path
also defaults to the camera position while mapper-controlled renders can use a
configured position. The ANARI device relies on this camera-position default
until explicit ANARI light objects are supported.
