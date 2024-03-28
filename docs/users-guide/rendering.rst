==============================
Rendering
==============================

.. index:: rendering

Rendering, the generation of images from data, is a key component to visualization.
To assist with rendering, |VTKm| provides a rendering package to produce imagery from data, which is located in the ``vtkm::rendering`` namespace.

The rendering package in |VTKm| is not intended to be a fully featured
rendering system or library. Rather, it is a lightweight rendering package
with two primary use cases:

  * New users getting started with |VTKm| need a "quick and dirty" render method to see their visualization results.
  * In situ visualization that integrates |VTKm| with a simulation or other data-generation system might need a lightweight rendering method.

Both of these use cases require just a basic rendering platform.
Because |VTKm| is designed to be integrated into larger systems, it does not aspire to have a fully featured rendering system.

.. didyouknow::
   |VTKm|'s big sister toolkit VTK is already integrated with |VTKm| and has its own fully featured rendering system.
   If you need more rendering capabilities than what |VTKm| provides, you can leverage VTK instead.


------------------------------
Scenes and Actors
------------------------------

.. index::
   double: rendering; actor

The primary intent of the rendering package in |VTKm| is to visually display the data that is loaded and processed.
Data are represented in |VTKm| by :class:`vtkm::cont::DataSet` objects, which are described in :chapref:`dataset:Data Sets`.
They are also the object created from :chapref:`io:File I/O` and :chapref:`running-filters:Running Filters`.

To render a :class:`vtkm::cont::DataSet`, the data are wrapped in a
:class:`vtkm::rendering::Actor` class. The :class:`vtkm::rendering::Actor` holds the
components of the :class:`vtkm::cont::DataSet` to render (a cell set, a
coordinate system, and a field). A color table can also be optionally be
specified, but a default color table will be specified otherwise.

.. load-example:: ActorScene
   :file: GuideExampleRendering.cxx
   :caption: Creating an :class:`vtkm::rendering::Actor` and adding it to a :class:`vtkm::rendering::Scene`.

.. doxygenclass:: vtkm::rendering::Actor
   :members:

.. index::
   double: rendering; scene

:class:`vtkm::rendering::Actor` objects are collected together in an object called :class:`vtkm::rendering::Scene`.
       An :class:`vtkm::rendering::Actor` is added to a :class:`vtkm::rendering::Scene` with the :func:`vtkm::rendering::Scene::AddActor` method.

.. doxygenclass:: vtkm::rendering::Scene
   :members:

The following example demonstrates creating a :class:`vtkm::rendering::Scene` with one :class:`vtkm::rendering::Actor`.


------------------------------
Canvas
------------------------------

.. index::
   double: rendering; canvas

A canvas is a unit that represents the image space that is the target of the rendering.
The canvas' primary function is to manage the buffers that hold the working image data during the rendering.
The canvas also manages the context and state of the rendering subsystem.

.. index::
   double: canvas; ray tracer

:class:`vtkm::rendering::Canvas` is the base class of all canvas objects.
Each type of rendering system has its own canvas subclass, but currently the only rendering system provided by |VTKm| is the internal ray tracer.
The canvas for the ray tracer is :class:`vtkm::rendering::CanvasRayTracer`.
:class:`vtkm::rendering::CanvasRayTracer` is typically constructed by giving the width and height of the image to render.

.. load-example:: Canvas
   :file: GuideExampleRendering.cxx
   :caption: Creating a canvas for rendering.

.. doxygenclass:: vtkm::rendering::CanvasRayTracer
   :members:

.. doxygenclass:: vtkm::rendering::Canvas
   :members:


------------------------------
Mappers
------------------------------

.. index::
   double: rendering; mapper

A mapper is a unit that converts data (managed by an :class:`vtkm::rendering::Actor`) and issues commands to the rendering subsystem to generate images.
All mappers in |VTKm| are a subclass of :class:`vtkm::rendering::Mapper`.
Different mappers could render different types of data in different ways.
For example, one mapper might render polygonal surfaces whereas another might render polyhedra as a translucent volume.


.. doxygenclass:: vtkm::rendering::Mapper
   :members:

..
  Also, different rendering systems (as established by the :class:`vtkm::rendering::Canvas`) often require different mappers.
  Thus, a mapper should be picked to match both the rendering system of the :class:`vtkm::rendering::Canvas` and the data in the :class:`vtkm::rendering::Actor`.

The following mappers are provided by |VTKm|.

.. doxygenclass:: vtkm::rendering::MapperCylinder
   :members:

.. doxygenclass:: vtkm::rendering::MapperGlyphBase
   :members:

.. doxygenclass:: vtkm::rendering::MapperGlyphScalar
   :members:

.. doxygenclass:: vtkm::rendering::MapperGlyphVector
   :members:

.. doxygenclass:: vtkm::rendering::MapperPoint
   :members:

.. doxygenclass:: vtkm::rendering::MapperQuad
   :members:

.. doxygenclass:: vtkm::rendering::MapperRayTracer
   :members:

.. doxygenclass:: vtkm::rendering::MapperVolume
   :members:

.. doxygenclass:: vtkm::rendering::MapperWireframer
   :members:


------------------------------
Views
------------------------------

.. index::
   double: rendering; view

A view is a unit that collects all the structures needed to perform rendering.
It contains everything needed to take a :class:`vtkm::rendering::Scene` and use a :class:`vtkm::rendering::Mapper` to render it onto a :class:`vtkm::rendering::Canvas`.
The view also annotates the image with spatial and scalar properties.

The base class for all views is :class:`vtkm::rendering::View`, which is an abstract class.
You must choose one of the three provided subclasses, :class:`vtkm::rendering::View3D`, :class:`vtkm::rendering::View2D`, and :class:`vtkm::rendering::View3D`, depending on the type of data being presented.
All three view classes take a :class:`vtkm::rendering::Scene`, a :class:`vtkm::rendering::Mapper`, and a :class:`vtkm::rendering::Canvas` as arguments to their constructor.

.. load-example:: ConstructView
   :file: GuideExampleRendering.cxx
   :caption: Constructing a :class:`vtkm::rendering::View`.

.. doxygenclass:: vtkm::rendering::View
   :members:

.. doxygenclass:: vtkm::rendering::View1D
   :members:

.. doxygenclass:: vtkm::rendering::View2D
   :members:

.. doxygenclass:: vtkm::rendering::View3D
   :members:

.. index::
   double: color; background
   double: color; foreground

The :class:`vtkm::rendering::View` also maintains a background color (the color used in areas where nothing is drawn) and a foreground color (the color used for annotation elements).
By default, the :class:`vtkm::rendering::View` has a black background and a white foreground.
These can be set in the view's constructor, but it is a bit more readable to set them using the :func:`vtkm::rendering::View::SetBackground` and :func:`vtkm::rendering::View::SetForeground` methods.
In either case, the colors are specified using the :class:`vtkm::rendering::Color` helper class, which manages the red, green, and blue color channels as well as an optional alpha channel.
These channel values are given as floating point values between 0 and 1.

.. load-example:: ViewColors
   :file: GuideExampleRendering.cxx
   :caption: Changing the background and foreground colors of a :class:`vtkm::rendering::View`.

.. commonerrors::
   Although the background and foreground colors are set independently, it will be difficult or impossible to see the annotation if there is not enough contrast between the background and foreground colors.
   Thus, when changing a :class:`vtkm::rendering::View`'s background color, it is always good practice to also change the foreground color.

.. doxygenclass:: vtkm::rendering::Color
   :members:

Once the :class:`vtkm::rendering::View` is constructed, intialized, and set up, it is ready to render.
This is done by calling the :func:`vtkm::rendering::View::Paint` method.

.. load-example:: PaintView
   :file: GuideExampleRendering.cxx
   :caption: Using :func:`vtkm::rendering::Canvas::Paint` in a display callback.

Putting together :numref:`ex:ConstructView`, :numref:`ex:ViewColors`, and :numref:`ex:PaintView`, the final render of a view looks like that in :numref:`fig:ExampleRendering`.

.. figure:: images/BasicRendering.png
   :width: 100%
   :name: fig:ExampleRendering

   Example output of |VTKm|'s rendering system.

.. Note: BasicRendering.png is generated by the GuideExampleRendering.cxx code.

Of course, the :class:`vtkm::rendering::CanvasRayTracer` created in :numref:`ex:ConstructView` is an offscreen rendering buffer, so you cannot immediately see the image.
When doing batch visualization, an easy way to output the image to a file for later viewing is with the :func:`vtkm::rendering::View::SaveAs` method.
This method can save the image in either PNG or in the portable pixelmap (PPM) format.

.. load-example:: SaveView
   :file: GuideExampleRendering.cxx
   :caption: Saving the result of a render as an image file.

We visit doing interactive rendering in a GUI later in :secref:`rendering:Interactive Rendering`.


------------------------------
Changing Rendering Modes
------------------------------

:numref:`ex:ConstructView` constructs the default mapper for ray tracing, which renders the data as an opaque solid.
However, you can change the rendering mode by using one of the other mappers listed in :secref:`rendering:Mappers`.
For example, say you just wanted to see a wireframe representation of your data.
You can achieve this by using :class:`vtkm::rendering::MapperWireframer`.

.. index::
   double: rendering; wireframe

.. load-example:: MapperEdge
   :file: GuideExampleRendering.cxx
   :caption: Creating a mapper for a wireframe representation.

Alternatively, perhaps you wish to render just the points of mesh.
:class:`vtkm::rendering::MapperGlyphScalar` renders the points as glyphs and also optionally can scale the glyphs based on field values.

.. load-example:: MapperGlyphScalar
   :file: GuideExampleRendering.cxx
   :caption: Creating a mapper for point representation.

These mappers respectively render the images shown in :numref:`fig:AlternateMappers`.
Other mappers, such as those that can render translucent volumes, are also available.

.. figure:: images/AlternateRendering.png
   :width: 100%
   :name: fig:AlternateMappers

   Examples of alternate rendering modes using different mappers.
   The top left image is rendered with :class:`vtkm::rendering::MapperWireframer`.
   The top right and bottom left images are rendered with :class:`vtkm::rendering::MapperGlyphScalar`.
   The bottom right image is rendered with :class:`vtkm::rendering::MapperGlyphVector`.


------------------------------
Manipulating the Camera
------------------------------

.. index::
   double: rendering; camera

The :class:`vtkm::rendering::View` uses an object called :class:`vtkm::rendering::Camera` to describe the vantage point from which to draw the geometry.
The camera can be retrieved from the :func:`vtkm::rendering::View::GetCamera` method.
That retrieved camera can be directly manipulated or a new camera can be provided by calling :func:`vtkm::rendering::View::SetCamera`.
In this section we discuss camera setups typical during view set up.
Camera movement during interactive rendering is revisited in :secref:`rendering:Camera Movement`.

.. doxygenclass:: vtkm::rendering::Camera
   :members:

A :class:`vtkm::rendering::Camera` operates in one of two major modes: 2D mode or 3D mode.
2D mode is designed for looking at flat geometry (or close to flat geometry) that is parallel to the x-y plane.
3D mode provides the freedom to place the camera anywhere in 3D space.
The different modes can be set with :func:`vtkm::rendering::Camera::SetModeTo2D` and :func:`vtkm::rendering::Camera::SetModeTo3D`, respectively.
The interaction with the camera in these two modes is very different.

Common Camera Controls
==============================

Some camera controls operate relative to the rendered image and are common among the 2D and 3D camera modes.

Pan
------------------------------

.. index::
   triple: camera; rendering; pan

A camera pan moves the viewpoint left, right, up, or down.
A camera pan is performed by calling the :func:`vtkm::cont::Camera::Pan` method.
:func:`vtkm::cont::Camera::Pan` takes two arguments: the amount to pan in x and the amount to pan in y.

The pan is given with respect to the projected space. So a pan of :math:`1` in
the x direction moves the camera to focus on the right edge of the image
whereas a pan of :math:`-1` in the x direction moves the camera to focus on the
left edge of the image.

.. load-example:: Pan
   :file: GuideExampleRenderingInteractive.cxx
   :caption: Panning the camera.

Zoom
------------------------------

.. index::
   triple: camera; rendering; zoom

A camera zoom draws the geometry larger or smaller.
A camera zoom is performed by calling the :func:`vtkm::rendering::Camera::Zoom` method.
:func:`vtkm::rendering::Camera::Zoom` takes a single argument specifying the zoom factor.
A positive number draws the geometry larger (zoom in), and larger zoom factor results in larger geometry.
Likewise, a negative number draws the geometry smaller (zoom out).
A zoom factor of 0 has no effect.

.. load-example:: Zoom
   :file: GuideExampleRenderingInteractive.cxx
   :caption: Zooming the camera.

2D Camera Mode
==============================

.. index::
   triple: camera; rendering; 2D

The 2D camera is restricted to looking at some region of the x-y plane.

View Range
------------------------------

.. index::
   triple: camera; rendering; view range

The vantage point of a 2D camera can be specified by simply giving the region in the x-y plane to look at.
This region is specified by calling :func:`vtkm::rendering::Camera::SetViewRange2D`.
This method takes the left, right, bottom, and top of the region to view.
Typically these are set to the range of the geometry in world space as shown in :numref:`fig:CameraViewRange2D`.

.. figure:: images/CameraViewRange2D.png
   :width: 100%
   :name: fig:CameraViewRange2D

   The view range bounds to give a :class:`vtkm::rendering::Camera`.

3D Camera Mode
==============================

.. index::
   triple: camera; rendering; 3D
   double: pinhole; camera

The 3D camera is a free-form camera that can be placed anywhere in 3D space and can look in any direction.
The projection of the 3D camera is based on the pinhole camera pinhole camera model in which all viewing rays intersect a single point.
This single point is the camera's position.

Position and Orientation
------------------------------

.. index::
   triple: camera; rendering; position
   triple: camera; rendering; look at
   triple: camera; rendering; focal point

The position of the camera, which is the point where the observer is viewing the scene, can be set with the :func:`vtkm::rendering::Camera::SetPosition` method.
The direction the camera is facing is specified by giving a position to focus on.
This is called either the "look at" point or the focal point and is specified with the :func:`vtkm::rendering::Camera::SetLookAt` method.
:numref:`fig:CameraPositionOrientation3D` shows the relationship between the position and look at points.

.. figure:: images/CameraPositionOrientation.png
   :width: 100%
   :name: fig:CameraPositionOrientation3D

   The position and orientation parameters for a :class:`vtkm::rendering::Camera`.

.. index::
   triple: camera; rendering; view up
   triple: camera; rendering; up

In addition to specifying the direction to point the camera, the camera must also know which direction is considered "up."
This is specified with the view up vector using the :func:`vtkm::rendering::Camera::SetViewUp` method.
The view up vector points from the camera position (in the center of the image) to the top of the image.
The view up vector in relation to the camera position and orientation is shown in :numref:`fig:CameraPositionOrientation3D`.

.. index::
   triple: camera; rendering; field of view

Another important parameter for the camera is its field of view.
The field of view specifies how wide of a region the camera can see.
It is specified by giving the angle in degrees of the cone of visible region emanating from the pinhole of the camera to the :func:`vtkm::rendering::Camera::SetFieldOfView` method.
The field of view angle in relation to the camera orientation is shown in :numref:`fig:CameraPositionOrientation3D`.
A field of view angle of :math:`60^{\circ}` usually works well.

.. index::
   triple: camera; rendering; clipping range
   triple: camera; rendering; near clip plane
   triple: camera; rendering; far clip plane

Finally, the camera must specify a clipping region that defines the valid range of depths for the object.
This is a pair of planes parallel to the image that all visible data must lie in.
Each of these planes is defined simply by their distance to the camera position.
The near clip plane is closer to the camera and must be in front of all geometry.
The far clip plane is further from the camera and must be behind all geometry.
The distance to both the near and far planes are specified with the :func:`vtkm::rendering::Camera::SetClippingRange` method.
:numref:`fig:CameraPositionOrientation3D` shows the clipping planes in relationship to the camera position and orientation.

.. load-example:: CameraPositionOrientation
   :file: GuideExampleRenderingInteractive.cxx
   :caption: Directly setting :class:`vtkm::rendering::Camera` position and orientation.

Movement
------------------------------

In addition to specifically setting the position and orientation of the camera, :class:`vtkm::rendering::Camera` contains several convenience methods that move the camera relative to its position and look at point.

.. index::
   triple: camera; rendering; elevation
   triple: camera; rendering; azimuth

Two such methods are elevation and azimuth, which move the camera around the sphere centered at the look at point.
:func:`vtkm::rendering::Camera::Elevation` raises or lowers the camera.
Positive values raise the camera up (in the direction of the view up vector) whereas negative values lower the camera down.
:func:`vtkm::rendering::Camera::Azimuth` moves the camera around the look at point to the left or right.
Positive values move the camera to the right whereas negative values move the camera to the left.
Both :func:`vtkm::rendering::Camera::Elevation` and :func:`vtkm::rendering::Camera::Azimuth` specify the amount of rotation in terms of degrees.
:numref:`fig:CameraMovement` shows the relative movements of :func:`vtkm::rendering::Camera::Elevation` and :func:`vtkm::rendering::Camera::Azimuth`.

.. figure:: images/CameraMovement.png
   :width: 100%
   :name: fig:CameraMovement

   :class:`vtkm::rendering::Camera` movement functions relative to position and orientation.

.. load-example:: CameraMovement
   :file: GuideExampleRenderingInteractive.cxx
   :caption: Moving the camera around the look at point.

.. commonerrors::
   The :func:`vtkm::rendering::Camera::Elevation` and :func:`vtkm::rendering::Camera::Azimuth` methods change the position of the camera, but not the view up vector.
   This can cause some wild camera orientation changes when the direction of the camera view is near parallel to the view up vector, which often happens when the elevation is raised or lowered by about 90 degrees.

In addition to rotating the camera around the look at point, you can move the camera closer or further from the look at point.
This is done with the :func:`vtkm::rendering::Camera::Dolly` method.
The :func:`vtkm::rendering::Camera::Dolly` method takes a single value that is the factor to scale the distance between camera and look at point.
Values greater than one move the camera away, values less than one move the camera closer.
The direction of dolly movement is shown in :numref:`fig:CameraMovement`.

Finally, the :func:`vtkm::rendering::Camera::Roll` method rotates the camera around the viewing direction.
It has the effect of rotating the rendered image.
The :func:`vtkm::rendering::Camera::Roll` method takes a single value that is the angle to rotate in degrees.
The direction of roll movement is shown in :numref:`fig:CameraMovement`.

Reset
------------------------------

.. index::
   triple: camera; rendering; reset

Setting a specific camera position and orientation can be frustrating, particularly when the size, shape, and location of the geometry is not known a priori.
Typically this involves querying the data and finding a good camera orientation.

To make this process simpler, the :func:`vtkm::rendering::Camera::ResetToBounds` convenience method automatically positions the camera based on the spatial bounds of the geometry.
The most expedient method to find the spatial bounds of the geometry being rendered is to get the :class:`vtkm::rendering::Scene` object and call :func:`vtkm::rendering::Scene::GetSpatialBounds`.
The :class:`vtkm::rendering::Scene` object can be retrieved from the :class:`vtkm::rendering::View`, which, as described in :secref:`rendering:Views`, is the central object for managing rendering.

.. load-example:: ResetCamera
   :file: GuideExampleRenderingInteractive.cxx
   :caption: Resetting a :class:`vtkm::rendering::Camera` to view geometry.

The :func:`vtkm::rendering::Camera::ResetToBounds` method operates by placing the look at point in the center of the bounds and then placing the position of the camera relative to that look at point.
The position is such that the view direction is the same as before the call to :func:`vtkm::rendering::Camera::ResetToBounds` and the distance between the camera position and look at point has the bounds roughly fill the rendered image.
This behavior is a convenient way to update the camera to make the geometry most visible while still preserving the viewing position.
If you want to reset the camera to a new viewing angle, it is best to set the camera to be pointing in the right direction and then calling :func:`vtkm::rendering::Camera::ResetToBounds` to adjust the position.

.. load-example:: AxisAlignedCamera
   :file: GuideExampleRenderingInteractive.cxx
   :caption: Resetting a :class:`vtkm::rendering::Camera` to be axis aligned.


------------------------------
Interactive Rendering
------------------------------

.. index::
   double: rendering; interactive

So far in our description of |VTKm|'s rendering capabilities we have talked about doing rendering of fixed scenes.
However, an important use case of scientific visualization is to provide an interactive rendering system to explore data.
In this case, you want to render into a GUI application that lets the user interact manipulate the view.
The full design of a 3D visualization application is well outside the scope of this book, but we discuss in general terms what you need to plug |VTKm|'s rendering into such a system.

In this section we discuss two important concepts regarding interactive rendering.
First, we need to write images into a GUI while they are being rendered.
Second, we want to translate user interaction to camera movement.

Rendering Into a GUI
==============================

.. index::
   triple: interactive; rendering; OpenGL

Before being able to show rendering to a user, we need a system rendering context in which to push the images.
In this section we demonstrate the display of images using the OpenGL rendering system, which is common for scientific visualization applications.
That said, you could also use other rendering systems like DirectX or even paste images into a blank widget.

Creating an OpenGL context varies depending on the OS platform you are using.
If you do not already have an application you want to integrate with |VTKm|'s rendering, you may wish to start with graphics utility API such as GLUT or GLFW.
The process of initializing an OpenGL context is not discussed here.

The process of rendering into an OpenGL context is straightforward.
First call :func:`vtkm::rendering::View::Paint` on the :class:`vtkm::rendering::View` object to do the actual rendering.
Second, get the image color data out of the :class:`vtkm::rendering::View`'s :class:`vtkm::rendering::Canvas` object.
This is done by calling :func:`vtkm::rendering::Canvas::GetColorBuffer`.
This will return a :class:`vtkm::cont::ArrayHandle` object containing the image's pixel color data.
(:class:`vtkm::cont::ArrayHandle` is discussed in detail in :chapref:`basic-array-handles:Basic Array Handles` and subsequent chapters.)
A raw pointer can be pulled out of this :class:`vtkm::cont::ArrayHandle` by casting it to the :class:`vtkm::cont::ArrayHandleBase` subclass and calling the :func:`vtkm::cont::ArrayHandleBase::GetReadPointer` method on that.
Third, the pixel color data are pasted into the OpenGL render context.
There are multiple ways to do so, but the most straightforward way is to use the ``glDrawPixels`` function provided by OpenGL.
Fourth, swap the OpenGL buffers.
The method to swap OpenGL buffers varies by OS platform.
The aforementioned graphics libraries GLUT and GLFW each provide a function for doing so.

.. load-example:: RenderToOpenGL
   :file: GuideExampleRenderingInteractive.cxx
   :caption: Rendering a :class:`vtkm::rendering::View` and pasting the result to an active OpenGL context.

Camera Movement
==============================

.. index::
   triple: interactive; rendering; camera
   triple: camera; rendering; mouse

When interactively manipulating the camera in a windowing system, the camera is usually moved in response to mouse movements.
Typically, mouse movements are detected through callbacks from the windowing system back to your application.
Once again, the details on how this works depend on your windowing system.
The assumption made in this section is that through the windowing system you will be able to track the x-y pixel location of the mouse cursor at the beginning of the movement and the end of the movement.
Using these two pixel coordinates, as well as the current width and height of the render space, we can make several typical camera movements.

.. commonerrors::
   Pixel coordinates in |VTKm|'s rendering system originate in the lower-left corner of the image.
   However, windowing systems generally report mouse coordinates with the origin in the *upper*-left corner.
   The upshot is that the y coordinates will have to be reversed when translating mouse coordinates to |VTKm| image coordinates.
   This inverting is present in all the following examples.

Interactive Rotate
------------------------------

.. index::
   double: mouse; rotation
   double: rotation; rendering

A common and important mode of interaction with 3D views is to allow the user to rotate the object under inspection by dragging the mouse.
To facilitate this type of interactive rotation, :class:`vtkm::rendering::Camera` provides a convenience method named :func:`vtkm::rendering::Camera::TrackballRotate`.
It takes a start and end position of the mouse on the image and rotates viewpoint as if the user grabbed a point on a sphere centered in the image at the start position and moved under the end position.

The :func:`vtkm::rendering::Camera::TrackballRotate` method is typically called from within a mouse movement callback.
The callback must record the pixel position from the last event and the new pixel position of the mouse.
Those pixel positions must be normalized to the range -1 to 1 where the position (-1,-1) refers to the lower left of the image and (1,1) refers to the upper right of the image.
The following example demonstrates the typical operations used to establish rotations when dragging the mouse.

.. load-example:: MouseRotate
   :file: GuideExampleRenderingInteractive.cxx
   :caption: Interactive rotations through mouse dragging with :func:`vtkm::rendering::Camera::TrackballRotate`.

Interactive Pan
------------------------------

.. index::
   double: mouse; pan
   double: pan; rendering

Panning can be performed by calling :func:`vtkm::rendering::Camera::Pan` with the translation relative to the width and height of the canvas.
For the translation to track the movement of the mouse cursor, simply scale the pixels the mouse has traveled by the width and height of the image.

.. load-example:: MousePan
   :file: GuideExampleRenderingInteractive.cxx
   :caption: Pan the view based on mouse movements.

Interactive Zoom
------------------------------

.. index::
   double: mouse; zoom
   double: zoom; rendering

Zooming can be performed by calling :func:`vtkm::rendering::Camera::Zoom` with a positive or negative zoom factor.
When using :func:`vtkm::rendering::Camera::Zoom` to respond to mouse movements, a natural zoom will divide the distance traveled by the mouse pointer by the width or height of the screen as demonstrated in the following example.

.. load-example:: MouseZoom
   :file: GuideExampleRenderingInteractive.cxx
   :caption: Zoom the view based on mouse movements.


------------------------------
Color Tables
------------------------------

.. index::
   double: rendering; color tables

An important feature of |VTKm|'s rendering units is the ability to :index:`pseudocolor` objects based on scalar data.
This technique maps each scalar to a potentially unique color.
This mapping from scalars to colors is defined by a :class:`vtkm::cont::ColorTable` object.
A :class:`vtkm::cont::ColorTable` can be specified as an optional argument when constructing a :class:`vtkm::rendering::Actor`.
(Use of :class:`vtkm::rendering::Actor` is discussed in :secref:`rendering:Scenes and Actors`.)

.. load-example:: SpecifyColorTable
   :file: GuideExampleRenderingInteractive.cxx
   :caption: Specifying a :class:`vtkm::cont::ColorTable` for a :class:`vtkm::rendering::Actor`.

.. doxygenclass:: vtkm::cont::ColorTable
   :members:

The easiest way to create a :class:`vtkm::cont::ColorTable` is to provide the
name of one of the many predefined sets of color provided by VTK-m. A list
of all available predefined color tables is provided below.

.. This file and all the images in images/color-tables are built by the GuideExampleColorTables test.
.. include:: color-table-presets.rst

* |Viridis| ``Viridis``
  Matplotlib Virdis, which is designed to have perceptual uniformity, accessibility to color blind viewers, and good conversion to black and white.
  This is the default color map.
* |Cool-to-Warm| ``Cool to Warm``
  A color table designed to be perceptually even, to work well on shaded 3D surfaces, and to generally perform well across many uses.
* |Cool-to-Warm-Extended| ``Cool to Warm Extended``
  This colormap is an expansion on cool to warm that moves through a wider range of hue and saturation.
  Useful if you are looking for a greater level of detail, but the darker colors at the end might interfere with 3D surfaces.
* |Inferno| ``Inferno``
  Matplotlib Inferno, which is designed to have perceptual uniformity, accessibility to color blind viewers, and good conversion to black and white.
* |Plasma| ``Plasma``
  Matplotlib Plasma, which is designed to have perceptual uniformity, accessibility to color blind viewers, and good conversion to black and white.
* |Black-Body-Radiation| ``Black Body Radiation``
  The colors are inspired by the wavelengths of light from black body radiation.
  The actual colors used are designed to be perceptually uniform.
* |X-Ray| ``X Ray``
  Greyscale colormap useful for making volume renderings similar to what you would expect in an x-ray.
* |Green| ``Green``
  A sequential color map of green varied by saturation.
* |Black---Blue---White| ``Black - Blue - White``
  A sequential color map from black to blue to white.
* |Blue-to-Orange| ``Blue to Orange``
  A double-ended (diverging) color table that goes from dark blues to a neutral white and then a dark orange at the other end.
* |Gray-to-Red| ``Gray to Red``
  A double-ended (diverging) color table with black/gray at the low end and orange/red at the high end.
* |Cold-and-Hot| ``Cold and Hot``
  A double-ended color map with a black middle color and diverging values to either side.
  Colors go from red to yellow on the positive side and through blue on the negative side.
* |Blue---Green---Orange| ``Blue - Green - Orange``
  A three-part color map with blue at the low end, green in the middle, and orange at the high end.
* |Yellow---Gray---Blue| ``Yellow - Gray - Blue``
  A three-part color map with yellow at the low end, gray in the middle, and blue at the high end.
* |Rainbow-Uniform| ``Rainbow Uniform``
  A color table that spans the hues of a rainbow.
  This color table modifies the hues to make them more perceptually uniform than the raw color wavelengths.
* |Jet| ``Jet``
  A rainbow color table that adds some darkness for greater perceptual resolution.
* |Rainbow-Desaturated| ``Rainbow Desaturated``
  Basic rainbow colors with periodic dark points to increase the local discriminability.
