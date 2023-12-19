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

#B+ Tree Library
g++ -g -c  -fpermissive ../BPlusTreeLib/BPlusTree.c -o ../BPlusTreeLib/BPlusTree.o
g++ -g -c  -fpermissive ../BPlusTreeLib/main.c -o ../BPlusTreeLib/main.o
g++ -g ../BPlusTreeLib/BPlusTree.o ../BPlusTreeLib/main.o -o ../BPlusTreeLib/main.exe

#core dir
g++ -g -c ../core/sql_create.c -o ../core/sql_create.o
g++ -g -c  -fpermissive ../core/BPlusTreeCompFn.c -o ../core/BPlusTreeCompFn.o
g++ -g -c ../core/Catalog.c -o ../core/Catalog.o
g++ -g -c ../core/sql_delete.c -o ../core/sql_delete.o

#Create an Executable
g++ -g lex.yy.o \
            SqlParserMain.o \
            SqlCreateParserCFG.o \
            SqlInsertIntoParserCFG.o \
            ../core/sql_create.o \
            ../BPlusTreeLib/BPlusTree.o \
            ../core/BPlusTreeCompFn.o \
            ../core/Catalog.o \
             ../core/sql_delete.o \
            -o dbms.exe -lfl
