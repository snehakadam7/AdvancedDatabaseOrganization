CC=gcc

test_assign4: test_assign4_1.c btree_mgr.c record_mgr.c expr.c rm_serializer.c storage_mgr.c dberror.c buffer_mgr.c buffer_mgr_stat.c

	gcc -o test_assign4 test_assign4_1.c btree_mgr.c record_mgr.c expr.c rm_serializer.c storage_mgr.c dberror.c buffer_mgr.c buffer_mgr_stat.c

test_expr: test_expr.c btree_mgr.c record_mgr.c expr.c rm_serializer.c storage_mgr.c dberror.c buffer_mgr.c buffer_mgr_stat.c

	gcc -o test_expr test_expr.c btree_mgr.c record_mgr.c expr.c rm_serializer.c storage_mgr.c dberror.c buffer_mgr.c buffer_mgr_stat.c

clean:
	rm -rf *.o test_assign4 test_expr

all: test_assign4 test_expr
