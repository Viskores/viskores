#ifndef vtk_m_cont_CellSetStructured_h
#define vtk_m_cont_CellSetStructured_h

#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/RegularConnectivity.h>

namespace vtkm {
namespace cont {


template<vtkm::IdComponent Dimension>
class CellSetStructured : public CellSet
{
  public:

  CellSetStructured(const std::string &n)
    : CellSet(n,Dimension)
  {
  }


  virtual vtkm::Id GetNumCells()
  {
    return regConn.GetNumberOfElements();
  }

  vtkm::cont::RegularConnectivity<Dimension> regConn;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetStructured_h
