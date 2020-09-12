Assignment 3 - Record manager 

***Group No. 2: Team members
1.	Sneha Shahaji Kadam (skadam5@hawk.iit.edu)
2.	Ayelet Bendor (abendor@hawk.iit.edu)
3.	Naga Sirisha Chalapati (nchalapati@hawk.iit.edu)

***Homework Assignment 3 has been completed and following files were uploaded under Assignment3 folder:
1.	Makefile
2.	record_mgr.c
3.	record_mgr.h
4.	expr.c
5.	expr.h
6.	table.h
7.	rm_serializer.c
8.	buffer_mgr.c
9.	buffer_mgr.h
10.	buffer_mgr_stat.c
11.	buffer_mgr_stat.h
12.	dberror.c
13.	dberror.h
14.	dt.h
15.	storage_mgr.c
16.	storage_mgr.h
17.	test_assign3_1.c
18.	test_expr.c
19.	test_helper.h

***Steps to compile and execute program is as follows:
1.	Open terminal and navigate to project directory.
example: cd CS525/Assignment3
2.	Type make to compile source code files.
By default "make" will compile test_assign3.c(test case file).
For compiling source code file test_expr.c, type "make test_expr".
3.	Execute program using executable file.
example:  ./test_assign3 or ./test_expr
4.	Type "make clean" to delete old compiled files.
5.	We can check memory leaks with valgrind using following command:
	valgrind --leak-check=full --track-origins=yes ./test_assign3

***Assumptions made per code design:
1.	Record scan and next method, we wrote the function under the assumption that the total record size stored on a file is less than a PAGE_SIZE, therefore we started the scan from the first page and we would not perform an additional validation nor updating the page number. This assumption was made after we look at the test cases for this assignment, if needed, we’ll make the required changes for assignet 4.
2.	testScanTwo test case passed on cygwin but not in fusion cluster.
Per testScanTwo test case, we ran into issues with the second scan. The issue occurred at line 591. Despite our debugging effort we could not detect the issue before the assignment’s submission.  



***Methods implemented in record_mgr.c are as follows:

// Table and Record Manager Functions //
1.	initRecordManager():
Initialize the record manager.

2.	shutdownRecordManager ():
Deallocates the memory for the data structure and writes all dirty pages to the disk.
3.	createTable ():
Creates a table and stores schema at the beginning of the table.

4.	openTable ():
Structure RM_TableData opens a table to store metadata such as Schema from the header page of the disk into memory.

5.	closeTable ():
Closes the table and contents of pages are copied to the Disk.

6.	deleteTable ():
Deletes table from memory.

7.	getNumTuples ():
Returns the number of tuples in the table.

// Record Functions //
8.	insertRecord ():
Inserts a record into the page file table and sets the ID for each record based on the page number and the slot. This insertion is done continuously one after the other. Since the page size is 4096 bytes and the recordsize being 12 bytes, every page holds 341 records.

9.	deleteRecord ():
Deletes record from the table and updates the record with the keyword  '*'

10.	updateRecord ():
Updates the record with the new value of the record in the table.

11.	getRecord ():
Gets the record associated with the RID.
// Scan Functions //
12.	startScan ():
Initializes the RM_ScanHandle data structure passed as an argument.

13.	next ():
Scans through all records and returns the record that matches the condition.

14.	closeScan ():
Closes the scan operation.

// Schema Functions //
15.	getRecordSize():
Returns the record size of the schema.

16.	createSchema():
Creates new schema from the parameters.

17.	freeSchema():
Removes a schema from memory and de-allocates all the memory space allocated to the schema.

// Attribute Functions //

18.	createRecord() :
Allocates memory which is equal to the size of the record for the record pointer. 

19.	freeRecord ():
Free’s the memory allocated for the record.

20.	getAttr ():
Gets the attribute based on the attribute number and fills the appropriate data type of the Value field.

21.	setAttr () :
Converts the attribute to string and fills the Record data field.

