# Initialize DIY in vtkm::cont::Initialize

This has the side effect of initialing MPI_Init (and will also
call MPI_Finalize at program exit). However, if the calling 
code has already called MPI_Init, then nothing will happen. 
Thus, if the calling code wants to manage MPI_Init/Finalize,
it can do so as long as it does before it initializes VTK-m.

