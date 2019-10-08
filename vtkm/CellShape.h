//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_CellShape_h
#define vtk_m_CellShape_h

#include <vtkm/StaticAssert.h>
#include <vtkm/Types.h>

#include <lcl/Polygon.h>
#include <lcl/Shapes.h>

// Vtk-c does not have tags for Empty and PolyLine. Define dummy tags here to
// avoid compilation errors. These tags are not used anywhere.
namespace lcl
{
struct Empty;
struct PolyLine;
}

namespace vtkm
{

/// CellShapeId identifies the type of each cell. Currently these are designed
/// to match up with VTK cell types.
///
enum CellShapeIdEnum
{
  // Linear cells
  CELL_SHAPE_EMPTY = lcl::ShapeId::EMPTY,
  CELL_SHAPE_VERTEX = lcl::ShapeId::VERTEX,
  //CELL_SHAPE_POLY_VERTEX      = 2,
  CELL_SHAPE_LINE = lcl::ShapeId::LINE,
  CELL_SHAPE_POLY_LINE = 4,
  CELL_SHAPE_TRIANGLE = lcl::ShapeId::TRIANGLE,
  //CELL_SHAPE_TRIANGLE_STRIP   = 6,
  CELL_SHAPE_POLYGON = lcl::ShapeId::POLYGON,
  //CELL_SHAPE_PIXEL            = 8,
  CELL_SHAPE_QUAD = lcl::ShapeId::QUAD,
  CELL_SHAPE_TETRA = lcl::ShapeId::TETRA,
  //CELL_SHAPE_VOXEL            = 11,
  CELL_SHAPE_HEXAHEDRON = lcl::ShapeId::HEXAHEDRON,
  CELL_SHAPE_WEDGE = lcl::ShapeId::WEDGE,
  CELL_SHAPE_PYRAMID = lcl::ShapeId::PYRAMID,

  NUMBER_OF_CELL_SHAPES
};

// If you wish to add cell shapes to this list, in addition to adding an index
// to the enum above, you at a minimum need to define an associated tag with
// VTKM_DEFINE_CELL_TAG and add a condition to the vtkmGenericCellShapeMacro.
// There are also many other cell-specific features that code might expect such
// as \c CellTraits and interpolations.

namespace internal
{

/// A class that can be used to determine if a class is a CellShapeTag or not.
/// The class will be either std::true_type or std::false_type.
///
template <typename T>
struct CellShapeTagCheck : std::false_type
{
};

/// Convert VTK-m tag to VTK-c tag
template <typename VtkmCellShapeTag>
struct CellShapeTagVtkmToVtkc;

} // namespace internal

/// Checks that the argument is a proper cell shape tag. This is a handy
/// concept check to make sure that a template argument is a proper cell shape
/// tag.
///
#define VTKM_IS_CELL_SHAPE_TAG(tag)                                                                \
  VTKM_STATIC_ASSERT_MSG(::vtkm::internal::CellShapeTagCheck<tag>::value,                          \
                         "Provided type is not a valid VTK-m cell shape tag.")

/// A traits-like class to get an CellShapeId known at compile time to a tag.
///
template <vtkm::IdComponent Id>
struct CellShapeIdToTag
{
  // If you get a compile error for this class about Id not being defined, that
  // probably means you are using an ID that does not have a defined cell
  // shape.

  using valid = std::false_type;
};

// Define a tag for each cell shape as well as the support structs to go
// between tags and ids. The following macro is only valid here.

#define VTKM_DEFINE_CELL_TAG(name, idname)                                                         \
  struct CellShapeTag##name                                                                        \
  {                                                                                                \
    static constexpr vtkm::UInt8 Id = vtkm::idname;                                                \
  };                                                                                               \
  namespace internal                                                                               \
  {                                                                                                \
  template <>                                                                                      \
  struct CellShapeTagCheck<vtkm::CellShapeTag##name> : std::true_type                              \
  {                                                                                                \
  };                                                                                               \
  template <>                                                                                      \
  struct CellShapeTagVtkmToVtkc<vtkm::CellShapeTag##name>                                          \
  {                                                                                                \
    using Type = lcl::name;                                                                        \
  };                                                                                               \
  }                                                                                                \
  static inline VTKM_EXEC_CONT const char* GetCellShapeName(vtkm::CellShapeTag##name)              \
  {                                                                                                \
    return #name;                                                                                  \
  }                                                                                                \
  template <>                                                                                      \
  struct CellShapeIdToTag<vtkm::idname>                                                            \
  {                                                                                                \
    using valid = std::true_type;                                                                  \
    using Tag = vtkm::CellShapeTag##name;                                                          \
  }

VTKM_DEFINE_CELL_TAG(Empty, CELL_SHAPE_EMPTY);
VTKM_DEFINE_CELL_TAG(Vertex, CELL_SHAPE_VERTEX);
//VTKM_DEFINE_CELL_TAG(PolyVertex, CELL_SHAPE_POLY_VERTEX);
VTKM_DEFINE_CELL_TAG(Line, CELL_SHAPE_LINE);
VTKM_DEFINE_CELL_TAG(PolyLine, CELL_SHAPE_POLY_LINE);
VTKM_DEFINE_CELL_TAG(Triangle, CELL_SHAPE_TRIANGLE);
//VTKM_DEFINE_CELL_TAG(TriangleStrip, CELL_SHAPE_TRIANGLE_STRIP);
VTKM_DEFINE_CELL_TAG(Polygon, CELL_SHAPE_POLYGON);
//VTKM_DEFINE_CELL_TAG(Pixel, CELL_SHAPE_PIXEL);
VTKM_DEFINE_CELL_TAG(Quad, CELL_SHAPE_QUAD);
VTKM_DEFINE_CELL_TAG(Tetra, CELL_SHAPE_TETRA);
//VTKM_DEFINE_CELL_TAG(Voxel, CELL_SHAPE_VOXEL);
VTKM_DEFINE_CELL_TAG(Hexahedron, CELL_SHAPE_HEXAHEDRON);
VTKM_DEFINE_CELL_TAG(Wedge, CELL_SHAPE_WEDGE);
VTKM_DEFINE_CELL_TAG(Pyramid, CELL_SHAPE_PYRAMID);

#undef VTKM_DEFINE_CELL_TAG

/// A special cell shape tag that holds a cell shape that is not known at
/// compile time. Unlike other cell set tags, the Id field is set at runtime
/// so its value cannot be used in template parameters. You need to use
/// \c vtkmGenericCellShapeMacro to specialize on the cell type.
///
struct CellShapeTagGeneric
{
  VTKM_EXEC_CONT
  CellShapeTagGeneric(vtkm::UInt8 shape)
    : Id(shape)
  {
  }

  vtkm::UInt8 Id;
};

namespace internal
{

template <typename VtkmCellShapeTag>
VTKM_EXEC_CONT inline typename CellShapeTagVtkmToVtkc<VtkmCellShapeTag>::Type make_VtkcCellShapeTag(
  const VtkmCellShapeTag&,
  vtkm::IdComponent numPoints = 0)
{
  using VtkcCellShapeTag = typename CellShapeTagVtkmToVtkc<VtkmCellShapeTag>::Type;
  static_cast<void>(numPoints); // unused
  return VtkcCellShapeTag{};
}

VTKM_EXEC_CONT
inline lcl::Polygon make_VtkcCellShapeTag(const vtkm::CellShapeTagPolygon&,
                                          vtkm::IdComponent numPoints = 0)
{
  return lcl::Polygon(numPoints);
}

VTKM_EXEC_CONT
inline lcl::Cell make_VtkcCellShapeTag(const vtkm::CellShapeTagGeneric& tag,
                                       vtkm::IdComponent numPoints = 0)
{
  return lcl::Cell(static_cast<std::int8_t>(tag.Id), numPoints);
}

} // namespace internal

#define vtkmGenericCellShapeMacroCase(cellShapeId, call)                                           \
  case vtkm::cellShapeId:                                                                          \
  {                                                                                                \
    using CellShapeTag = vtkm::CellShapeIdToTag<vtkm::cellShapeId>::Tag;                           \
    call;                                                                                          \
  }                                                                                                \
  break

/// \brief A macro used in a \c switch statement to determine cell shape.
///
/// The \c vtkmGenericCellShapeMacro is a series of case statements for all
/// of the cell shapes supported by VTK-m. This macro is intended to be used
/// inside of a switch statement on a cell type. For each cell shape condition,
/// a \c CellShapeTag typedef is created and the given \c call is executed.
///
/// A typical use case of this class is to create the specialization of a
/// function overloaded on a cell shape tag for the generic cell shape like as
/// following.
///
/// \code{.cpp}
/// template<typename WorkletType>
/// VTKM_EXEC
/// void MyCellOperation(vtkm::CellShapeTagGeneric cellShape,
///                      const vtkm::exec::FunctorBase &worklet)
/// {
///   switch(cellShape.CellShapeId)
///   {
///     vtkmGenericCellShapeMacro(
///       MyCellOperation(CellShapeTag())
///       );
///     default: worklet.RaiseError("Encountered unknown cell shape."); break
///   }
/// }
/// \endcode
///
/// Note that \c vtkmGenericCellShapeMacro does not have a default case. You
/// should consider adding one that gives a
///
#define vtkmGenericCellShapeMacro(call)                                                            \
  vtkmGenericCellShapeMacroCase(CELL_SHAPE_EMPTY, call);                                           \
  vtkmGenericCellShapeMacroCase(CELL_SHAPE_VERTEX, call);                                          \
  vtkmGenericCellShapeMacroCase(CELL_SHAPE_LINE, call);                                            \
  vtkmGenericCellShapeMacroCase(CELL_SHAPE_POLY_LINE, call);                                       \
  vtkmGenericCellShapeMacroCase(CELL_SHAPE_TRIANGLE, call);                                        \
  vtkmGenericCellShapeMacroCase(CELL_SHAPE_POLYGON, call);                                         \
  vtkmGenericCellShapeMacroCase(CELL_SHAPE_QUAD, call);                                            \
  vtkmGenericCellShapeMacroCase(CELL_SHAPE_TETRA, call);                                           \
  vtkmGenericCellShapeMacroCase(CELL_SHAPE_HEXAHEDRON, call);                                      \
  vtkmGenericCellShapeMacroCase(CELL_SHAPE_WEDGE, call);                                           \
  vtkmGenericCellShapeMacroCase(CELL_SHAPE_PYRAMID, call)

} // namespace vtkm

#endif //vtk_m_CellShape_h
