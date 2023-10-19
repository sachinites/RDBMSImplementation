rm -f dbms.exe
g++ -g -c  SqlParserMain.c -o SqlParserMain.o
g++ -g -c  SqlSelectParserCFG.c -o SqlSelectParserCFG.o
g++ -g -c  SqlCreateParserCFG.c -o SqlCreateParserCFG.o
g++ -g -c  SqlInsertIntoParserCFG.c -o SqlInsertIntoParserCFG.o
flex Parser.l
g++ -g -c  lex.yy.c -o lex.yy.o
g++ -g -c  ../core/qep.c -o ../core/qep.o 
g++ -g -c  ../core/sql_create.c -o ../core/sql_create.o 
g++ -g -c  ../core/sql_insert_into.c -o ../core/sql_insert_into.o 
g++ -g -c  ../core/sql_utils.c -o ../core/sql_utils.o 
g++ -g -c  ../core/sql_io.c -o ../core/sql_io.o 
g++ -g -c  ../core/sql_group_by.c -o ../core/sql_group_by.o
g++ -g -c  -fpermissive ../BPlusTreeLib/BPlusTree.c -o ../BPlusTreeLib/BPlusTree.o
g++ -g -c  ../core/Catalog.c -o ../core/Catalog.o
g++ -g -c  ../gluethread/glthread.c -o ../gluethread/glthread.o
g++ -g -c ../c-hashtable/hashtable.c -o ../c-hashtable/hashtable.o
g++ -g -c   ../c-hashtable/hashtable_itr.c -o ../c-hashtable/hashtable_itr.o
g++ -g -c  ../core/SqlMexprIntf.cpp -o ../core/SqlMexprIntf.o

g++ -g ../core/qep.o lex.yy.o SqlSelectParserCFG.o SqlCreateParserCFG.o SqlParserMain.o ../BPlusTreeLib/BPlusTree.o ../core/sql_utils.o ../core/sql_create.o ../core/Catalog.o ../gluethread/glthread.o SqlInsertIntoParserCFG.o ../core/sql_insert_into.o ../c-hashtable/hashtable.o ../c-hashtable/hashtable_itr.o ../core/sql_io.o ../core/SqlMexprIntf.o ../core/sql_group_by.o -o dbms.exe -lfl -lm -L ../../MathExpressionParser/ -lMexp