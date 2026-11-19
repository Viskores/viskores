## Render rays without a camera in the scalar renderer

`viskores::rendering::ScalarRenderer::Render` now also accepts rays that you
create, e.g., for a subset of the pixels. The pixel indices of the rays refer
to an image of the width and height of the renderer. Without a camera, there
is no surface shading: the result holds the fields and the depths.
`viskores::rendering::raytracing::ScalarRenderer::Render` has matching
overloads without a camera.
