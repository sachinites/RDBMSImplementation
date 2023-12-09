rm -f dbms.exe
rm -f *.o
rm -f ../core/*.o
rm -f ../BPlusTreeLib/*.o
rm -f ../uapi/*.o
rm -f ../uapi/*.exe

g++ -g -c  SqlParserMain.c -o SqlParserMain.o
g++ -g -c  SqlSelectParserCFG.c -o SqlSelectParserCFG.o
g++ -g -c  SqlCreateParserCFG.c -o SqlCreateParserCFG.o
g++ -g -c  SqlInsertIntoParserCFG.c -o SqlInsertIntoParserCFG.o
g++ -g -c  SqlDeleteParserCFG.c -o SqlDeleteParserCFG.o
g++ -g -c SqlUpdateParserCFG.c -o SqlUpdateParserCFG.o
flex Parser.l
g++ -g -c  lex.yy.c -o lex.yy.o
g++ -g -c  ../core/qep.c -o ../core/qep.o 
g++ -g -c  ../core/sql_create.c -o ../core/sql_create.o 
g++ -g -c  ../core/sql_update.c -o ../core/sql_update.o 
g++ -g -c  ../core/sql_insert_into.c -o ../core/sql_insert_into.o 
g++ -g -c  ../core/sql_utils.c -o ../core/sql_utils.o 
g++ -g -c  ../core/sql_io.c -o ../core/sql_io.o 
g++ -g -c  ../core/sql_group_by.c -o ../core/sql_group_by.o
g++ -g -c  ../core/sql_order_by.c -o ../core/sql_order_by.o
g++ -g -c  -fpermissive ../BPlusTreeLib/BPlusTree.c -o ../BPlusTreeLib/BPlusTree.o
g++ -g -c  -fpermissive ../BPlusTreeLib/BPlusTreeCompFn.c -o ../BPlusTreeLib/BPlusTreeCompFn.o
g++ -g -c  -fpermissive ../BPlusTreeLib/main.c -o ../BPlusTreeLib/main.o
g++ -g -c  ../core/Catalog.c -o ../core/Catalog.o
g++ -g -c  ../gluethread/glthread.c -o ../gluethread/glthread.o
g++ -g -c ../c-hashtable/hashtable.c -o ../c-hashtable/hashtable.o
g++ -g -c   ../c-hashtable/hashtable_itr.c -o ../c-hashtable/hashtable_itr.o
g++ -g -c  ../core/SqlMexprIntf.cpp -o ../core/SqlMexprIntf.o
g++ -g -c ../uapi/sql_api.cpp -o ../uapi/sql_api.o
g++ -g -c ../uapi/sql_uapi_test.cpp -o ../uapi/sql_uapi_test.o
g++ -g -c ../core/sql_delete.c -o ../core/sql_delete.o
g++ -g ../uapi/sql_api.o ../core/qep.o lex.yy.o SqlSelectParserCFG.o SqlDeleteParserCFG.o SqlUpdateParserCFG.o SqlCreateParserCFG.o ../BPlusTreeLib/BPlusTree.o ../BPlusTreeLib/BPlusTreeCompFn.o ../core/sql_utils.o ../core/sql_create.o ../core/Catalog.o ../gluethread/glthread.o SqlInsertIntoParserCFG.o ../core/sql_insert_into.o ../core/sql_delete.o ../core/sql_update.o  ../c-hashtable/hashtable.o ../c-hashtable/hashtable_itr.o ../core/sql_io.o ../core/SqlMexprIntf.o ../core/sql_group_by.o ../core/sql_order_by.o SqlParserMain.o -o dbms.exe -lfl -lm -L ../../MathExpressionParser/ -lMexp

g++ -g ../BPlusTreeLib/BPlusTree.o ../BPlusTreeLib/BPlusTreeCompFn.o ../BPlusTreeLib/main.o -o ../BPlusTreeLib/BPlusTree.exe

g++ -g ../uapi/sql_api.o ../core/qep.o lex.yy.o SqlSelectParserCFG.o SqlDeleteParserCFG.o SqlUpdateParserCFG.o SqlCreateParserCFG.o ../BPlusTreeLib/BPlusTree.o ../BPlusTreeLib/BPlusTreeCompFn.o ../core/sql_utils.o ../core/sql_create.o ../core/Catalog.o ../gluethread/glthread.o SqlInsertIntoParserCFG.o ../core/sql_insert_into.o ../c-hashtable/hashtable.o ../c-hashtable/hashtable_itr.o ../core/sql_io.o ../core/SqlMexprIntf.o  ../core/sql_group_by.o ../core/sql_update.o  ../core/sql_delete.o ../core/sql_order_by.o ../uapi/sql_uapi_test.o -o ../uapi/sql_uapi_test.exe -lfl -lm -L ../../MathExpressionParser/ -lMexp
