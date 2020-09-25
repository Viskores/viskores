//=======================================================================================
//
//	Parallel Peak Pruning v. 2.0
//
//	Started June 15, 2017
//
// Copyright Hamish Carr, University of Leeds
//
// main_tree_compiler.cpp - main routine for the external verification programme
//
//=======================================================================================
//
// COMMENTS:
//
//	Just a harness for the TreeCompiler routines
//
//=======================================================================================

#include <stdio.h>
#include <vtkm/worklet/contourtree_distributed/TreeCompiler.h>

// main routine
int main(int argc, char** argv)
{ // main()
  // the compiler for putting them together
  vtkm::worklet::contourtree_distributed::TreeCompiler compiler;

  // we just loop through the arguments, reading them in and adding them
  for (int argument = 1; argument < argc; argument++)
  { // per argument
    // create a temporary file
    FILE* inFile = fopen(argv[argument], "r");

    // if it's bad, choke
    if (inFile == NULL)
    { // bad filename
      printf("Bad filename %s\n", argv[argument]);
      return EXIT_FAILURE;
    } // bad filename

    // read and append
    compiler.ReadBinary(inFile);

  } // per argument

  // now compile and print
  compiler.ComputeSuperarcs();
  compiler.PrintSuperarcs(true);
  return EXIT_SUCCESS;
} // main()
