//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
//  Copyright (c) 2016, Los Alamos National Security, LLC
//  All rights reserved.
//
//  Copyright 2016. Los Alamos National Security, LLC.
//  This software was produced under U.S. Government contract DE-AC52-06NA25396
//  for Los Alamos National Laboratory (LANL), which is operated by
//  Los Alamos National Security, LLC for the U.S. Department of Energy.
//  The U.S. Government has rights to use, reproduce, and distribute this
//  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC
//  MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE
//  USE OF THIS SOFTWARE.  If software is modified to produce derivative works,
//  such modified software should be clearly marked, so as not to confuse it
//  with the version available from LANL.
//
//  Additionally, redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Los Alamos National Security, LLC, Los Alamos
//     National Laboratory, LANL, the U.S. Government, nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
//  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS
//  NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
//  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

//  This code is based on the algorithm presented in the paper:
//  “Parallel Peak Pruning for Scalable SMP Contour Tree Computation.”
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/Initialize.h>

#include <vtkm/filter/ContourTreeUniform.h>

#include <fstream>
#include <vector>

// Compute and render an isosurface for a uniform grid example
int main(int argc, char* argv[])
{
  std::cout << "ContourTreeMesh2D Example" << std::endl;

  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);
  if (argc != 2)
  {
    std::cout << "Usage: "
              << "$ " << argv[0] << " [-d device] input_file" << std::endl;
    std::cout << "File is expected to be ASCII with xdim ydim integers " << std::endl;
    std::cout << "followed by vector data last dimension varying fastest" << std::endl;
    return 0;
  }

  // open input file
  std::ifstream inFile(argv[1]);
  if (inFile.bad())
    return 0;

  // read size of mesh
  vtkm::Id2 vdims;
  inFile >> vdims[0];
  inFile >> vdims[1];
  std::size_t nVertices = static_cast<std::size_t>(vdims[0] * vdims[1]);

  // read data
  std::vector<vtkm::Float32> values(nVertices);
  for (std::size_t vertex = 0; vertex < nVertices; vertex++)
  {
    inFile >> values[vertex];
  }
  inFile.close();

  // build the input dataset
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::cont::DataSet inDataSet = dsb.Create(vdims);

  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(inDataSet, "values", values);

  // Convert 2D mesh of values into contour tree, pairs of vertex ids
  vtkm::filter::ContourTreeMesh2D filter;
  filter.SetActiveField("values");
  // Output data set is pairs of saddle and peak vertex IDs
  vtkm::cont::DataSet output = filter.Execute(inDataSet);
  vtkm::cont::Field resultField = output.GetField("saddlePeak");
  ;
  vtkm::cont::ArrayHandle<vtkm::Pair<vtkm::Id, vtkm::Id>> saddlePeak;
  resultField.GetData().CopyTo(saddlePeak);

  return 0;
}
