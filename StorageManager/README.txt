Assignment 1 - Storage Manager


***Group No. 2: Team members
1. Sneha Shahaji Kadam (skadam5@hawk.iit.edu)
2. Ayelet Bendor (abendor@hawk.iit.edu)
3. Naga Sirisha Chalapati (nchalapati@hawk.iit.edu)

***Homework Assignment 1 has been completed and following files were uploaded under Assignment1 folder:
1. dberror.h
2. storage_mgr.h
3. test_helper.h
4. dberror.c
5. storage_mgr.c
6. test_assign1_1.c
7. README.txt
8. Makefile

***Steps to compile and execute program is as follows:
1. Open terminal and navigate to project directory.
example: cd CS525/Assignment1
2. Type make to compile all source code files including "test_assign1_1.c" file.
3. Execute program using executable file.
example:  ./assign1
4. Type "make clean" to delete old compiled files.
5. We can check memory leaks with valgrind using following command:
	valgrind --leak-check=full --track-origins=yes ./assign_1 

***Assumptions made per code design:
1. Write operation writes an entire block size 
2. writeBlock method writes to an existing block on file
3. To add a new block to file we invoke appendEmptyBlock or ensureCapacity (and send in method the total number of pages we wish to include in the file)

***Methods implemented in storage_mgr.c are as follows:
/* manipulating page files */
Note: header_offset is defined which reserves space of 4 bytes at the beginning of the file.

1. initStorageManager():
Set file pointer to be null.

2. createPageFile():
Creates a new page file with a file size of one page and fills this single page with '\0' bytes.

3. openPageFile(): 
Opens an existing page file. 
This method returns RC_FILE_NOT_FOUND if the file does not exist.
If opening the file is successful, then the fields of the file handle will be initialized with the information about the opened file. 

4. closePageFile():
        Closes an open page file.

5. destroyPageFile():
        Deletes a page file from disk.

/* reading blocks from disc */
1. readBlock ():
        Reads the pageNum block from a file to memory

2. getBlockPos():
Returns the current page position in a file.

3. readFirstBlock():
Read the first page of the file. Calls readBlock method where pageNum = 0

4. readPreviousBlock():
Reads the Previous page relative to the value of curPagePos. This method internally calls the readBlock()  method to read the page. 
If a page before the first page is attempted to read, then it returns the error RC_READ_NON_EXISTING_PAGE.

5. readCurrentBlock():
Reads the current page relative to the value of curPagePos. This method too internally calls the readBlock()  method to read the page.

6. readNextBlock():
Reads the current page relative to the curPagePos of the file. 
If the user tries to read a block which is the first page after the last page of the file, the method will return RC_READ_NON_EXISTING_PAGE.

7. readLastBlock():
Reads last page of the file. Calls readBlock method where pageNum = totalNumPages-1

/* writing blocks to a page file */
1. writeBlock():
Writes a page to file using an absolute page position.

2. writeCurrentBlock():
Writes a page to disk using current page position. 

3. appendEmptyBlock():
Increases the number of pages in the file by one. The new last page is filled with zero bytes.

4. ensureCapacity(): 
If the pages in file are less than numberOfPages then add extra pages to the file so that the total number of pages in file matches numberOfPages. This method internally calls appendEmptyBlock() method to add extra pages.

***Additional test cases are implemented in testMultiplePageContent() function:
This test case consists of scenarios with multiple pages in a file to mainly test the read and write methods implemented in storage_mgr.c
1. The method starts with createPageFile which creates a file (test_pagefile.bin). 
2. Then openPageFile creates a new page (Page0) of zero bytes and initializes the total number of pages (totalNumPages) and the current page position (cuPagePos). 
3. A negative scenario of readPreviousBlock and readNextBlock are tested where RC_READ_NON_EXISTING_PAGE must be returned since the curPagePos is 0.
4. readCurrentBlock method must read the current page of index 0 and return RC_OK.
5. By using ensureCapacity, a total of 5 pages are created which internally calls appendEmptyBlock. The appendEmptyBlock creates 4 new pages of 0 bytes.
6. Test for the writeBlock method is done by writing to Page1 and Page2, with characters ‘1’ and ‘2’ respectively. The curPagePos keeps updating to 1 and then to 2 respectively.
7. Read methods are tested using the readPreviousBlock which should read Page1 and update the curPagePos to 1, then readNextBlock which should read Page2 and update curPagePos to 2. readLastBlock reads Page4 which is empty and updates curPagePos to 4.
8. writeCurrentBlock is then called which writes character 4 to the entire Page4 and updates the curPagePos to 4. readCurrentBlock then reads the block which is written.
9. Methods for closing the file and destroying it are called which then completes the test case.


***Additional test cases are implemented in testNonExistingBlock():
The test case goal is showcasing scenarios where we send a non existing block parameters (pageNum variable) to methods to ensure code validation in each function is working properly. 
First, we create a new file and append an additional 1 block. The current file contains the total of 2 blocks. 
Next, we envoke readBlock and writeBlock methods. In both cases we send to function pageNum = 2, which is the 3rd block. 
As expected both functions will fail and output to terminal the correct error output. 
Last, we close the file and delete it from the disk. We call delete function again, this time the file is already removed from the disk, therefore we expect an error.