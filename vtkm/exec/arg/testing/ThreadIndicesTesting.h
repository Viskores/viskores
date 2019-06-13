//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_testing_ThreadIndicesTesting_h
#define vtk_m_exec_arg_testing_ThreadIndicesTesting_h

#include <vtkm/Types.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief Simplified version of ThreadIndices for unit testing purposes
///
class ThreadIndicesTesting
{
public:
  VTKM_EXEC_CONT
  ThreadIndicesTesting(vtkm::Id index)
    : InputIndex(index)
    , OutputIndex(index)
    , VisitIndex(0)
  {
  }

  VTKM_EXEC_CONT
  ThreadIndicesTesting(vtkm::Id inputIndex, vtkm::Id outputIndex, vtkm::IdComponent visitIndex)
    : InputIndex(inputIndex)
    , OutputIndex(outputIndex)
    , VisitIndex(visitIndex)
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetInputIndex() const { return this->InputIndex; }

  VTKM_EXEC_CONT
  vtkm::Id3 GetInputIndex3D() const { return vtkm::Id3(this->GetInputIndex(), 0, 0); }

  VTKM_EXEC_CONT
  vtkm::Id GetOutputIndex() const { return this->OutputIndex; }

  VTKM_EXEC_CONT
  vtkm::IdComponent GetVisitIndex() const { return this->VisitIndex; }

  VTKM_EXEC_CONT
  vtkm::Id GetGlobalIndex() const { return this->OutputIndex; }

private:
  vtkm::Id InputIndex;
  vtkm::Id OutputIndex;
  vtkm::IdComponent VisitIndex;
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_testing_ThreadIndicesTesting_h
