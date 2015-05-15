#ifndef vtk_m_cont_CellSetStructured_h
#define vtk_m_cont_CellSetStructured_h

#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/RegularConnectivity.h>
#include <vtkm/cont/RegularStructure.h>

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
    return regStruct.GetNumberOfCells();
  }

  vtkm::cont::RegularConnectivity<NODE,CELL,Dimension> 
  GetNodeToCellConnectivity()
  {
    vtkm::cont::RegularConnectivity<NODE,CELL,Dimension> regConn;
    regConn.SetNodeDimension(regStruct.nodeDims.Max[0],
                             regStruct.nodeDims.Max[1],
                             regStruct.nodeDims.Max[2]);
    return regConn;
  }

public:
  vtkm::cont::RegularStructure<Dimension> regStruct;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetStructured_h
