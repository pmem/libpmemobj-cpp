Requirements:
* Constructors should check whether the container is being constructed on pmem and within a transaction.
  If not, appropriate exception should be thrown.
* If any operation has to or is not allowed to be called inside of a transaction there should be a proper check for that.
* Operations which modify the entire container (like operator=, clear(), etc.) should be transactional.
* If any operation inside destructor can fail there should be a separate method like free_data/destroy.
* Each container should have a mechanism for checking layout compatibility. For containers with big memory overhead
  (like concurrent_hash_map) size of key and value should be stored on pmem and compared with sizeof(value_type) after pool reopen.
  Containers with lower memory overhead (like vector) can implement some heuristic (like comparing pmemobj_alloc_usable_size
  with expected capacity).
* Padding should always be explicit
* If any object from standard library is being stored on pmem it's size should be verified by static_assert
* for_each_ptr method should be implemented to allow defragmentation
