# Fix crash when CUDA device is disabled

There was an issue where if VTK-m was compiled with CUDA support but then
run on a computer where no CUDA device was available, an inappropriate
exception was thrown (instead of just disabling the device). The
initialization code should now properly check for the existance of a CUDA
device.
