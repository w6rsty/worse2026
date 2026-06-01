export module worse.core.algorithm.algorithm;

// Umbrella: re-export every algorithm submodule so callers can pull the whole library
// in with a single `import worse.core.algorithm.algorithm;`. Each `export import` makes
// the imported module's exported names visible to importers of this one. Granular
// imports of the individual submodules remain available for callers who want to keep
// their import surface (and build dependencies) minimal.
export import worse.core.algorithm.heap;
export import worse.core.algorithm.binary_search;
export import worse.core.algorithm.sort;
export import worse.core.algorithm.nonmodifying;
export import worse.core.algorithm.modifying;
