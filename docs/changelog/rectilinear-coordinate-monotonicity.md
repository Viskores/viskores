## Support decreasing rectilinear coordinate axes

Rectilinear cell location and spline evaluation now support strictly monotonic
coordinate axes in either direction. Non-monotonic axes, including duplicate
coordinates, are rejected with `ErrorBadValue`.
