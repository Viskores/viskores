//mark that we are including headers as test for completeness.
//This is used by headers that include thrust to properly define a proper
//device backend / system
#define VTKM_TEST_HEADER_BUILD

#define BOOST_SP_DISABLE_THREADS

#include <vtkm/cont/DataSet.h>

int Test_Build_For_DataSet()
{
    std::cout<<"Running DataSet test"<<std::endl;
    vtkm::cont::DataModel m;
    int nVerts = 3;
    m.Points.PrepareForOutput(nVerts, VTKM_DEFAULT_DEVICE_ADAPTER_TAG()); //vtkm::cont::DeviceAdapterTagSerial());
    m.Field.PrepareForOutput(nVerts, vtkm::cont::DeviceAdapterTagSerial());

    vtkm::Vec<vtkm::FloatDefault,3> V0 = vtkm::Vec<vtkm::FloatDefault,3>(0, 0, 0);
    vtkm::Vec<vtkm::FloatDefault,3> V1 = vtkm::Vec<vtkm::FloatDefault,3>(1, 0, 0);
    vtkm::Vec<vtkm::FloatDefault,3> V2 = vtkm::Vec<vtkm::FloatDefault,3>(1, 1, 0);
    
    m.Points.GetPortalControl().Set(0, V0);
    m.Points.GetPortalControl().Set(1, V1);
    m.Points.GetPortalControl().Set(2, V2);

    m.Field.GetPortalControl().Set(0, vtkm::Vec<vtkm::FloatDefault,1>(10));
    m.Field.GetPortalControl().Set(1, vtkm::Vec<vtkm::FloatDefault,1>(20));
    m.Field.GetPortalControl().Set(2, vtkm::Vec<vtkm::FloatDefault,1>(30));
    
    return 0;
}

int
TestBuild_vtkm_cont_DataSet(int, char*[])
{
    return Test_Build_For_DataSet();
}
