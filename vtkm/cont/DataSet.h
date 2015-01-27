#ifndef vtk_m_cont_DataModel_h
#define vtk_m_cont_DataModel_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DynamicArrayHandle.h>

namespace vtkm {
namespace cont {
    
class DataModel
{
public:
    DataModel()
    {
	//Initialize the Points to some hardcoded value.
	//Make some triangles.
    }

    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault,3> > Points;
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault,1> > Field;
};

}
} // namespace vtkm::cont


#endif //vtk_m_cont_DataModel_h
