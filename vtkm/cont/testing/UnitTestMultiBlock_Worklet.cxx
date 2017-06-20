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
#include <vtkm/worklet/FieldStatistics.h>
#include <vtkm/worklet/WorkletMapMultiBlock.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapMultiBlock.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/AverageByKey.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/ScatterUniform.h>

#include <vtkm/filter/CellAverage.h>
#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/Histogram.h>
/*namespace vtkm {
namespace filter {

class DivideField : public vtkm::filter::FilterField<DivideField>
{
public:
  VTKM_CONT
  DivideField()
  {}

  VTKM_CONT
  void SetDividerValue(vtkm::Id value){ this->DividerValue = value; }

  template<typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT
  vtkm::filter::ResultField DoExecute(const vtkm::cont::DataSet& input,
                                        const vtkm::cont::ArrayHandle<T, StorageType>& fieldata,
                                        const vtkm::filter::FieldMetadata& fieldMeta,
                                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                        const DeviceAdapter& tag)
  { 
    vtkm::cont::Field output;
    output.SetData(fieldata);
    typedef vtkm::cont::ArrayHandleConstant<vtkm::Id> ConstIdArray;
    ConstIdArray constArray(this->DividerValue, fieldata.GetNumberOfValues());
    vtkm::worklet::DispatcherMapField<vtkm::worklet::DivideWorklet> dispatcher;
    vtkm::worklet::DispatcherMapField<vtkm::worklet::FieldStatistics<vtkm::Float64, VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::SubtractConst> dispatcher2(vtkm::worklet::FieldStatistics<vtkm::Float64, VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::SubtractConst(0.5));
    //dispatcher.Invoke(fieldata,constArray,output); 
    dispatcher2.Invoke(fieldata,output);
    return vtkm::filter::ResultField(input,output.GetData(),std::string("pointvar"),vtkm::cont::Field::ASSOC_POINTS);
  }
private:
  vtkm::Id DividerValue;
};

template<>
class FilterTraits<DivideField>
{ //currently the Clip filter only works on scalar data.
public:
  typedef TypeListTagScalarAll InputFieldTypeList;
};


}
}*/

const std::vector<vtkm::filter::ResultField> MultiBlock_WorkletTest();

void TestMultiBlock_Worklet()
{
  std::cout << std::endl;
  std::cout << "--TestDataSet Uniform and Rectilinear--" << std::endl << std::endl;
  std::vector<vtkm::filter::ResultField> results=MultiBlock_WorkletTest();
  for(std::size_t j=0; j<results.size(); j++)
  { std::cout<<"dataset "<<j<<" \n";
    results[j].GetField().PrintSummary(std::cout);
    for(std::size_t i=0; i<results[j].GetField().GetData().GetNumberOfValues(); i++)
    { 
      //results[j].GetField().GetData().CopyTo(array);
      //VTKM_TEST_ASSERT(array.GetPortalConstControl().Get(i) == vtkm::Float64(j/2.0), "result incorrect");
    }

  }
}

namespace vtkm {
namespace worklet {

class Threshold : public vtkm::worklet::WorkletMapMultiBlock
{
public:
  typedef void ControlSignature(WholeArrayIn<> inputdata, MultiBlockOut<> filterresult, WholeArrayIn<> fieldname);
  typedef void ExecutionSignature (_1 , _2, OutputIndex, _3);
  typedef _1 InputDomain;
  
  //using ScatterType = vtkm::worklet::ScatterUniform ;
  
 // VTKM_CONT
  //ScatterType GetScatter () const { return vtkm::worklet::ScatterUniform(2); }

  template <typename T,typename H, typename Index, typename FieldName>
  VTKM_EXEC
  void operator()( const T Input,  H Out, Index index, FieldName fieldname) const
  {  
    
    if(Input.GetNumberOfValues())
    {  
      vtkm::Id FiledLength= Input.Get(0).GetField(fieldname.Get(0)).GetData().GetNumberOfValues();

      vtkm::cont::ArrayHandle<vtkm::Float64> concreteHandle; 
      Input.Get(index/FiledLength).GetField(fieldname.Get(0)).GetData().CopyTo(concreteHandle);
      
      vtkm::Id FiledValue=concreteHandle.GetPortalConstControl().Get(index%FiledLength);
      std::cout<<index<<" "<<index/FiledLength<<" "<<index%FiledLength<<"value"<<FiledValue<<"\n";
      //Out=1 ; 
    }
    
    return ;
  }
};

}
}


template <typename T>
vtkm::cont::MultiBlock UniformMultiBlockBuilder()
{
  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetFieldAdd dsf;
  vtkm::Vec<T,3> origin(0);
  vtkm::Vec<T,3> spacing(1);
  vtkm::cont::MultiBlock Blocks;
  for (vtkm::Id trial = 0; trial < 7; trial++)
  {
    vtkm::Id3 dimensions(10, 10, 10);
    vtkm::Id numPoints = dimensions[0] * dimensions[1];
    vtkm::Id numCells = (dimensions[0]-1) * (dimensions[1]-1);
    std::vector<T> varP2D(static_cast<std::size_t>(numPoints));
    for (std::size_t i = 0; i < static_cast<std::size_t>(numPoints); i++)
    {
      //varP2D[i] = static_cast<T>((trial-1)*i);
      varP2D[i] = static_cast<T>(trial);
    }
    std::vector<T> varC2D(static_cast<std::size_t>(numCells));
    for (std::size_t i = 0; i < static_cast<std::size_t>(numCells); i++)
    {
      varC2D[i] = static_cast<T>(trial*i);
    }
    dataSet = dataSetBuilder.Create(vtkm::Id2(dimensions[0], dimensions[1]),
                                    vtkm::Vec<T,2>(origin[0], origin[1]),
                                    vtkm::Vec<T,2>(spacing[0], spacing[1]));
    dsf.AddPointField(dataSet, "pointvar", varP2D);
    dsf.AddCellField(dataSet, "cellvar", varC2D);
    Blocks.AddBlock(dataSet);
  }
  return Blocks;
}

template<typename FilterType, typename SpecsType>
std::vector<vtkm::filter::ResultField> Apply(vtkm::cont::MultiBlock MB, FilterType filter, SpecsType specs)
{
  std::vector<vtkm::filter::ResultField> results;
  for(std::size_t j=0; j<MB.GetNumberOfBlocks(); j++)
  {
    vtkm::filter::ResultField result = filter.Execute(MB.GetBlock(j), std::string(specs));
    results.push_back(result);
  }

  return results;
}

template<typename T>
vtkm::cont::DynamicArrayHandle CreateDynamicArray()
{
  // Declared static to prevent going out of scope.
  static T buffer[700];
  for (vtkm::Id index = 0; index < 700; index++)
  {
    buffer[index] = (vtkm::Id)1;
  }

  return vtkm::cont::DynamicArrayHandle(
        vtkm::cont::make_ArrayHandle(buffer, 700));
}

const std::vector<vtkm::filter::ResultField> MultiBlock_WorkletTest()
{
 
  vtkm::cont::DynamicArrayHandle array = CreateDynamicArray<vtkm::Id>();
  
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::MultiBlock Blocks=UniformMultiBlockBuilder<vtkm::Float64>();
  vtkm::worklet::DispatcherMapMultiBlock<vtkm::worklet::Threshold> dispatcher;
  std::string fieldname[1]={"pointvar"};
   
  dispatcher.Invoke(make_ArrayHandle(Blocks.GetBlocks()),array,vtkm::cont::make_ArrayHandle(fieldname, 1));

  std::vector<vtkm::filter::ResultField> results;
  //printSummary_ArrayHandle(make_ArrayHandle(Blocks.GetBlocks()),std::cout);
  vtkm::filter::CellAverage cellAverage;
  //results = Apply(Blocks,divider,"pointvar");
  results = cellAverage.Execute(Blocks, std::string("pointvar"));
  /*for(std::size_t j=100; j<Blocks.GetNumberOfBlocks(); j++)
  {   
    divider.SetDividerValue(2);
    vtkm::filter::ResultField result = divider.Execute(Blocks.GetBlock(j), std::string("pointvar"));
    results.push_back(result); 
  }*/

  return results;
}



int UnitTestMultiBlock_Worklet(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestMultiBlock_Worklet);
}
