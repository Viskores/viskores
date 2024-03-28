==============================
Working with Cells
==============================

.. index:: cell

In the control environment, data is defined in mesh structures that comprise a set of finite cells.
(See :secref:`dataset:Cell Sets` for information on defining cell sets in the control environment.)
When worklets that operate on cells are scheduled, these grid structures are broken into their independent cells, and that data is handed to the worklet.
Thus, cell-based operations in the execution environment exclusively operate on independent cells.

Unlike some other libraries such as VTK, |VTKm| does not have a cell class that holds all the information pertaining to a cell of a particular type.
Instead, |VTKm| provides tags or identifiers defining the cell shape, and companion data like coordinate and field information are held in separate structures.
This organization is designed so a worklet may specify exactly what information it needs, and only that information will be loaded.


------------------------------
Cell Shape Tags and Ids
------------------------------

.. index::
   double: tag; cell shape
   double: tag; shape

Cell shapes can be specified with either a tag (defined with a struct with a name like ``CellShapeTag*``) or an enumerated identifier (defined with a constant number with a name like ``CELL_SHAPE_*``).
These shape tags and identifiers are defined in :file:`vtkm/CellShape.h` and declared in the ``vtkm`` namespace (because they can be used in either the control or the execution environment).
:numref:`fig:CellShapes` gives both the identifier and the tag names.

.. figure:: images/CellConnections.png
   :width: 100%
   :name: fig:CellShapes

   Basic Cell Shapes.

.. doxygenstruct:: vtkm::CellShapeTagVertex
   :members:
.. doxygenenumvalue:: vtkm::CELL_SHAPE_VERTEX
.. doxygenstruct:: vtkm::CellShapeTagLine
   :members:
.. doxygenenumvalue:: vtkm::CELL_SHAPE_LINE
.. doxygenstruct:: vtkm::CellShapeTagPolyLine
   :members:
.. doxygenenumvalue:: vtkm::CELL_SHAPE_POLY_LINE
.. doxygenstruct:: vtkm::CellShapeTagTriangle
   :members:
.. doxygenenumvalue:: vtkm::CELL_SHAPE_TRIANGLE
.. doxygenstruct:: vtkm::CellShapeTagPolygon
   :members:
.. doxygenenumvalue:: vtkm::CELL_SHAPE_POLYGON
.. doxygenstruct:: vtkm::CellShapeTagQuad
   :members:
.. doxygenenumvalue:: vtkm::CELL_SHAPE_QUAD
.. doxygenstruct:: vtkm::CellShapeTagTetra
   :members:
.. doxygenenumvalue:: vtkm::CELL_SHAPE_TETRA
.. doxygenstruct:: vtkm::CellShapeTagHexahedron
   :members:
.. doxygenenumvalue:: vtkm::CELL_SHAPE_HEXAHEDRON
.. doxygenstruct:: vtkm::CellShapeTagWedge
   :members:
.. doxygenenumvalue:: vtkm::CELL_SHAPE_WEDGE
.. doxygenstruct:: vtkm::CellShapeTagPyramid
   :members:
.. doxygenenumvalue:: vtkm::CELL_SHAPE_PYRAMID

In addition to the basic cell shapes, there is a special "empty" cell with the identifier :enumerator:`vtkm::CELL_SHAPE_EMPTY` and tag :class:`vtkm::CellShapeTagEmpty`.
This type of cell has no points, edges, or faces and can be thought of as a placeholder for a null or void cell.

.. doxygenstruct:: vtkm::CellShapeTagEmpty
   :members:
.. doxygenenumvalue:: vtkm::CELL_SHAPE_EMPTY

There is also a special cell shape "tag" named :class:`vtkm::CellShapeTagGeneric` that is used when the actual cell shape is not known at compile time.
:class:`vtkm::CellShapeTagGeneric` actually has a member variable named :var:`vtkm::CellShapeTagGeneric::Id` that stores the identifier for the cell shape.
There is no equivalent identifier for a generic cell; cell shape identifiers can be placed in a :type:`vtkm::IdComponent` at runtime.

.. doxygenstruct:: vtkm::CellShapeTagGeneric
   :members:

When using cell shapes in templated classes and functions, you can use the :c:macro:`VTKM_IS_CELL_SHAPE_TAG` to ensure a type is a valid cell shape tag.
This macro takes one argument and will produce a compile error if the argument is not a cell shape tag type.

.. doxygendefine:: VTKM_IS_CELL_SHAPE_TAG

Converting Between Tags and Identifiers
========================================

Every cell shape tag has a member variable named ``Id`` that contains the identifier for the cell shape.
This provides a convenient mechanism for converting a cell shape tag to an identifier.
Most cell shape tags have their ``Id`` member as a compile-time constant, but :var:`vtkm::CellShapeTagGeneric::Id` is set at run time.

The :file:`vtkm/CellShape.h` header also declares a templated class named :class:`vtkm::CellShapeIdToTag` that converts a cell shape identifier to a cell shape tag.
:class:`vtkm::CellShapeIdToTag` has a single template argument that is the identifier.
Inside the class is a type named :var:`vtkm::CellShapeIdToTag::Tag` that is the type of the correct tag.

.. doxygenstruct:: vtkm::CellShapeIdToTag
   :members:

.. load-example:: CellShapeIdToTag
   :file: GuideExampleCellShapes.cxx
   :caption: Using :class:`vtkm::CellShapeIdToTag`.

However, :class:`vtkm::CellShapeIdToTag` is only viable if the identifier can be resolved at compile time.
In the case where a cell identifier is stored in a variable or an array or the code is using a :class:`vtkm::CellShapeTagGeneric`, the correct cell shape is not known until run time.
In this case, the :c:macro:`vtkmGenericCellShapeMacro` macro can be used to check all possible conditions.
This macro is embedded in a switch statement where the condition is the cell shape identifier.

.. doxygendefine:: vtkmGenericCellShapeMacro

Often this method is used to implement the condition for a :class:`vtkm::CellShapeTagGeneric` in a function overloaded for cell types.
A demonstration of :c:macro:`vtkmGenericCellShapeMacro` is given in :numref:`ex:GenericCellNormal`.

Cell Traits
==============================

.. index:: cell traits

The :file:`vtkm/CellTraits.h` header file contains a traits class named :class:`vtkm::CellTraits` that provides information about a cell.

.. doxygenstruct:: vtkm::CellTraits
   :members:

.. doxygenstruct:: vtkm::CellTopologicalDimensionsTag
.. doxygenstruct:: vtkm::CellTraitsTagSizeFixed
.. doxygenstruct:: vtkm::CellTraitsTagSizeVariable

.. load-example:: GenericCellNormal
   :file: GuideExampleCellShapes.cxx
   :caption: Using :class:`vtkm::CellTraits` to implement a polygon normal estimator.


-----------------------------------
Parametric and World Coordinates
-----------------------------------

.. index::
   double: cell; parametric coordinates
   double: cell; world coordinates

Each cell type supports a one-to-one mapping between a set of parametric coordinates in the unit cube (or some subset of it) and the points in 3D space that are the locus contained in the cell.
Parametric coordinates are useful because certain features of the cell, such as vertex location and center, are at a consistent location in parametric space irrespective of the location and distortion of the cell in world space.
Also, many field operations are much easier with parametric coordinates.

The :file:`vtkm/exec/ParametricCoordinates.h` header file contains the following functions for working with parametric coordinates.
These functions contain several overloads for using different cell shape tags.

.. doxygenfunction:: vtkm::exec::ParametricCoordinatesCenter(vtkm::IdComponent, vtkm::CellShapeTagGeneric, vtkm::Vec<ParametricCoordType, 3>&)
.. doxygenfunction:: vtkm::exec::ParametricCoordinatesPoint(vtkm::IdComponent, vtkm::IdComponent, vtkm::CellShapeTagGeneric, vtkm::Vec<ParametricCoordType, 3>&)
.. doxygenfunction:: vtkm::exec::ParametricCoordinatesToWorldCoordinates(const WorldCoordVector&, const vtkm::Vec<PCoordType, 3>&, vtkm::CellShapeTagGeneric, typename WorldCoordVector::ComponentType&)
.. doxygenfunction:: vtkm::exec::WorldCoordinatesToParametricCoordinates(const WorldCoordVector&, const typename WorldCoordVector::ComponentType&, vtkm::CellShapeTagGeneric, typename WorldCoordVector::ComponentType&)


------------------------------
Interpolation
------------------------------

.. index::
   double: cell; interpolation

The shape of every cell is defined by the connections of some finite set of points.
Field values defined on those points can be interpolated to any point within the cell to estimate a continuous field.

The :file:`vtkm/exec/CellInterpolate.h` header contains the function :func:`vtkm::exec::CellInterpolate` to do this interpolation.

.. doxygenfunction:: vtkm::exec::CellInterpolate(const FieldVecType&, const vtkm::Vec<ParametricCoordType, 3>&, vtkm::CellShapeTagGeneric, typename FieldVecType::ComponentType&)

.. load-example:: CellCenters
   :file: GuideExampleCellOperations.cxx
   :caption: Interpolating field values to a cell's center.


------------------------------
Derivatives
------------------------------

.. index::
   double: cell; derivative
   double: cell; gradient

Since interpolations provide a continuous field function over a cell, it is reasonable to consider the derivative of this function.
The :file:`vtkm/exec/CellDerivative.h` header contains the function :func:`vtkm::exec::CellDerivative` to compute derivatives.
The derivative is returned in a :class:`vtkm::Vec` of size 3 corresponding to the partial derivatives in the :math:`x`, :math:`y`, and :math:`z` directions.
This derivative is equivalent to the gradient of the field.

.. load-example:: CellDerivatives
   :file: GuideExampleCellOperations.cxx
   :caption: Computing the derivative of the field at cell centers.


------------------------------
Edges and Faces
------------------------------

.. index::
   single: point
   single: cell; point
   double: cell; shape

As explained earlier in this chapter, a cell is defined by a collection of points and a shape identifier that describes how the points come together to form the structure of the cell.
The cell shapes supported by |VTKm| are documented in :secref:`working-with-cells:Cell Shape Tags and Ids`.
It contains :numref:`fig:CellShapes`, which shows how the points for each shape form the structure of the cell.

.. index::
   single: edge
   single: cell; edge
   single: shape; edge

Most cell shapes can be broken into subelements.
2D and 3D cells have pairs of points that form *edges* at the boundaries of the cell.
Likewise, 3D cells have loops of edges that form *faces* that encase the cell.
:numref:`fig:CellConstituents` demonstrates the relationship of these constituent elements for some example cell shapes.

.. figure:: images/CellConstituents.png
   :width: 50%
   :name: fig:CellConstituents

   The constituent elements (points, edges, and faces) of cells..

The header file :file:`vtkm/exec/CellEdge.h` contains a collection of functions to help identify the edges of a cell.

.. doxygenfunction:: vtkm::exec::CellEdgeNumberOfEdges(vtkm::IdComponent, vtkm::CellShapeTagGeneric, vtkm::IdComponent&)
.. doxygenfunction:: vtkm::exec::CellEdgeLocalIndex(vtkm::IdComponent, vtkm::IdComponent, vtkm::IdComponent, vtkm::CellShapeTagGeneric, vtkm::IdComponent&)
.. doxygenfunction:: vtkm::exec::CellEdgeCanonicalId

The following example demonstrates a pair of worklets that use the cell edge functions.
As is typical for operations of this nature, one worklet counts the number of edges in each cell and another uses this count to generate the data.

.. didyouknow::
   :numref:`ex:CellEdge` demonstrates one of many techniques for creating cell sets in a worklet.
   Chapter~\ref{chap:GeneratingCellSets} describes this and many more such techniques.

.. todo:: Fix chap:GeneratingCellSets reference above.

.. load-example:: CellEdge
   :file: GuideExampleCellEdgesFaces.cxx
   :caption: Using cell edge functions.

.. index::
   single: face
   single: cell; face
   single: shape; face

The header file :file:`vtkm/exec/CellFace.h` contains a collection of functions to help identify the faces of a cell.

.. doxygenfunction:: vtkm::exec::CellFaceNumberOfFaces
.. doxygenfunction:: vtkm::exec::CellFaceNumberOfPoints
.. doxygenfunction:: vtkm::exec::CellFaceShape
.. doxygenfunction:: vtkm::exec::CellFaceLocalIndex
.. doxygenfunction:: vtkm::exec::CellFaceCanonicalId

The following example demonstrates a triple of worklets that use the cell face functions.
As is typical for operations of this nature, the worklets are used in steps to first count entities and then generate new entities.
In this case, the first worklet counts the number of faces and the second worklet counts the points in each face.
The third worklet generates cells for each face.

.. load-example:: CellFace
   :file: GuideExampleCellEdgesFaces.cxx
   :caption: Using cell face functions.
