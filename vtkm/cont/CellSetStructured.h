#ifndef vtk_m_cont_CellSetStructured_h
#define vtk_m_cont_CellSetStructured_h

#include <vtkm/cont/CellSet.h>
#include <vtkm/RegularConnectivity.h>
#include <vtkm/RegularStructure.h>

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
    return structure.GetNumberOfCells();
  }

  vtkm::RegularConnectivity<NODE,CELL,Dimension> 
  GetNodeToCellConnectivity()
  {
    vtkm::RegularConnectivity<NODE,CELL,Dimension> regConn;
    regConn.SetNodeDimension(structure.nodeDims.Max[0],
                             structure.nodeDims.Max[1],
                             structure.nodeDims.Max[2]);
    return regConn;
  }

public:
  vtkm::RegularStructure<Dimension> structure;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetStructured_h
