rm -f *.o

lex Parser.l
g++ -g -c lex.yy.c -o lex.yy.o
g++ -g -c  SqlParserMain.c -o SqlParserMain.o
g++ -g -c  SqlCreateParserCFG.c -o SqlCreateParserCFG.o

#core dir
g++ -g -c ../core/sql_create.c -o ../core/sql_create.o


g++ -g lex.yy.o \
            SqlParserMain.o \
            SqlCreateParserCFG.o \
            ../core/sql_create.o \
            -o dbms.exe -lfl
