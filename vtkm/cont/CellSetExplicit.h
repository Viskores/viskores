#ifndef vtk_m_cont_CellSetExplicit_h
#define vtk_m_cont_CellSetExplicit_h

#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/ExplicitConnectivity.h>

namespace vtkm {
namespace cont {

class CellSetExplicit
{
  public:
    ExplicitConnectivity nodesOfCellsConnectivity;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetExplicit_h
