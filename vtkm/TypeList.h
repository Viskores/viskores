//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_TypeList_h
#define vtk_m_TypeList_h

#ifndef VTKM_DEFAULT_TYPE_LIST
#define VTKM_DEFAULT_TYPE_LIST ::vtkm::TypeListCommon
#endif

#include <vtkm/List.h>
#include <vtkm/Types.h>

namespace vtkm
{

/// A list containing the type vtkm::Id.
///
using TypeListId = vtkm::List<vtkm::Id>;

/// A list containing the type vtkm::Id2.
///
using TypeListId2 = vtkm::List<vtkm::Id2>;

/// A list containing the type vtkm::Id3.
///
using TypeListId3 = vtkm::List<vtkm::Id3>;

/// A list containing the type vtkm::Id4.
///
using TypeListId4 = vtkm::List<vtkm::Id4>;

/// A list containing the type vtkm::IdComponent
///
using TypeListIdComponent = vtkm::List<vtkm::IdComponent>;

/// A list containing types used to index arrays. Contains vtkm::Id, vtkm::Id2,
/// and vtkm::Id3.
///
using TypeListIndex = vtkm::List<vtkm::Id, vtkm::Id2, vtkm::Id3>;

/// A list containing types used for scalar fields. Specifically, contains
/// floating point numbers of different widths (i.e. vtkm::Float32 and
/// vtkm::Float64).
using TypeListFieldScalar = vtkm::List<vtkm::Float32, vtkm::Float64>;

/// A list containing types for values for fields with two dimensional
/// vectors.
///
using TypeListFieldVec2 = vtkm::List<vtkm::Vec2f_32, vtkm::Vec2f_64>;

/// A list containing types for values for fields with three dimensional
/// vectors.
///
using TypeListFieldVec3 = vtkm::List<vtkm::Vec3f_32, vtkm::Vec3f_64>;

/// A list containing types for values for fields with four dimensional
/// vectors.
///
using TypeListFieldVec4 = vtkm::List<vtkm::Vec4f_32, vtkm::Vec4f_64>;

/// A list containing common types for floating-point vectors. Specifically contains
/// floating point vectors of size 2, 3, and 4 with floating point components.
/// Scalars are not included.
///
using TypeListFloatVec = vtkm::List<vtkm::Vec2f_32,
                                    vtkm::Vec2f_64,
                                    vtkm::Vec3f_32,
                                    vtkm::Vec3f_64,
                                    vtkm::Vec4f_32,
                                    vtkm::Vec4f_64>;

/// A list containing common types for values in fields. Specifically contains
/// floating point scalars and vectors of size 2, 3, and 4 with floating point
/// components.
///
using TypeListField = vtkm::List<vtkm::Float32,
                                 vtkm::Float64,
                                 vtkm::Vec2f_32,
                                 vtkm::Vec2f_64,
                                 vtkm::Vec3f_32,
                                 vtkm::Vec3f_64,
                                 vtkm::Vec4f_32,
                                 vtkm::Vec4f_64>;

/// A list of all scalars defined in vtkm/Types.h. A scalar is a type that
/// holds a single number.
///
using TypeListScalarAll = vtkm::List<vtkm::Int8,
                                     vtkm::UInt8,
                                     vtkm::Int16,
                                     vtkm::UInt16,
                                     vtkm::Int32,
                                     vtkm::UInt32,
                                     vtkm::Int64,
                                     vtkm::UInt64,
                                     vtkm::Float32,
                                     vtkm::Float64>;

/// A list of the most commonly use Vec classes. Specifically, these are
/// vectors of size 2, 3, or 4 containing either unsigned bytes, signed
/// integers of 32 or 64 bits, or floating point values of 32 or 64 bits.
///
using TypeListVecCommon = vtkm::List<vtkm::Vec2ui_8,
                                     vtkm::Vec2i_32,
                                     vtkm::Vec2i_64,
                                     vtkm::Vec2f_32,
                                     vtkm::Vec2f_64,
                                     vtkm::Vec3ui_8,
                                     vtkm::Vec3i_32,
                                     vtkm::Vec3i_64,
                                     vtkm::Vec3f_32,
                                     vtkm::Vec3f_64,
                                     vtkm::Vec4ui_8,
                                     vtkm::Vec4i_32,
                                     vtkm::Vec4i_64,
                                     vtkm::Vec4f_32,
                                     vtkm::Vec4f_64>;

namespace internal
{

/// A list of uncommon Vec classes with length up to 4. This is not much
/// use in general, but is used when joined with \c TypeListVecCommon
/// to get a list of all vectors up to size 4.
///
using TypeListVecUncommon = vtkm::List<vtkm::Vec2i_8,
                                       vtkm::Vec2i_16,
                                       vtkm::Vec2ui_16,
                                       vtkm::Vec2ui_32,
                                       vtkm::Vec2ui_64,
                                       vtkm::Vec3i_8,
                                       vtkm::Vec3i_16,
                                       vtkm::Vec3ui_16,
                                       vtkm::Vec3ui_32,
                                       vtkm::Vec3ui_64,
                                       vtkm::Vec4i_8,
                                       vtkm::Vec4i_16,
                                       vtkm::Vec4ui_16,
                                       vtkm::Vec4ui_32,
                                       vtkm::Vec4ui_64>;

} // namespace internal

/// A list of all vector classes with standard types as components and
/// lengths between 2 and 4.
///
using TypeListVecAll =
  vtkm::ListAppend<vtkm::TypeListVecCommon, vtkm::internal::TypeListVecUncommon>;

/// A list of all basic types listed in vtkm/Types.h. Does not include all
/// possible VTK-m types like arbitrarily typed and sized Vecs (only up to
/// length 4) or math types like matrices.
///
using TypeListAll = vtkm::ListAppend<vtkm::TypeListScalarAll, vtkm::TypeListVecAll>;

/// A list of the most commonly used types across multiple domains. Includes
/// integers, floating points, and 3 dimensional vectors of floating points.
///
using TypeListCommon = vtkm::List<vtkm::UInt8,
                                  vtkm::Int32,
                                  vtkm::Int64,
                                  vtkm::Float32,
                                  vtkm::Float64,
                                  vtkm::Vec3f_32,
                                  vtkm::Vec3f_64>;

} // namespace vtkm

#endif //vtk_m_TypeList_h
