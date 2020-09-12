CC=gcc

test_assign3: test_assign3_1.c record_mgr.c expr.c rm_serializer.c storage_mgr.c dberror.c buffer_mgr.c buffer_mgr_stat.c

	gcc -o test_assign3 test_assign3_1.c record_mgr.c expr.c rm_serializer.c storage_mgr.c dberror.c buffer_mgr.c buffer_mgr_stat.c

test_expr: test_expr.c record_mgr.c expr.c rm_serializer.c storage_mgr.c dberror.c buffer_mgr.c buffer_mgr_stat.c

	gcc -o test_expr test_expr.c record_mgr.c expr.c rm_serializer.c storage_mgr.c dberror.c buffer_mgr.c buffer_mgr_stat.c

clean:
	rm -rf *.o test_assign3 test_expr

all: test_assign3 test_expr
