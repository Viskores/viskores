//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_source_Source_h
#define vtk_m_source_Source_h

#include <vtkm/cont/DataSet.h>

namespace vtkm
{
namespace source
{

class Source
{
public:
  VTKM_CONT
  Source() {}

  VTKM_CONT
  ~Source() {}

  //VTKM_CONT vtkm::cont::DataSet Execute();
};

} // namespace source
} // namespace vtkm

#endif // vtk_m_source_Source_h
