project(gc)
cmake_minimum_required(VERSION 3.18)
add_library(${PROJECT_NAME} STATIC
    alloc.c
    reclaim.c
    allchblk.c
    misc.c
    mach_dep.c
    os_dep.c
    mark_rts.c
    headers.c
    mark.c
    obj_map.c
    blacklst.c
    finalize.c
    new_hblk.c
    dbg_mlc.c
    malloc.c
    dyn_load.c
    typd_mlc.c
    ptr_chck.c
    mallocx.c
    gc_dlopen.c
    pthread_start.c
    pthread_support.c
    pthread_stop_world.c
    specific.c
    thread_local_alloc.c
    gcj_mlc.c
    fnlz_mlc.c
    backgraph.c
    cordbscs.c
    cordprnt.c
    cordxtra.c
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/../include
    ${PROJECT_SOURCE_DIR}/../include/gc
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -DENABLE_DISCLAIM
    -DGC_ATOMIC_UNCOLLECTABLE
    -DGC_BUILTIN_ATOMIC
    -DGC_ENABLE_SUSPEND_THREAD
    -DGC_GCJ_SUPPORT
    -DGC_NOT_DLL
    -DGC_REQUIRE_WCSDUP
    -DHANDLE_FORK
    -DHAVE_DLADDR
    -DHAVE_DL_ITERATE_PHDR
    -DHAVE_PTHREAD_SETNAME_NP_WITH_TID
    -DJAVA_FINALIZATION
    -DPARALLEL_MARK
    -DTHREAD_LOCAL_ALLOC
    -DNDEBUG
    -DALL_INTERIOR_POINTERS
    -DNO_EXECUTE_PERMISSION
    -DGC_THREADS
    -D_REENTRANT
    -DUSE_MMAP
    -DUSE_MUNMAP
    -fno-strict-aliasing
    $<$<CONFIG:DEBUG>:-g3>
    $<$<CONFIG:DEBUG>:-Og>
    $<$<CONFIG:DEBUG>:-DMEMORY_DEBUG>
    $<$<CONFIG:DEBUG>:-DENA_TRACE>
    $<$<CONFIG:RELEASE>:-Ofast>
    $<$<CONFIG:PROFILE>:-pg -O0>
)
