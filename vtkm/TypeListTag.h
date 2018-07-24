//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_TypeListTag_h
#define vtk_m_TypeListTag_h

#ifndef VTKM_DEFAULT_TYPE_LIST_TAG
#define VTKM_DEFAULT_TYPE_LIST_TAG ::vtkm::TypeListTagCommon
#endif

#include <vtkm/ListTag.h>
#include <vtkm/Types.h>

namespace vtkm
{

/// A list containing the type vtkm::Id.
///
struct TypeListTagId : vtkm::ListTagBase<vtkm::Id>
{
};

/// A list containing the type vtkm::Id2.
///
struct TypeListTagId2 : vtkm::ListTagBase<vtkm::Id2>
{
};

/// A list containing the type vtkm::Id3.
///
struct TypeListTagId3 : vtkm::ListTagBase<vtkm::Id3>
{
};

/// A list containing the type vtkm::IdComponent
///
struct TypeListTagIdComponent : vtkm::ListTagBase<vtkm::IdComponent>
{
};

/// A list containing types used to index arrays. Contains vtkm::Id, vtkm::Id2,
/// and vtkm::Id3.
///
struct TypeListTagIndex : vtkm::ListTagBase<vtkm::Id, vtkm::Id2, vtkm::Id3>
{
};

/// A list containing types used for scalar fields. Specifically, contains
/// floating point numbers of different widths (i.e. vtkm::Float32 and
/// vtkm::Float64).
struct TypeListTagFieldScalar : vtkm::ListTagBase<vtkm::Float32, vtkm::Float64>
{
};

/// A list containing types for values for fields with two dimensional
/// vectors.
///
struct TypeListTagFieldVec2
  : vtkm::ListTagBase<vtkm::Vec<vtkm::Float32, 2>, vtkm::Vec<vtkm::Float64, 2>>
{
};

/// A list containing types for values for fields with three dimensional
/// vectors.
///
struct TypeListTagFieldVec3
  : vtkm::ListTagBase<vtkm::Vec<vtkm::Float32, 3>, vtkm::Vec<vtkm::Float64, 3>>
{
};

/// A list containing types for values for fields with four dimensional
/// vectors.
///
struct TypeListTagFieldVec4
  : vtkm::ListTagBase<vtkm::Vec<vtkm::Float32, 4>, vtkm::Vec<vtkm::Float64, 4>>
{
};

/// A list containing common types for floating-point vectors. Specifically contains
/// floating point vectors of size 2, 3, and 4 with floating point components.
/// Scalars are not included.
///
struct TypeListTagFloatVec : vtkm::ListTagBase<vtkm::Vec<vtkm::Float32, 2>,
                                               vtkm::Vec<vtkm::Float64, 2>,
                                               vtkm::Vec<vtkm::Float32, 3>,
                                               vtkm::Vec<vtkm::Float64, 3>,
                                               vtkm::Vec<vtkm::Float32, 4>,
                                               vtkm::Vec<vtkm::Float64, 4>>
{
};

/// A list containing common types for values in fields. Specifically contains
/// floating point scalars and vectors of size 2, 3, and 4 with floating point
/// components.
///
struct TypeListTagField : vtkm::ListTagBase<vtkm::Float32,
                                            vtkm::Float64,
                                            vtkm::Vec<vtkm::Float32, 2>,
                                            vtkm::Vec<vtkm::Float64, 2>,
                                            vtkm::Vec<vtkm::Float32, 3>,
                                            vtkm::Vec<vtkm::Float64, 3>,
                                            vtkm::Vec<vtkm::Float32, 4>,
                                            vtkm::Vec<vtkm::Float64, 4>>
{
};

/// A list of all scalars defined in vtkm/Types.h. A scalar is a type that
/// holds a single number.
///
struct TypeListTagScalarAll : vtkm::ListTagBase<vtkm::Int8,
                                                vtkm::UInt8,
                                                vtkm::Int16,
                                                vtkm::UInt16,
                                                vtkm::Int32,
                                                vtkm::UInt32,
                                                vtkm::Int64,
                                                vtkm::UInt64,
                                                vtkm::Float32,
                                                vtkm::Float64>
{
};

/// A list of the most commonly use Vec classes. Specifically, these are
/// vectors of size 2, 3, or 4 containing either unsigned bytes, signed
/// integers of 32 or 64 bits, or floating point values of 32 or 64 bits.
///
struct TypeListTagVecCommon : vtkm::ListTagBase<vtkm::Vec<vtkm::UInt8, 2>,
                                                vtkm::Vec<vtkm::Int32, 2>,
                                                vtkm::Vec<vtkm::Int64, 2>,
                                                vtkm::Vec<vtkm::Float32, 2>,
                                                vtkm::Vec<vtkm::Float64, 2>,
                                                vtkm::Vec<vtkm::UInt8, 3>,
                                                vtkm::Vec<vtkm::Int32, 3>,
                                                vtkm::Vec<vtkm::Int64, 3>,
                                                vtkm::Vec<vtkm::Float32, 3>,
                                                vtkm::Vec<vtkm::Float64, 3>,
                                                vtkm::Vec<vtkm::UInt8, 4>,
                                                vtkm::Vec<vtkm::Int32, 4>,
                                                vtkm::Vec<vtkm::Int64, 4>,
                                                vtkm::Vec<vtkm::Float32, 4>,
                                                vtkm::Vec<vtkm::Float64, 4>>
{
};

namespace internal
{

/// A list of uncommon Vec classes with length up to 4. This is not much
/// use in general, but is used when joined with \c TypeListTagVecCommon
/// to get a list of all vectors up to size 4.
///
struct TypeListTagVecUncommon : vtkm::ListTagBase<vtkm::Vec<vtkm::Int8, 2>,
                                                  vtkm::Vec<vtkm::Int16, 2>,
                                                  vtkm::Vec<vtkm::UInt16, 2>,
                                                  vtkm::Vec<vtkm::UInt32, 2>,
                                                  vtkm::Vec<vtkm::UInt64, 2>,
                                                  vtkm::Vec<vtkm::Int8, 3>,
                                                  vtkm::Vec<vtkm::Int16, 3>,
                                                  vtkm::Vec<vtkm::UInt16, 3>,
                                                  vtkm::Vec<vtkm::UInt32, 3>,
                                                  vtkm::Vec<vtkm::UInt64, 3>,
                                                  vtkm::Vec<vtkm::Int8, 4>,
                                                  vtkm::Vec<vtkm::Int16, 4>,
                                                  vtkm::Vec<vtkm::UInt16, 4>,
                                                  vtkm::Vec<vtkm::UInt32, 4>,
                                                  vtkm::Vec<vtkm::UInt64, 4>>
{
};

} // namespace internal

/// A list of all vector classes with standard types as components and
/// lengths between 2 and 4.
///
struct TypeListTagVecAll
  : vtkm::ListTagJoin<vtkm::TypeListTagVecCommon, vtkm::internal::TypeListTagVecUncommon>
{
};

/// A list of all basic types listed in vtkm/Types.h. Does not include all
/// possible VTK-m types like arbitrarily typed and sized Vecs (only up to
/// length 4) or math types like matrices.
///
struct TypeListTagAll : vtkm::ListTagJoin<vtkm::TypeListTagScalarAll, vtkm::TypeListTagVecAll>
{
};

/// A list of the most commonly used types across multiple domains. Includes
/// integers, floating points, and 3 dimensional vectors of floating points.
///
struct TypeListTagCommon : vtkm::ListTagBase<vtkm::Int32,
                                             vtkm::Int64,
                                             vtkm::Float32,
                                             vtkm::Float64,
                                             vtkm::Vec<vtkm::Float32, 3>,
                                             vtkm::Vec<vtkm::Float64, 3>>
{
};

// Special implementation of ListContains for TypeListTagAll to always be
// true. Although TypeListTagAll is necessarily finite, the point is to
// be all inclusive. Besides, this should speed up the compilation when
// checking a list that should contain everything.
template <typename Type>
struct ListContains<vtkm::TypeListTagAll, Type>
{
  static constexpr bool value = true;
};

} // namespace vtkm

#endif //vtk_m_TypeListTag_h
