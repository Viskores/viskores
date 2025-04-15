//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/Initialize.h>
#include <viskores/source/Wavelet.h>

int main(int argc, char** argv)
{
  viskores::cont::Initialize(argc, argv, viskores::cont::InitializeOptions::Strict);
  viskores::source::Wavelet source;

  auto output = source.Execute();
  output.PrintSummary(std::cout);

  return EXIT_SUCCESS;
}
