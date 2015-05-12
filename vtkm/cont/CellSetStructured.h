#ifndef vtk_m_cont_CellSetStructured_h
#define vtk_m_cont_CellSetStructured_h

#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/RegularConnectivity.h>

namespace vtkm {
namespace cont {

class CellSetStructured : public CellSet
{
  public:

  CellSetStructured(const std::string &n)
    : CellSet(n,3)
  {
  }


  virtual int GetNumCells()
  {
   return structure.GetNumberOfElements();
  }

  RegularConnectivity structure;

};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetStructured_h
