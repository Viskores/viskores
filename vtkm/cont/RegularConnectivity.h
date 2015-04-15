#ifndef vtk_m_cont_RegularConnectivity_h
#define vtk_m_cont_RegularConnectivity_h

#include <vtkm/CellType.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace vtkm {
namespace cont {

class RegularConnectivity
{
public:
  void SetNodeDimension(int node_i, int node_j, int node_k)
  {
    cellDims[0] = node_i-1;
    cellDims[1] = node_j-1;
    cellDims[2] = node_k-1;
    nodeDims[0] = node_i;
    nodeDims[1] = node_j;
    nodeDims[2] = node_k;
  }

  vtkm::Id GetNumberOfElements()
  {
    return cellDims[0]*cellDims[1]*cellDims[2];
  }
  vtkm::Id GetNumberOfIndices(vtkm::Id)
  {
    return 8;
  }
  vtkm::Id GetElementShapeType(vtkm::Id)
  {
    return VTKM_VOXEL;
  }
  template <vtkm::IdComponent ItemTupleLength>
  void GetIndices(vtkm::Id index, vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    int i,j,k;
    CalculateLogicalCellIndices(index, i,j,k);
    ///\todo: assert ItemTupleLength >= 8, or return early?
    ids[0] = CalculateNodeIndex(i, j, k);
    if (ItemTupleLength <= 1) return;
    ids[1] = ids[0] + 1;
    if (ItemTupleLength <= 2) return;
    ids[2] = ids[0] + nodeDims[0];
    if (ItemTupleLength <= 3) return;
    ids[3] = ids[2] + 1;
    if (ItemTupleLength <= 4) return;
    ids[4] = ids[0] + nodeDims[0]*nodeDims[1];
    if (ItemTupleLength <= 5) return;
    ids[5] = ids[4] + 1;
    if (ItemTupleLength <= 6) return;
    ids[6] = ids[4] + nodeDims[0];
    if (ItemTupleLength <= 7) return;
    ids[7] = ids[6] + 1;
  }
private:
  int cellDims[3];
  int nodeDims[3];
  int CalculateCellIndex(int i, int j, int k)
  {
    return (k * cellDims[1] + j) * cellDims[0] + i;
  }
  int CalculateNodeIndex(int i, int j, int k)
  {
    return (k * nodeDims[1] + j) * nodeDims[0] + i;
  }
  void CalculateLogicalCellIndices(int index, int &i, int &j, int &k)
  {
    int cellDims01 = cellDims[0] * cellDims[1];
    k = index / cellDims01;
    int indexij = index % cellDims01;
    j = indexij / cellDims[0];
    i = indexij % cellDims[0];
  }
  void CalculateLogicalNodeIndices(int index, int &i, int &j, int &k)
  {
    int nodeDims01 = nodeDims[0] * nodeDims[1];
    k = index / nodeDims01;
    int indexij = index % nodeDims01;
    j = indexij / nodeDims[0];
    i = indexij % nodeDims[0];
  }
};

//TODO:
//Add specialized 1D and 2D versions.
    
}
} // namespace vtkm::cont

#endif //vtk_m_cont_RegularConnectivity_h
