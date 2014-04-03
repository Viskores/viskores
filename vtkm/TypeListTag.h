//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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

namespace vtkm {

struct TypeListTagId : vtkm::ListTagBase<vtkm::Id> { };
struct TypeListTagId2 : vtkm::ListTagBase<vtkm::Id2> { };
struct TypeListTagId3 : vtkm::ListTagBase<vtkm::Id3> { };
struct TypeListTagScalar : vtkm::ListTagBase<vtkm::Scalar> { };
struct TypeListTagVector2 : vtkm::ListTagBase<vtkm::Vector2> { };
struct TypeListTagVector3 : vtkm::ListTagBase<vtkm::Vector3> { };
struct TypeListTagVector4 : vtkm::ListTagBase<vtkm::Vector4> { };

struct TypeListTagIndex
    : vtkm::ListTagBase3<vtkm::Id,vtkm::Id2,vtkm::Id3> { };

struct TypeListTagReal
    : vtkm::ListTagBase4<vtkm::Scalar,vtkm::Vector2,vtkm::Vector3,vtkm::Vector4>
{ };

/// A list of all basic types listed in vtkm/Types.h. Does not include all
/// possible VTK-m types like arbitrarily typed and sized tuples or math
/// types like matrices.
///
struct TypeListTagAll
    : vtkm::ListTagJoin<vtkm::TypeListTagIndex, vtkm::TypeListTagReal>
{ };

/// A list of the most commonly used types across multiple domains. Includes
/// Id, Scalar, and Vector3.
///
struct TypeListTagCommon
    : vtkm::ListTagBase3<vtkm::Id, vtkm::Scalar, vtkm::Vector3>
{ };

} // namespace vtkm

#endif //vtk_m_TypeListTag_h
