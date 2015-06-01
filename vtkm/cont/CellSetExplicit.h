#ifndef vtk_m_cont_CellSetExplicit_h
#define vtk_m_cont_CellSetExplicit_h

#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/ExplicitConnectivity.h>

namespace vtkm {
namespace cont {

template<typename ShapeStorageTag         = VTKM_DEFAULT_STORAGE_TAG,
         typename IndiceStorageTag        = VTKM_DEFAULT_STORAGE_TAG,
         typename ConnectivityStorageTag  = VTKM_DEFAULT_STORAGE_TAG >
class CellSetExplicit : public CellSet
{
public:
  typedef ExplicitConnectivity<ShapeStorageTag,
                               IndiceStorageTag,
                               ConnectivityStorageTag
                               > ExplicitConnectivityType;

  CellSetExplicit(const std::string &n, int d)
    : CellSet(n,d)
  {
  }

  virtual int GetNumCells()
  {
    return nodesOfCellsConnectivity.GetNumberOfElements();
  }

  ExplicitConnectivityType &GetNodeToCellConnectivity()
  {
    return nodesOfCellsConnectivity;
  }

  virtual void PrintSummary(std::ostream &out)
  {
      out<<"   ExplicitCellSet: "<<name<<" dim= "<<dimensionality<<std::endl;
      nodesOfCellsConnectivity.PrintSummary(out);
  }

public:
  ExplicitConnectivityType nodesOfCellsConnectivity;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetExplicit_h
