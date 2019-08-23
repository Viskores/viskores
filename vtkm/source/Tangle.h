//
// Created by ollie on 8/22/19.
//

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

  VTKM_CONT_EXPORT
  vtkm::cont::DataSet Execute() const;

private:
  vtkm::Id3 Dims;
};
} //namespace source
} //namespace vtkm

#endif //VTKM_TANGLE_H
