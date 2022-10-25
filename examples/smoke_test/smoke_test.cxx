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
#include <vtkm/source/Wavelet.h>

int main(int argc, char** argv)
{
  vtkm::cont::Initialize(argc, argv, vtkm::cont::InitializeOptions::Strict);
  vtkm::source::Wavelet source;

  auto output = source.Execute();
  output.PrintSummary(std::cout);

  return EXIT_SUCCESS;
}
