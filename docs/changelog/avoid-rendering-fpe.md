# Avoid floating point exceptions in rendering code

There were some places in the rendering code where floating point
exceptions (FPE) could happen under certain circumstances. Often we do not
care about invalid floating point operation in rendering as they often
occur in degenerate cases that don't contribute anyway. However,
simulations that might include VTK-m might turn on FPE to check their own
operations. In such cases, we don't want errant rendering arithmetic
causing an exception and bringing down the whole code. Thus, we turn on FPE
in some of our test platforms and avoid such operations in general.
