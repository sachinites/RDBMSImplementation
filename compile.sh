rm -f *exe
lex Parsers/SQLSelectParser.l
mv lex.yy.c Parsers/
gcc -g -c Parsers/lex.yy.c -o  Parsers/lex.yy.o
gcc -g -c Parsers/Ast.c -o Parsers/Ast.o
gcc -g -c Parsers/MExpr.c -o Parsers/MExpr.o
gcc -g -c   disk_io/disk_io.cpp -o disk_io/disk_io.o
gcc -g -c   disk_io/pager.cpp -o disk_io/pager.o
gcc -g -c   gluethread/glthread.c -o gluethread/glthread.o
gcc -g -c   mem_allocator/mem_allocator.cpp -o mem_allocator/mem_allocator.o
gcc -g -c   mem_allocator/mem.cpp -o mem_allocator/mem.o
gcc -g -c   low_level_test.cpp -o low_level_test.o
gcc -g -c   c-hashtable/hashtable.c -o c-hashtable/hashtable.o
gcc -g -c   c-hashtable/hashtable_itr.c -o c-hashtable/hashtable_itr.o
gcc -g -c   list_test.cpp -o list_test.o
#gcc -g low_level_test.o disk_io/disk_io.o disk_io/pager.o mem_allocator/mem_allocator.o mem_allocator/mem.o gluethread/glthread.o c-hashtable/hashtable.o c-hashtable/hashtable_itr.o -o low_level_test.exe
#gcc -g list_test.o disk_io/disk_io.o disk_io/pager.o mem_allocator/mem_allocator.o mem_allocator/mem.o gluethread/glthread.o c-hashtable/hashtable.o c-hashtable/hashtable_itr.o -o list_test.exe
gcc -g -c   core/Catalog.c -o core/Catalog.o
gcc -g -c   core/sql_intf.c -o core/sql_intf.o
gcc -g -c   core/sql_utils.c -o core/sql_utils.o
gcc -g -c   core/sql_io.c -o core/sql_io.o
gcc -g -c   core/select.c -o core/select.o
gcc -g -c Tracer/tracer.c -o Tracer/tracer.o
gcc -g -c core/qplanner.c -o core/qplanner.o
gcc -g -c core/sql_where.c -o core/sql_where.o
gcc -g -c core/sql_delete.c -o core/sql_delete.o
gcc -g -c core/sql_groupby.c -o core/sql_groupby.o
gcc -g -c stack/stack.c -o stack/stack.o
gcc -g -c gluethread/glthread.c -o gluethread/glthread.o
gcc -g -c Tree/avl.c -o Tree/avl.o
gcc -g -c   BPlusTreeLib/BPlusTree.c -o BPlusTreeLib/BPlusTree.o
gcc -g Parsers/Ast.o stack/stack.o Parsers/lex.yy.o core/Catalog.o core/sql_intf.o core/sql_utils.o core/sql_io.o c-hashtable/hashtable.o c-hashtable/hashtable_itr.o BPlusTreeLib/BPlusTree.o core/select.o gluethread/glthread.o Tracer/tracer.o core/qplanner.o core/sql_where.o Parsers/MExpr.o core/sql_groupby.o core/sql_delete.o Tree/avl.o -o exe -lfl -lm -lpthread
