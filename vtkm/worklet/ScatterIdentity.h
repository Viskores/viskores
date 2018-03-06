//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_worklet_ScatterIdentity_h
#define vtk_m_worklet_ScatterIdentity_h

#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleIndex.h>

namespace vtkm
{
namespace worklet
{

/// \brief A scatter that maps input directly to output.
///
/// The \c Scatter classes are responsible for defining how much output is
/// generated based on some sized input. \c ScatterIdentity establishes a 1 to
/// 1 mapping from input to output (and vice versa). That is, every input
/// element generates one output element associated with it. This is the
/// default for basic maps.
///
struct ScatterIdentity
{
  using OutputToInputMapType = vtkm::cont::ArrayHandleIndex;
  VTKM_CONT
  OutputToInputMapType GetOutputToInputMap(vtkm::Id inputRange) const
  {
    return OutputToInputMapType(inputRange);
  }
  VTKM_CONT
  OutputToInputMapType GetOutputToInputMap(vtkm::Id3 inputRange) const
  {
    return this->GetOutputToInputMap(inputRange[0] * inputRange[1] * inputRange[2]);
  }

  using VisitArrayType = vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>;
  VTKM_CONT
  VisitArrayType GetVisitArray(vtkm::Id inputRange) const { return VisitArrayType(0, inputRange); }
  VTKM_CONT
  VisitArrayType GetVisitArray(vtkm::Id3 inputRange) const
  {
    return this->GetVisitArray(inputRange[0] * inputRange[1] * inputRange[2]);
  }

  template <typename RangeType>
  VTKM_CONT RangeType GetOutputRange(RangeType inputRange) const
  {
    return inputRange;
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_ScatterIdentity_h
