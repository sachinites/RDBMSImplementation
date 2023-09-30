gcc -g -c SqlParserMain.c -o SqlParserMain.o
gcc -g -c SqlSelectParserCFG.c -o SqlSelectParserCFG.o
gcc -g -c SqlCreateParserCFG.c -o SqlCreateParserCFG.o
gcc -g -c SqlInsertIntoParserCFG.c -o SqlInsertIntoParserCFG.o
lex Parser.l
gcc -g -c lex.yy.c -o lex.yy.o
gcc -g -c ../core/sql_mexpr_intf.c -o ../core/sql_mexpr_intf.o
gcc -g -c ../core/qep.c -o ../core/qep.o 
gcc -g -c ../core/sql_create.c -o ../core/sql_create.o 
gcc -g -c ../core/sql_insert_into.c -o ../core/sql_insert_into.o 
gcc -g -c ../core/sql_utils.c -o ../core/sql_utils.o 
gcc -g -c ../core/sql_io.c -o ../core/sql_io.o 
gcc -g -c ../BPlusTreeLib/BPlusTree.c -o ../BPlusTreeLib/BPlusTree.o
gcc -g -c ../core/Catalog.c -o ../core/Catalog.o
gcc -g -c ../gluethread/glthread.c -o ../gluethread/glthread.o
gcc -g ../core/qep.o ../core/sql_mexpr_intf.o lex.yy.o SqlSelectParserCFG.o SqlCreateParserCFG.o SqlParserMain.o ../BPlusTreeLib/BPlusTree.o ../../MathExpressionParser/ParserMexpr.o ../../MathExpressionParser/MExpr.o ../../MathExpressionParser/ExpressionParser.o ../core/sql_utils.o ../core/sql_create.o ../core/Catalog.o ../gluethread/glthread.o SqlInsertIntoParserCFG.o ../core/sql_insert_into.o  ../core/sql_io.o -o exe -lfl -lm
            

