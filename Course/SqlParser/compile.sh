rm -f *.o

lex Parser.l
g++ -g -c lex.yy.c -o lex.yy.o
g++ -g -c  SqlParserMain.c -o SqlParserMain.o
g++ -g -c  SqlCreateParserCFG.c -o SqlCreateParserCFG.o
g++ -g lex.yy.o SqlParserMain.o SqlCreateParserCFG.o -o dbms.exe -lfl
