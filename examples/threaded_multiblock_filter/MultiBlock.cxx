//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/cuda/internal/CudaAllocator.h>
#include <vtkm/filter/contour/Contour.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>
#include <vtkm/source/Tangle.h>

#include <cstdlib>
#include <iostream>

struct Options
{
public:
  enum RunModeType
  {
    SERIAL = 0,
    OPENMP = 1,
    GPU = 2
  };

  std::string DataPath = "";
  std::string DataFile = "";
  std::string Field = "";
  std::string MapField = "";
  int IsoLevels = 1;
  std::vector<double> IsoValues;
  std::string ThreadMode = "serial";
  bool SyncMemAlloc = true;
  int NumTasks = 0;
  bool Tangle = false;
  vtkm::Id NumTangle;
  vtkm::Id3 TangleDims;
  RunModeType RunMode = SERIAL;
  std::string OutputFile = "./";

  int ThreadModeToInt() const
  {
    if (this->ThreadMode == "serial")
      return 0;
    else if (this->ThreadMode == "openmp")
      return 1;
    else if (this->ThreadMode == "task" && this->SyncMemAlloc)
      return 2;
    else if (this->ThreadMode == "task" && !this->SyncMemAlloc)
      return 3;

    return -1;
  }

  bool ParseOptions(int argc, char** argv)
  {
    if (argc < 3)
    {
      this->PrintUsage();
      return false;
    }

    //Put args into a list of pairs.
    std::vector<std::pair<std::string, std::vector<std::string>>> args;
    for (int i = 1; i < argc; i++)
    {
      std::string tmp = argv[i];
      if (tmp.find("--") != std::string::npos)
      {
        std::pair<std::string, std::vector<std::string>> entry;
        entry.first = tmp;
        args.push_back(entry);
      }
      else
        args.back().second.push_back(tmp);
    }

    //Fill out member data from args.
    this->DataPath = ".";
    for (const auto& a : args)
    {
      if (a.first == "--output")
        this->OutputFile = a.second[0];
      else if (a.first == "--data")
      {
        if (a.second.empty())
          return false;

        auto tmp = a.second[0];
        auto pos = tmp.rfind("/");
        if (pos != std::string::npos)
        {
          this->DataPath = tmp.substr(0, pos);
          this->DataFile = tmp.substr(pos + 1, tmp.size() - pos - 1);
        }
      }
      else if (a.first == "--tangle")
      {
        this->Tangle = true;
        this->NumTangle = std::atoi(a.second[0].c_str());
        this->TangleDims[0] = std::atoi(a.second[1].c_str());
        this->TangleDims[1] = std::atoi(a.second[2].c_str());
        this->TangleDims[2] = std::atoi(a.second[3].c_str());
      }
      else if (a.first == "--field")
      {
        if (a.second.empty())
          return false;
        this->Field = a.second[0];
      }
      else if (a.first == "--mapfield")
      {
        if (a.second.empty())
          return false;
        this->MapField = a.second[0];
      }
      else if (a.first == "--threading")
      {
        if (a.second.empty())
          return false;
        if (a.second[0] == "serial")
          this->ThreadMode = "serial";
        else if (a.second[0] == "openmp")
          this->ThreadMode = "openmp";
        else if (a.second[0] == "task")
        {
          if (a.second.size() != 2)
            return false;
          this->NumTasks = std::stoi(a.second[1]);
          this->ThreadMode = "task";
        }
      }
      else if (a.first == "--sync_mem_alloc")
        this->SyncMemAlloc = true;
      else if (a.first == "--async_mem_alloc")
        this->SyncMemAlloc = false;
      else if (a.first == "--isolevels")
      {
        if (a.second.empty())
          return false;
        this->IsoLevels = std::stoi(a.second[0]);
      }
      else if (a.first == "--isovalues")
      {
        if (a.second.empty())
          return false;
        this->IsoValues.clear();
        for (const auto& aa : a.second)
          this->IsoValues.push_back(std::stod(aa));
      }
      else if (a.first == "--serial")
        this->RunMode = SERIAL;
      else if (a.first == "--openmp")
        this->RunMode = OPENMP;
      else if (a.first == "--gpu")
        this->RunMode = GPU;
    }

    if (this->MapField == "")
      this->MapField = this->Field;

    if ((!this->Tangle && (this->DataFile == "" || this->ThreadMode == "")) || this->Field == "")
    {
      std::cerr << "Error in options" << std::endl;
      return false;
    }

    return true;
  }

  void PrintUsage() const
  {
    std::cerr << "Usage: --data <dataFile> or --tangle n d0 d1 d2 --field <field> --mapfield "
                 "<mapfield> --thread <serial openmp task N> --sync_mem_alloc --async_mem_alloc"
              << std::endl;
  }
};


int main(int argc, char** argv)
{
  vtkm::cont::Initialize(argc, argv);

  Options opts;
  if (!opts.ParseOptions(argc, argv))
  {
    opts.PrintUsage();
    return 0;
  }

  vtkm::Id numCPUThreads = 1, numGPUThreads = 1;
  if (opts.ThreadMode == "task")
  {
    numCPUThreads = opts.NumTasks;
    numGPUThreads = opts.NumTasks;
  }
  if (opts.RunMode == Options::RunModeType::SERIAL)
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(vtkm::cont::DeviceAdapterTagSerial{});
  else if (opts.RunMode == Options::RunModeType::OPENMP)
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(vtkm::cont::DeviceAdapterTagOpenMP{});
  else if (opts.RunMode == Options::RunModeType::GPU)
  {
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(vtkm::cont::DeviceAdapterTagCuda{});
#ifdef VTKM_CUDA
    if (opts.ThreadMode == "task")
    {
      if (opts.SyncMemAlloc)
      {
        vtkm::cont::cuda::internal::CudaAllocator::ForceSyncMemoryAllocator();
        std::cout << "  Task: Sync memory alloc = ON" << std::endl;
      }
      else
      {
        vtkm::cont::cuda::internal::CudaAllocator::ForceAsyncMemoryAllocator();
        std::cout << "  Task: Sync memory alloc = OFF" << std::endl;
      }
    }
#endif
  }

  vtkm::cont::PartitionedDataSet dataSets;
  if (opts.Tangle)
  {
    for (vtkm::Id i = 0; i < opts.NumTangle; i++)
    {
      vtkm::source::Tangle tangle(opts.TangleDims);
      dataSets.AppendPartition(tangle.Execute());
    }
  }

  vtkm::filter::contour::Contour contour;
  contour.SetRunMultiThreadedFilter(opts.ThreadMode == "task");
  contour.SetThreadsPerCPU(numCPUThreads);
  contour.SetThreadsPerGPU(numGPUThreads);
  contour.SetGenerateNormals(true);
  contour.SetActiveField(opts.Field);

  if (!opts.IsoValues.empty())
  {
    for (std::size_t i = 0; i < opts.IsoValues.size(); i++)
      contour.SetIsoValue((vtkm::Id)i, opts.IsoValues[i]);
  }
  else
  {
    auto field = dataSets.GetPartition(0).GetField(opts.Field);
    vtkm::Range range;
    field.GetRange(&range);
    vtkm::FloatDefault dR =
      (range.Max - range.Min) / static_cast<vtkm::FloatDefault>(opts.IsoLevels + 1);
    vtkm::FloatDefault v = dR;
    for (int i = 0; i < opts.IsoLevels; i++)
    {
      contour.SetIsoValue((vtkm::Id)i, v);
      v += dR;
    }
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  auto result = contour.Execute(dataSets);
  auto t2 = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> dt =
    std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
  std::cout << "Timer= " << dt.count() << std::endl;

  std::ofstream out(opts.OutputFile, std::ios_base::app);
  out << opts.ThreadModeToInt() << ", " << opts.NumTasks << ", " << opts.NumTangle << ", "
      << opts.TangleDims[0] << ", " << opts.IsoLevels << ", " << dt.count() << std::endl;

  return 0;
}
