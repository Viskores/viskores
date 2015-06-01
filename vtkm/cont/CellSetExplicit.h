#ifndef vtk_m_cont_CellSetExplicit_h
#define vtk_m_cont_CellSetExplicit_h

#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/ExplicitConnectivity.h>

namespace vtkm {
namespace cont {

class CellSetExplicit : public CellSet
{
public:
  //CellSetExplicit() : CellSet("", 0)
  //{
  //}

  CellSetExplicit(const std::string &n, int d)
    : CellSet(n,d)
  {
  }

  virtual int GetNumCells()
  {
    return nodesOfCellsConnectivity.GetNumberOfElements();
  }

  ExplicitConnectivity<> &GetNodeToCellConnectivity()
  {
    return nodesOfCellsConnectivity;
  }

  virtual void PrintSummary(std::ostream &out)
  {
      out<<"   ExplicitCellSet: "<<name<<" dim= "<<dimensionality<<std::endl;
      nodesOfCellsConnectivity.PrintSummary(out);
  }

public:
  ExplicitConnectivity<> nodesOfCellsConnectivity;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetExplicit_h
