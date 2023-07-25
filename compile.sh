rm *exe
g++ -g -c -fpermissive disk_io/disk_io.cpp -o disk_io/disk_io.o
g++ -g -c -fpermissive disk_io/pager.cpp -o disk_io/pager.o
g++ -g -c -fpermissive gluethread/glthread.c -o gluethread/glthread.o
g++ -g -c -fpermissive mem_allocator/mem_allocator.cpp -o mem_allocator/mem_allocator.o
g++ -g -c -fpermissive mem_allocator/mem.cpp -o mem_allocator/mem.o
g++ -g -c -fpermissive low_level_test.cpp -o low_level_test.o
g++ -g -c -fpermissive c-hashtable/hashtable.c -o c-hashtable/hashtable.o
g++ -g -c -fpermissive c-hashtable/hashtable_itr.c -o c-hashtable/hashtable_itr.o
g++ -g -c -fpermissive list_test.cpp -o list_test.o
g++ -g -c -fpermissive BTreeLib/bptree.cpp -o  BTreeLib/bptree.o 
g++ -g -c -fpermissive BTreeLib/bptree_mem.cpp -o  BTreeLib/bptree_mem.o 
g++ -g low_level_test.o disk_io/disk_io.o disk_io/pager.o mem_allocator/mem_allocator.o mem_allocator/mem.o gluethread/glthread.o c-hashtable/hashtable.o c-hashtable/hashtable_itr.o -o low_level_test.exe
g++ -g list_test.o disk_io/disk_io.o disk_io/pager.o mem_allocator/mem_allocator.o mem_allocator/mem.o gluethread/glthread.o c-hashtable/hashtable.o c-hashtable/hashtable_itr.o -o list_test.exe
g++ -g disk_io/disk_io.o disk_io/pager.o mem_allocator/mem_allocator.o mem_allocator/mem.o gluethread/glthread.o c-hashtable/hashtable.o c-hashtable/hashtable_itr.o BTreeLib/bptree.o  BTreeLib/bptree_mem.o -o BTreeLib/bptree.exe