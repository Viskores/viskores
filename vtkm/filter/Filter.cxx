//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/TaskQueue.h>

#include <future>

namespace vtkm
{
namespace filter
{

namespace
{
void RunFilter(Filter* self, vtkm::filter::DataSetQueue& input, vtkm::filter::DataSetQueue& output)
{
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  bool prevVal = tracker.GetThreadFriendlyMemAlloc();
  tracker.SetThreadFriendlyMemAlloc(true);

  std::pair<vtkm::Id, vtkm::cont::DataSet> task;
  while (input.GetTask(task))
  {
    auto outDS = self->Execute(task.second);
    output.Push(std::make_pair(task.first, std::move(outDS)));
  }

  vtkm::cont::Algorithm::Synchronize();
  tracker.SetThreadFriendlyMemAlloc(prevVal);
}

} // anonymous namespace

Filter::Filter()
{
  this->SetActiveCoordinateSystem(0);
}

Filter::~Filter() = default;

bool Filter::CanThread() const
{
  return true;
}

//----------------------------------------------------------------------------
void Filter::SetFieldsToPass(const vtkm::filter::FieldSelection& fieldsToPass)
{
  this->FieldsToPass = fieldsToPass;
}

void Filter::SetFieldsToPass(vtkm::filter::FieldSelection&& fieldsToPass)
{
  this->FieldsToPass = std::move(fieldsToPass);
}

void Filter::SetFieldsToPass(const vtkm::filter::FieldSelection& fieldsToPass,
                             vtkm::filter::FieldSelection::Mode mode)
{
  this->FieldsToPass = fieldsToPass;
  this->FieldsToPass.SetMode(mode);
}

VTKM_CONT void Filter::SetFieldsToPass(std::initializer_list<std::string> fields,
                                       vtkm::filter::FieldSelection::Mode mode)
{
  this->SetFieldsToPass(vtkm::filter::FieldSelection{ fields, mode });
}

void Filter::SetFieldsToPass(
  std::initializer_list<std::pair<std::string, vtkm::cont::Field::Association>> fields,
  vtkm::filter::FieldSelection::Mode mode)
{
  this->SetFieldsToPass(vtkm::filter::FieldSelection{ fields, mode });
}

void Filter::SetFieldsToPass(const std::string& fieldname,
                             vtkm::cont::Field::Association association,
                             vtkm::filter::FieldSelection::Mode mode)
{
  this->SetFieldsToPass(vtkm::filter::FieldSelection{ fieldname, association, mode });
}


//----------------------------------------------------------------------------
vtkm::cont::PartitionedDataSet Filter::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  vtkm::cont::PartitionedDataSet output;

  if (this->GetRunMultiThreadedFilter())
  {
    vtkm::filter::DataSetQueue inputQueue(input);
    vtkm::filter::DataSetQueue outputQueue;

    vtkm::Id numThreads = this->DetermineNumberOfThreads(input);

    //Run 'numThreads' filters.
    std::vector<std::future<void>> futures(static_cast<std::size_t>(numThreads));
    for (std::size_t i = 0; i < static_cast<std::size_t>(numThreads); i++)
    {
      auto f = std::async(
        std::launch::async, RunFilter, this, std::ref(inputQueue), std::ref(outputQueue));
      futures[i] = std::move(f);
    }

    for (auto& f : futures)
      f.get();

    //Get results from the outputQueue.
    output = outputQueue.Get();
  }
  else
  {
    for (const auto& inBlock : input)
    {
      vtkm::cont::DataSet outBlock = this->Execute(inBlock);
      output.AppendPartition(outBlock);
    }
  }

  return this->CreateResult(input, output);
}

vtkm::cont::DataSet Filter::Execute(const vtkm::cont::DataSet& input)
{
  return this->DoExecute(input);
}

vtkm::cont::PartitionedDataSet Filter::Execute(const vtkm::cont::PartitionedDataSet& input)
{
  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf,
                 "Filter (%d partitions): '%s'",
                 (int)input.GetNumberOfPartitions(),
                 vtkm::cont::TypeToString<decltype(*this)>().c_str());

  return this->DoExecutePartitions(input);
}

vtkm::cont::DataSet Filter::CreateResult(const vtkm::cont::DataSet& inDataSet) const
{
  auto fieldMapper = [](vtkm::cont::DataSet& out, const vtkm::cont::Field& fieldToPass) {
    out.AddField(fieldToPass);
  };
  return this->CreateResult(inDataSet, inDataSet.GetCellSet(), fieldMapper);
}

vtkm::cont::PartitionedDataSet Filter::CreateResult(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::cont::PartitionedDataSet& resultPartitions) const
{
  auto fieldMapper = [](vtkm::cont::PartitionedDataSet& out, const vtkm::cont::Field& fieldToPass) {
    out.AddField(fieldToPass);
  };
  return this->CreateResult(input, resultPartitions, fieldMapper);
}

vtkm::cont::DataSet Filter::CreateResultField(const vtkm::cont::DataSet& inDataSet,
                                              const vtkm::cont::Field& resultField) const
{
  vtkm::cont::DataSet outDataSet = this->CreateResult(inDataSet);
  outDataSet.AddField(resultField);
  VTKM_ASSERT(!resultField.GetName().empty());
  VTKM_ASSERT(outDataSet.HasField(resultField.GetName(), resultField.GetAssociation()));
  return outDataSet;
}

vtkm::Id Filter::DetermineNumberOfThreads(const vtkm::cont::PartitionedDataSet& input)
{
  vtkm::Id numDS = input.GetNumberOfPartitions();

  vtkm::Id availThreads = 1;

  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();

  if (tracker.CanRunOn(vtkm::cont::DeviceAdapterTagCuda{}))
    availThreads = this->NumThreadsPerGPU;
  else if (tracker.CanRunOn(vtkm::cont::DeviceAdapterTagKokkos{}))
  {
    //Kokkos doesn't support threading on the CPU.
#ifdef VTKM_KOKKOS_CUDA
    availThreads = this->NumThreadsPerGPU;
#else
    availThreads = 1;
#endif
  }
  else if (tracker.CanRunOn(vtkm::cont::DeviceAdapterTagSerial{}))
    availThreads = 1;
  else
    availThreads = this->NumThreadsPerCPU;

  vtkm::Id numThreads = std::min<vtkm::Id>(numDS, availThreads);
  return numThreads;
}

void Filter::ResizeIfNeeded(size_t index_st)
{
  if (this->ActiveFieldNames.size() <= index_st)
  {
    auto oldSize = this->ActiveFieldNames.size();
    this->ActiveFieldNames.resize(index_st + 1);
    this->ActiveFieldAssociation.resize(index_st + 1);
    this->UseCoordinateSystemAsField.resize(index_st + 1);
    this->ActiveCoordinateSystemIndices.resize(index_st + 1);
    for (std::size_t i = oldSize; i <= index_st; ++i)
    {
      this->ActiveFieldAssociation[i] = cont::Field::Association::Any;
      this->UseCoordinateSystemAsField[i] = false;
      this->ActiveCoordinateSystemIndices[i] = 0;
    }
  }
}

} // namespace filter
} // namespace vtkm
