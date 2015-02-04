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
  ExplicitConnectivity() : Shapes(), Connectivity() {}

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
  void GetIndices(vtkm::Id index, vtkm::Vec<vtkm::Id,8> &ids)
  {
    int n = GetNumberOfIndices(index);
    int start = MapCellToConnectivityIndex.GetPortalControl().Get(index);
    for (int i=0; i<n; i++)
      ids[i] = Connectivity.GetPortalControl().Get(start+i);
  }

  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> Shapes;
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> NumIndices;
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> Connectivity;
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> MapCellToConnectivityIndex;
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

  //traditional data-model
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault,3> > Points;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault,1> > Field;
};

}
} // namespace vtkm::cont


#endif //vtk_m_cont_DataSet_h
