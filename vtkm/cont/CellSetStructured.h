#ifndef vtk_m_cont_CellSetStructured_h
#define vtk_m_cont_CellSetStructured_h

#include <vtkm/cont/CellSet.h>
#include <vtkm/RegularConnectivity.h>
#include <vtkm/RegularStructure.h>

namespace vtkm {
namespace cont {



template<vtkm::IdComponent DIMENSION>
class CellSetStructured : public CellSet
{
public:
  static const int Dimension=DIMENSION;

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
    typedef vtkm::RegularConnectivity<vtkm::cont::NODE,
                                      vtkm::cont::CELL,
                                      Dimension> NodeToCellConnectivity;
    return NodeToCellConnectivity(structure);
  }

  vtkm::RegularConnectivity<vtkm::cont::CELL,vtkm::cont::NODE,Dimension>
  GetCellToNodeConnectivity()
  {
    typedef vtkm::RegularConnectivity<vtkm::cont::CELL,
                                      vtkm::cont::NODE,
                                      Dimension> CellToNodeConnectivity;
    return CellToNodeConnectivity(structure);
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
