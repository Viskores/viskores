#ifndef vtk_m_cont_DataSet_h
#define vtk_m_cont_DataSet_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DynamicArrayHandle.h>

namespace vtkm {
namespace cont {

class ExplicitConnectivity
{
public:
    ExplicitConnectivity() : Shapes(), Connectivity() {}

    vtkm::Id GetNumberOfCells() { return Shapes.GetNumberOfValues(); }

    vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> Shapes;
    vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> Connectivity;
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
