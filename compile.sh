rm exe
g++ -g -c -fpermissive disk_io/disk_io.cpp -o disk_io/disk_io.o
g++ -g -c -fpermissive disk_io/pager.cpp -o disk_io/pager.o
g++ -g -c -fpermissive gluethread/glthread.c -o gluethread/glthread.o
g++ -g -c -fpermissive mem_allocator/mem_allocator.cpp -o mem_allocator/mem_allocator.o
g++ -g -c -fpermissive mem_allocator/mem.cpp -o mem_allocator/mem.o
g++ -g -c -fpermissive test.cpp -o test.o
g++ -g test.o disk_io/disk_io.o disk_io/pager.o mem_allocator/mem_allocator.o gluethread/glthread.o -o exe
