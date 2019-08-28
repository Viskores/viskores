//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_source_Tangle_h
#define vtk_m_source_Tangle_h

#include <vtkm/source/Source.h>

namespace vtkm
{
namespace source
{
class Tangle : public vtkm::source::Source
{
public:
  VTKM_CONT
  Tangle(vtkm::Id3 dims)
    : Dims(dims)
  {
  }

  VTKM_SOURCE_EXPORT
  vtkm::cont::DataSet Execute() const;

private:
  vtkm::Id3 Dims;
};
} //namespace source
} //namespace vtkm

#endif //VTKM_TANGLE_H
