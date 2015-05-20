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

  vtkm::RegularConnectivity<vtkm::cont::NODE,vtkm::cont::CELL,Dimension> 
  GetNodeToCellConnectivity()
  {
    vtkm::RegularConnectivity<vtkm::cont::NODE,vtkm::cont::CELL,Dimension> regConn;
    regConn.SetNodeDimension(structure.nodeDims[0],
			     structure.nodeDims[1],
			     structure.nodeDims[2]);
    return regConn;
  }

  vtkm::RegularConnectivity<vtkm::cont::CELL,vtkm::cont::NODE,Dimension> 
  GetCellToNodeConnectivity()
  {
    vtkm::RegularConnectivity<vtkm::cont::CELL,vtkm::cont::NODE,Dimension> regConn;
    regConn.SetNodeDimension(structure.nodeDims[0],
			     structure.nodeDims[1],
			     structure.nodeDims[2]);
    return regConn;
  }

  virtual void PrintSummary(std::ostream &out)
  {
      out<<"  StructuredCellSet: "<<name<<" dim= "<<dimensionality<<std::endl;
      structure.PrintSummary(out);
  }

public:
  vtkm::RegularStructure<Dimension> structure;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetStructured_h
