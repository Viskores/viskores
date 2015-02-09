#ifndef vtk_m_cont_DataSet_h
#define vtk_m_cont_DataSet_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DynamicArrayHandle.h>

namespace vtkm {
namespace cont {

enum CellShape
{
    VTKM_POINT,
    VTKM_BEAM,
    VTKM_TRI,
    VTKM_QUAD,
    VTKM_PIXEL,
    VTKM_TET,
    VTKM_PYRAMID,
    VTKM_WEDGE,
    VTKM_HEX,
    VTKM_VOXEL,
    VTKM_TRISTRIP,
    VTKM_POLYGON,
    VTKM_OTHER
};


class ExplicitConnectivity
{
public:
  ExplicitConnectivity() {}

  vtkm::Id GetNumberOfElements()
  {
    return Shapes.GetNumberOfValues();
  }
  vtkm::Id GetNumberOfIndices(vtkm::Id index)
  {
    return NumIndices.GetPortalControl().Get(index);
  }
  vtkm::Id GetElementShapeType(vtkm::Id index)
  {
    return Shapes.GetPortalControl().Get(index);
  }
  template <vtkm::IdComponent ItemTupleLength>
  void GetIndices(vtkm::Id index, vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    int n = GetNumberOfIndices(index);
    int start = MapCellToConnectivityIndex.GetPortalControl().Get(index);
    for (int i=0; i<n && i<ItemTupleLength; i++)
      ids[i] = Connectivity.GetPortalControl().Get(start+i);
  }

  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> Shapes;
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> NumIndices;
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> Connectivity;
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> MapCellToConnectivityIndex;
};
    
class RegularConnectivity3D
{
public:
  void SetNodeDimension3D(int node_i, int node_j, int node_k)
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
    CalculateLogicalCellIndices3D(index, i,j,k);
    ///\todo: assert ItemTupleLength >= 8, or return early?
    ids[0] = CalculateNodeIndex3D(i, j, k);
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
  int CalculateCellIndex3D(int i, int j, int k)
  {
    return (k * cellDims[1] + j) * cellDims[0] + i;
  }
  int CalculateNodeIndex3D(int i, int j, int k)
  {
    return (k * nodeDims[1] + j) * nodeDims[0] + i;
  }
  void CalculateLogicalCellIndices3D(int index, int &i, int &j, int &k)
  {
    int cellDims01 = cellDims[0] * cellDims[1];
    k = index / cellDims01;
    int indexij = index % cellDims01;
    j = indexij / cellDims[0];
    i = indexij % cellDims[0];
  }
  void CalculateLogicalNodeIndices3D(int index, int &i, int &j, int &k)
  {
    int nodeDims01 = nodeDims[0] * nodeDims[1];
    k = index / nodeDims01;
    int indexij = index % nodeDims01;
    j = indexij / nodeDims[0];
    i = indexij % nodeDims[0];
  }
};
    
class DataSet
{
public:
  DataSet() {}
    
  //EAVL-esque everything is a field data model
  //vtkm::Vec<vtkm::cont::ArrayHandle<FloatDefault, vtkm::cont::StorageTagBasic>, 1> Fields;
  std::vector<vtkm::cont::ArrayHandle<FloatDefault,
                                      vtkm::cont::StorageTagBasic> > Fields;
  vtkm::Id x_idx, y_idx, z_idx;

  ExplicitConnectivity conn;
  RegularConnectivity3D reg;

  //traditional data-model
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault,3> > Points;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault,1> > Field;
};

}
} // namespace vtkm::cont


#endif //vtk_m_cont_DataSet_h
