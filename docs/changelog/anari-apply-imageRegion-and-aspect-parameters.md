## Applied imageRegion and aspect parameters to the perspective camera

The ANARI device now applies the `imageRegion` and `aspect` parameters to the perspective camera.

Both parameters are optional: If you do not explictly set `imageRegion` using `anariSetParameter`, the full viewport is rendered by default. If you do not explicitly set `aspect`, the device calculates the aspect ratio automatically using the camera's width and height.
