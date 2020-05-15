set width 0
set height 0
set verbose off
set confirm off
set breakpoint pending on
set pagination off

# This test does the following:
# 1. Run all code until gdb_sync1 invocation and pause all threads
# 2. Advance thread 10 (thread_id == 0) to internal_insert_ndode invocation
#    (threads 11 and 12 are waiting on spin loop)
# 3. Advance thread 11 (thread_id == 1) to after first set_next invocation (in internal_insert_node)
# 4. Advance thread 12 (thread_id == 2, reader thread) map->count invocation
#    New node should now be visible (thread 11 partially completed)

break gdb_sync1
run
rbreak concurrent_skip_list_impl.hpp:internal_insert_node
thread 10
c
set scheduler-locking on
thread 11
set variable loop_sync_1 = 0
c
break set_next
c
finish
finish
finish
n
n
del 3
thread 12
set variable loop_sync_2 = 0
break gdb_sync2 thread 12
c
finish
info threads
quit
