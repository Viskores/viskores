//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/CellShape.h>

#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/MultiBlock.h>
#include <vtkm/exec/ConnectivityStructured.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/AverageByKey.h>


static void MultiBlock_WorkletTest();

void TestMultiBlock_Worklet()
{
  std::cout << std::endl;
  std::cout << "--TestDataSet Uniform and Rectilinear--" << std::endl << std::endl;
  MultiBlock_WorkletTest();
}

/*namespace vtkm {
namespace worklet {

class Threshold : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<Scalar> InputField, FieldOut<Scalar> FilterResult);
  typedef void ExecutionSignature (_1 , _2);
  //typedef _1 InputDomain;

  template <typename T,typename H>
  VTKM_EXEC
  void operator()( const T Input,  H Out) const
  {  
    if(Input > 5)
    {   Out=1 ; }
    else
    {   Out=0 ;}
    return ;
  }
};

}
}*/


static void MultiBlock_WorkletTest()
{
  vtkm::cont::DynamicArrayHandle output;
  
  vtkm::worklet::DispatcherMapField<vtkm::worklet::DivideWorklet> dispatcher;
  
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  std::vector<vtkm::cont::DataSet> Vblocks;
  Vblocks.push_back(testDataSet.Make2DRectilinearDataSet0());
  Vblocks.push_back(testDataSet.Make3DRegularDataSet1());
  Vblocks.push_back(testDataSet.Make3DRegularDataSet0());
  Vblocks.push_back(testDataSet.Make3DExplicitDataSet4());

  vtkm::cont::MultiBlock T2Blocks(Vblocks);

  std::vector<vtkm::cont::DataSet> InBlocks = T2Blocks.GetBlocks();
  for(std::size_t j=0; j<InBlocks.size(); j++)
  { 
    output=InBlocks[j].GetCellField("cellvar").GetData();
    typedef vtkm::cont::ArrayHandleConstant<vtkm::Id> ConstIdArray;
    ConstIdArray constArray(3, InBlocks[j].GetCellField("cellvar").GetData().GetNumberOfValues());
    //divide each cell's field value by 3 and store the results in "output" 
    dispatcher.Invoke(InBlocks[j].GetCellField("cellvar").GetData(),constArray,output); 
  }

  return ;
}



int UnitTestMultiBlock_Worklet(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestMultiBlock_Worklet);
}
