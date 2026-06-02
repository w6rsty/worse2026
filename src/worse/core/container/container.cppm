export module worse.core.container.container;

/**
 * \file
 * \brief Umbrella that re-exports every container submodule so callers can pull the whole
 *        container library in with a single `import worse.core.container.container;`.
 * \note Each `export import` makes the imported module's exported names visible to importers
 *       of this one. Granular imports of the individual submodules remain available for
 *       callers who want to keep their import surface (and build dependencies) minimal.
 *       (Mirrors `worse.core.algorithm.algorithm`.)
 */

// --- shared infrastructure ---
export import worse.core.container.allocator;
export import worse.core.container.allocator_traits;
export import worse.core.container.iterator;
export import worse.core.container.memory_util;

// --- contiguous ---
export import worse.core.container.array;
export import worse.core.container.static_array;
export import worse.core.container.fixed_array;

// --- adapters / flat associative / intrusive ---
export import worse.core.container.priority_queue;
export import worse.core.container.flat_set;
export import worse.core.container.flat_map;
export import worse.core.container.intrusive_list;

// --- hash (open-addressing Robin Hood + SwissTable engine) ---
export import worse.core.container.hash;
export import worse.core.container.hash_table;
export import worse.core.container.unordered_set;
export import worse.core.container.unordered_map;
export import worse.core.container.swiss_table;

// --- node lists (allocating + inline fixed-capacity) ---
export import worse.core.container.list;
export import worse.core.container.forward_list;
export import worse.core.container.fixed_list;
export import worse.core.container.fixed_slist;

// --- ordered associative (red-black tree) ---
export import worse.core.container.rb_tree;
export import worse.core.container.set;
export import worse.core.container.map;
