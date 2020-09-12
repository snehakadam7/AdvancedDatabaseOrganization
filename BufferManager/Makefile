CC=gcc

test_assign2_1: test_assign2_1.c storage_mgr.c dberror.c buffer_mgr.c buffer_mgr_stat.c

	gcc -o test_assign2_1 buffer_mgr.c buffer_mgr_stat.c test_assign2_1.c storage_mgr.c dberror.c

test_assign2_2: test_assign2_2.c storage_mgr.c dberror.c buffer_mgr.c buffer_mgr_stat.c

	gcc -o test_assign2_2 buffer_mgr.c buffer_mgr_stat.c test_assign2_2.c storage_mgr.c dberror.c

clean:
	rm -rf *.o test_assign2_1 test_assign2_2

all: test_assign2_1 test_assign2_2
