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
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_worklet_ScatterUniform_h
#define vtk_m_worklet_ScatterUniform_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleImplicit.h>

namespace vtkm {
namespace worklet {

namespace detail {

struct FunctorModulus
{
  vtkm::IdComponent Modulus;

  VTKM_EXEC_CONT_EXPORT
  FunctorModulus(vtkm::IdComponent modulus = 1)
    : Modulus(modulus)
  {  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent operator()(vtkm::Id index) const
  {
    return static_cast<vtkm::IdComponent>(index % this->Modulus);
  }
};

struct FunctorDiv
{
  vtkm::Id Divisor;

  VTKM_EXEC_CONT_EXPORT
  FunctorDiv(vtkm::Id divisor = 1)
    : Divisor(divisor)
  {  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id operator()(vtkm::Id index) const
  {
    return index / this->Divisor;
  }
};

}

/// \brief A scatter that maps input to some constant numbers of output.
///
/// The \c Scatter classes are responsible for defining how much output is
/// generated based on some sized input. \c ScatterUniform establishes a 1 to N
/// mapping from input to output. That is, every input element generates N
/// elements associated with it where N is the same for every input. The output
/// elements are grouped by the input associated.
///
struct ScatterUniform
{
  VTKM_CONT_EXPORT
  ScatterUniform(vtkm::IdComponent numOutputsPerInput)
    : NumOutputsPerInput(numOutputsPerInput)
  {  }

  VTKM_CONT_EXPORT
  vtkm::Id GetOutputRange(vtkm::Id inputRange) const
  {
    return inputRange * this->NumOutputsPerInput;
  }
  VTKM_CONT_EXPORT
  vtkm::Id GetOutputRange(vtkm::Id3 inputRange) const
  {
    return this->GetOutputRange(inputRange[0]*inputRange[1]*inputRange[2]);
  }

  typedef vtkm::cont::ArrayHandleImplicit<vtkm::Id, detail::FunctorDiv>
      OutputToInputMapType;
  template<typename RangeType>
  VTKM_CONT_EXPORT
  OutputToInputMapType GetOutputToInputMap(RangeType inputRange) const
  {
    return OutputToInputMapType(detail::FunctorDiv(this->NumOutputsPerInput),
                                this->GetOutputRange(inputRange));
  }

  typedef vtkm::cont::ArrayHandleImplicit<vtkm::IdComponent, detail::FunctorModulus>
      VisitArrayType;
  template<typename RangeType>
  VTKM_CONT_EXPORT
  VisitArrayType GetVisitArray(RangeType inputRange) const
  {
    return VisitArrayType(detail::FunctorModulus(this->NumOutputsPerInput),
                          this->GetOutputRange(inputRange));
  }

private:
  vtkm::IdComponent NumOutputsPerInput;
};

}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_ScatterUniform_h
