# worse::core -- the foundational subsystem. container / algorithm / math / coroutine are
# module partitions WITHIN this one library (they are mutually dependent -- e.g. container
# uses algorithm.sort while algorithm uses container.iterator -- so they are not separable
# into independent targets). Module interface units are listed explicitly: they define the
# public module graph and BMI build order, so additions should be intentional and reviewable.
# Implementation .cpp / textual .hpp are globbed per-directory (lower risk, less churn).

set(CORE_MODULES
    core.cppm
    basic_type.cppm
    intrinsics.cppm
    memory.cppm
    platform.cppm
    type_traits.cppm
    utility.cppm

    container/allocator.cppm
    container/allocator_traits.cppm
    container/array.cppm
    container/container.cppm
    container/fixed_array.cppm
    container/fixed_list.cppm
    container/fixed_slist.cppm
    container/flat_map.cppm
    container/flat_set.cppm
    container/forward_list.cppm
    container/hash.cppm
    container/hash_table.cppm
    container/intrusive_list.cppm
    container/iterator.cppm
    container/list.cppm
    container/map.cppm
    container/memory_util.cppm
    container/priority_queue.cppm
    container/rb_tree.cppm
    container/set.cppm
    container/static_array.cppm
    container/swiss_table.cppm
    container/unordered_map.cppm
    container/unordered_set.cppm

    algorithm/algorithm.cppm
    algorithm/binary_search.cppm
    algorithm/heap.cppm
    algorithm/modifying.cppm
    algorithm/nonmodifying.cppm
    algorithm/sort.cppm

    math/math.cppm
    math/matrix.cppm
    math/simd.cppm
    math/vector.cppm

    coroutine/coroutine.cppm
    coroutine/task.cppm
)
list(TRANSFORM CORE_MODULES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)

file(GLOB_RECURSE CORE_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

worse_add_library(core
    MODULES ${CORE_MODULES}
    HEADERS ${CORE_HEADERS}
    SOURCES ${CORE_SOURCES}
    # No PUBLIC_DEPS yet -- core is the base. Future: worse_add_library(rhi PUBLIC_DEPS worse::core ...)
)
