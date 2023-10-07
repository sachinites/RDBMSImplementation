rm -f exe
g++ -g -c -fpermissive SqlParserMain.c -o SqlParserMain.o
g++ -g -c -fpermissive SqlSelectParserCFG.c -o SqlSelectParserCFG.o
g++ -g -c -fpermissive SqlCreateParserCFG.c -o SqlCreateParserCFG.o
g++ -g -c -fpermissive SqlInsertIntoParserCFG.c -o SqlInsertIntoParserCFG.o
flex Parser.l
g++ -g -c -fpermissive lex.yy.c -o lex.yy.o
g++ -g -c -fpermissive ../core/qep.c -o ../core/qep.o 
g++ -g -c -fpermissive ../core/sql_create.c -o ../core/sql_create.o 
g++ -g -c -fpermissive ../core/sql_insert_into.c -o ../core/sql_insert_into.o 
g++ -g -c -fpermissive ../core/sql_utils.c -o ../core/sql_utils.o 
g++ -g -c -fpermissive ../core/sql_io.c -o ../core/sql_io.o 
g++ -g -c -fpermissive ../BPlusTreeLib/BPlusTree.c -o ../BPlusTreeLib/BPlusTree.o
g++ -g -c -fpermissive ../core/Catalog.c -o ../core/Catalog.o
g++ -g -c -fpermissive ../gluethread/glthread.c -o ../gluethread/glthread.o
g++ -g -c ../c-hashtable/hashtable.c -o ../c-hashtable/hashtable.o
g++ -g -c   ../c-hashtable/hashtable_itr.c -o ../c-hashtable/hashtable_itr.o
g++ -g -c -fpermissive ../core/SqlMexprIntf.cpp -o ../core/SqlMexprIntf.o
g++ -g ../core/qep.o lex.yy.o SqlSelectParserCFG.o SqlCreateParserCFG.o SqlParserMain.o ../BPlusTreeLib/BPlusTree.o ../../MathExpressionParser/ExpressionParser.o ../../MathExpressionParser/Operators.o ../../MathExpressionParser/Dtype.o ../../MathExpressionParser/MexprTree.o ../../MathExpressionParser/EnumConvertor.o ../core/sql_utils.o ../core/sql_create.o ../core/Catalog.o ../gluethread/glthread.o SqlInsertIntoParserCFG.o ../core/sql_insert_into.o ../c-hashtable/hashtable.o ../c-hashtable/hashtable_itr.o ../core/sql_io.o ../core/SqlMexprIntf.o -o exe -lfl -lm