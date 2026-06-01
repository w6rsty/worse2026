#pragma once

// Shared declarations for the container benchmark suites. Exactly one TU
// (`bench_main.cpp`) defines ANKERL_NANOBENCH_IMPLEMENT and owns `main()`; every other
// bench file contributes a `void bench<Suite>(ankerl::nanobench::Bench&)` entry point that
// `main` invokes against a single shared Bench. Add a new suite by declaring it here, adding
// its `.cpp`, and calling it from `bench_main.cpp`.

#include <nanobench.h>

void benchArrays(ankerl::nanobench::Bench& bench);
void benchNodeLists(ankerl::nanobench::Bench& bench);
