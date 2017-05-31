#Mandelbrot Set in Pure C with OpenMP

Reimplementation branch for incorporating coloring techniques.

This one doesn't use intrinsics and is deliberately extremeely simplified to focus on the differences needed for coloring.

This supports smooth coloring with logarithmic index scaling and palette lookup, and has been verified as portable between Windows and POSIX systems. However, the version I compiled with GCC is much slower than that compiled with MSVC - possibly because of more optimizations that MSVC can enable without running into segfaults.

This is reasonably fast - 580ms for 3840x2160 overall M-set at 256 iterations with smooth coloring, and about 450ms without smooth coloring.
