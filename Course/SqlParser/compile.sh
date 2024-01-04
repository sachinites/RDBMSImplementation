rm -f *.o
rm -f ../core/*.o
rm -f ../BPlusTreeLib/*.o 
rm -f ../BPlusTreeLib/*.exe 
rm -f *.exe

#SqlParser dir
lex Parser.l
g++ -g -c lex.yy.c -o lex.yy.o
g++ -g -c  SqlParserMain.c -o SqlParserMain.o
g++ -g -c  SqlCreateParserCFG.c -o SqlCreateParserCFG.o
g++ -g -c  SqlInsertIntoParserCFG.c -o SqlInsertIntoParserCFG.o
g++ -g -c SqlSelectParserCFG.c -o SqlSelectParserCFG.o
g++ -g -c SqlToMexprEnumMapper.c -o SqlToMexprEnumMapper.o

#B+ Tree Library
g++ -g -c  -fpermissive ../BPlusTreeLib/BPlusTree.c -o ../BPlusTreeLib/BPlusTree.o
g++ -g -c  -fpermissive ../BPlusTreeLib/main.c -o ../BPlusTreeLib/main.o
g++ -g ../BPlusTreeLib/BPlusTree.o ../BPlusTreeLib/main.o -o ../BPlusTreeLib/main.exe

#core dir
g++ -g -c ../core/sql_create.c -o ../core/sql_create.o
g++ -g -c  -fpermissive ../core/BPlusTreeCompFn.c -o ../core/BPlusTreeCompFn.o
g++ -g -c ../core/Catalog.c -o ../core/Catalog.o
g++ -g -c ../core/sql_delete.c -o ../core/sql_delete.o
g++ -g -c ../core/sql_insert_into.c -o ../core/sql_insert_into.o
g++ -g -c ../core/SqlMexprIntf.cpp -o ../core/SqlMexprIntf.o
g++ -g -c ../core/qep.c -o ../core/qep.o
g++ -g -c ../core/sql_select.c -o ../core/sql_select.o
g++ -g -c ../core/sql_join.c -o ../core/sql_join.o
g++ -g -c ../core/sql_io.c -o ../core/sql_io.o
g++ -g -c ../core/sql_utils.c -o ../core/sql_utils.o
g++ -g -c ../core/sql_name.c -o ../core/sql_name.o

#Create an Executable
g++ -g lex.yy.o \
            SqlParserMain.o \
            SqlCreateParserCFG.o \
            SqlInsertIntoParserCFG.o \
            SqlSelectParserCFG.o \
            ../core/sql_create.o \
            ../BPlusTreeLib/BPlusTree.o \
            ../core/BPlusTreeCompFn.o \
            ../core/Catalog.o \
            ../core/sql_delete.o \
            ../core/sql_insert_into.o \
            SqlToMexprEnumMapper.o \
            ../core/SqlMexprIntf.o \
            ../core/qep.o \
            ../core/sql_select.o \
            ../core/sql_join.o \
            ../core/sql_io.o \
            ../core/sql_utils.o \
            ../core/sql_name.o \
            -o dbms.exe -lfl -L ../../../MathExpressionParser/Course/ -lMexpr
