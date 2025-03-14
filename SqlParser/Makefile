GCC=g++
CFLAGS=-g -Wall -std=c++11

TARGET_DBMS_EXE=dbms.exe
TARGET_LIBDBMS=libdbms.a
TARGET_SQL_ULIB=libsqlapi.a
SQL_UAPI_TEST_EXE=../uapi/sql_uapi_test.exe

TARGET=${TARGET_DBMS_EXE} ${TARGET_LIBDBMS} ${TARGET_SQL_ULIB} ${SQL_UAPI_TEST_EXE}

EXTERNAL_LIBS=-L ../../MathExpressionParser/ -lMexpr
DBMS_LIB=-L . -ldbms
SQL_ULIB=-L . -lsqlapi
STDLIBS=-lfl -lm

OBJS=../core/qep.o \
			../core/sql_name.o \
			lex.yy.o \
			SqlSelectParserCFG.o \
			SqlDeleteParserCFG.o \
			SqlUpdateParserCFG.o \
			SqlCreateParserCFG.o \
			SqlInsertIntoParserCFG.o \
			../BPlusTreeLib/BPlusTree.o \
			../core/BPlusTreeCompFn.o \
			../core/sql_utils.o \
			../core/sql_create.o \
			../core/sql_select.o \
			../core/sql_where.o \
			../core/Catalog.o \
			../gluethread/glthread.o \
			../core/sql_insert_into.o \
			../core/sql_delete.o \
			../core/sql_update.o  \
			../core/sql_join.o \
			../c-hashtable/hashtable.o \
			../c-hashtable/hashtable_itr.o \
			../core/sql_io.o \
			../core/SqlMexprIntf.o \
			../core/sql_group_by.o \
			../core/sql_order_by.o

all: ${TARGET}
	@echo DBMS lib Build Finished Successfully.

lex.yy.o:Parser.l 
	flex Parser.l
	${GCC} ${CFLAGS} -c lex.yy.c

${TARGET_LIBDBMS}:${OBJS} ../uapi/sql_api.o
	ar rcs ${TARGET_LIBDBMS} ${OBJS} ../uapi/sql_api.o
	@echo ${TARGET_LIBDBMS} is created successfully

${TARGET_DBMS_EXE}:SqlParserMain.o SqlToMexprEnumMapper.o ${TARGET_LIBDBMS}
	${GCC} ${CFLAGS} SqlParserMain.o SqlToMexprEnumMapper.o -o ${TARGET_DBMS_EXE} ${DBMS_LIB} ${EXTERNAL_LIBS} ${STDLIBS}
	@echo ${TARGET_DBMS_EXE} is created successfully

${TARGET_SQL_ULIB}:../uapi/sql_api.o
	ar rcs ${TARGET_SQL_ULIB} ../uapi/sql_api.o
	@echo ${TARGET_SQL_ULIB} is created successfully

${SQL_UAPI_TEST_EXE}:../uapi/sql_uapi_test.o SqlToMexprEnumMapper.o ${TARGET_SQL_ULIB} ${TARGET_LIBDBMS}
	${GCC} ${CFLAGS} ../uapi/sql_uapi_test.o SqlToMexprEnumMapper.o -o ${SQL_UAPI_TEST_EXE} ${SQL_ULIB} ${DBMS_LIB} ${EXTERNAL_LIBS} ${STDLIBS}
	@echo ${SQL_UAPI_TEST_EXE} is created successfully

clean:
	rm -f *.exe
	rm -f *.a
	rm -f *.o
	rm -f ../core/*.o
	rm -f ../BPlusTreeLib/*.o
	rm -f ../uapi/*.o
	rm -f ../uapi/*.exe

#Compilation Rules for Individial files
SqlParserMain.o:SqlParserMain.c
	${GCC} ${CFLAGS} -c SqlParserMain.c -o SqlParserMain.o

SqlSelectParserCFG.o:SqlSelectParserCFG.c
	${GCC} ${CFLAGS} -c SqlSelectParserCFG.c -o SqlSelectParserCFG.o

SqlCreateParserCFG.o:SqlCreateParserCFG.c
	${GCC} ${CFLAGS} -c SqlCreateParserCFG.c -o SqlCreateParserCFG.o

SqlInsertIntoParserCFG.o:SqlInsertIntoParserCFG.c
	${GCC} ${CFLAGS} -c SqlInsertIntoParserCFG.c -o SqlInsertIntoParserCFG.o

SqlDeleteParserCFG.o:SqlDeleteParserCFG.c
	${GCC} ${CFLAGS} -c SqlDeleteParserCFG.c -o SqlDeleteParserCFG.o

SqlUpdateParserCFG.o:SqlUpdateParserCFG.c
	${GCC} ${CFLAGS} -c SqlUpdateParserCFG.c -o SqlUpdateParserCFG.o

SqlToMexprEnumMapper.o:SqlToMexprEnumMapper.c
	${GCC} ${CFLAGS} -c SqlToMexprEnumMapper.c -o SqlToMexprEnumMapper.o

../core/qep.o:../core/qep.c
	${GCC} ${CFLAGS} -c ../core/qep.c	-o ../core/qep.o

../core/sql_create.o:../core/sql_create.c
	${GCC} ${CFLAGS} -c ../core/sql_create.c -o ../core/sql_create.o

../core/sql_update.o:../core/sql_update.c	
	${GCC} ${CFLAGS} -c ../core/sql_update.c -o ../core/sql_update.o

../core/sql_insert_into.o:../core/sql_insert_into.c
	${GCC} ${CFLAGS} -c ../core/sql_insert_into.c -o ../core/sql_insert_into.o

../core/sql_select.o:../core/sql_select.c
	${GCC} ${CFLAGS} -c ../core/sql_select.c -o ../core/sql_select.o

../core/sql_utils.o:../core/sql_utils.c
	${GCC} ${CFLAGS} -c ../core/sql_utils.c -o ../core/sql_utils.o

../core/sql_io.o:../core/sql_io.c
	${GCC} ${CFLAGS} -c ../core/sql_io.c  -o ../core/sql_io.o

../core/sql_group_by.o:../core/sql_group_by.c
	${GCC} ${CFLAGS} -c ../core/sql_group_by.c -o ../core/sql_group_by.o

../core/sql_order_by.o:../core/sql_order_by.c
	${GCC} ${CFLAGS} -c ../core/sql_order_by.c -o ../core/sql_order_by.o

../core/sql_where.o:../core/sql_where.c
	${GCC} ${CFLAGS} -c ../core/sql_where.c -o ../core/sql_where.o

../core/sql_join.o:../core/sql_join.c
	${GCC} ${CFLAGS} -c ../core/sql_join.c -o ../core/sql_join.o

../core/sql_name.o:../core/sql_name.c
	${GCC} ${CFLAGS} -c ../core/sql_name.c -o ../core/sql_name.o

../BPlusTreeLib/BPlusTree.o:../BPlusTreeLib/BPlusTree.c
	${GCC} ${CFLAGS} -c -fpermissive ../BPlusTreeLib/BPlusTree.c -o ../BPlusTreeLib/BPlusTree.o

../core/BPlusTreeCompFn.o:../core/BPlusTreeCompFn.c
	${GCC} ${CFLAGS} -c -fpermissive ../core/BPlusTreeCompFn.c -o ../core/BPlusTreeCompFn.o

../BPlusTreeLib/main.o:../BPlusTreeLib/main.c
	${GCC} ${CFLAGS} -c ../BPlusTreeLib/main.c -o ../BPlusTreeLib/main.o

../core/Catalog.o:../core/Catalog.c
	${GCC} ${CFLAGS} -c ../core/Catalog.c -o ../core/Catalog.o

../gluethread/glthread.o:../gluethread/glthread.c
	${GCC} ${CFLAGS} -c ../gluethread/glthread.c -o ../gluethread/glthread.o

../c-hashtable/hashtable.o:../c-hashtable/hashtable.c
	${GCC} ${CFLAGS} -c ../c-hashtable/hashtable.c -o ../c-hashtable/hashtable.o

../c-hashtable/hashtable_itr.o:../c-hashtable/hashtable_itr.c
	${GCC} ${CFLAGS} -c ../c-hashtable/hashtable_itr.c -o ../c-hashtable/hashtable_itr.o

../core/SqlMexprIntf.o:../core/SqlMexprIntf.cpp
	${GCC} ${CFLAGS} -c ../core/SqlMexprIntf.cpp	-o ../core/SqlMexprIntf.o

../uapi/sql_api.o:../uapi/sql_api.cpp
	${GCC} ${CFLAGS} -c ../uapi/sql_api.cpp -o ../uapi/sql_api.o 

../uapi/sql_uapi_test.o:../uapi/sql_uapi_test.cpp
	${GCC} ${CFLAGS} -c ../uapi/sql_uapi_test.cpp -o ../uapi/sql_uapi_test.o

../core/sql_delete.o:../core/sql_delete.c
	${GCC} ${CFLAGS} -c ../core/sql_delete.c -o ../core/sql_delete.o