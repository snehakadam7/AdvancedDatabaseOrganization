Assignment 2 - Buffer manager 


***Group No. 2: Team members
1. Sneha Shahaji Kadam (skadam5@hawk.iit.edu)
2. Ayelet Bendor (abendor@hawk.iit.edu)
3. Naga Sirisha Chalapati (nchalapati@hawk.iit.edu)


***Homework Assignment 2 has been completed and following files were uploaded under Assignment2 folder:


1. Makefile
2. buffer_mgr.c
3. buffer_mgr.h
4. buffer_mgr_stat.c
5. buffer_mgr_stat.h
6. dberror.c
7. dberror.h
8. dt.h
9. storage_mgr.h
10. test_assign2_1.c
11. test_assign2_2.c
12. test_helper.h


***Steps to compile and execute program is as follows:
1. Open terminal and navigate to project directory.
example: cd CS525/Assignment2
2. Type make to compile source code files.
By default "make" will compile test_assign2_1.c(test case file).
For compiling source code file test_assign2_2.c, type "make test_assign2_2".
3. Execute program using executable file.
example:  ./test_assign2_1 or ./test_assign2_2
4. Type "make clean" to delete old compiled files.
5. We can check memory leaks with valgrind using following command:
        valgrind --leak-check=full --track-origins=yes ./test_assign2_1 


***Assumptions made per code design:
1. Write operation writes an entire block size 
2. writeBlock method writes to an existing block on file
3. To add a new block to file we invoke appendEmptyBlock or ensureCapacity (and send in method the total number of pages we wish to include in the file)
4. Per assignment 2, we performed a memory leak test, the results showed that our program has  some memory leak. 
In FIFO method, we create a new pointer of type pageFrame and allocate memory using malloc to read from the disk to memory. Next, this new pointer is sent to fifoStrategy() function, to replace one of the existing pages in memory with the data of the new pointer. 
The memory block that is occupied by the new pointer is not being freed, we realized it only after performing Valgrind test. 
One of the methods we implemented to remove the memory leak was using realloc to the existing frame structure. The idea was to expand the memory occupied by frame structure by 1 and read from the disk to the new memory block. When we ran Valgrind we saw 0 memory leaks and some errors. 
Due to the lack of time until submission, we decided to go with a program that we tested and runs without error although it contains memory leaks, but we will fix this issue.   
5. For LRU method, the array is sorted by a timestamp of when the frame was last used. When executing the test cases, all dummy pages are created at once with a minor time interval between one another. 
We choose to use a small time unit per timestamp, which is microseconds. Even when using microseconds, LRU algorithm were not always working as expected. 
To overcome this challenge, we used a sleep function set to 0.1 seconds before adding a timestamp. 

***Methods implemented in buffer_mgr.c are as follows:

// Buffer Manager Interface Pool Handling
      1. initBufferPool():
Send bm pointer and metadata (such as number of pages it holds, replacement strategy, and a pointer to stored frames) to initialize the buffer pool and frame structure.
In addition, we keep a reference to frame structure in bm->mgmtData = frame.

      2. shutdownBufferPool():
Deleting buffer pool manager structure and its stored metadata by freeing memory occupied by the manager buffer on the heap memory. Before shutting down the buffer pool manager, we perform a check that fixCount =0 and write all dirty pages back to file.


         3. forceFlushPool():
Writing all dirty pages in the buffer pool manager back to file. 


// Buffer Manager Interface Access Pages
            4. markDirty ():
Mark a page sent to function by a pointer as dirty

            5. unpinPage ():
Decrement fixCount by 1.

            6. forcePage ():
Write a specific page to file. Page number to write to file is passed to function via pointer.

            7. pinPage ():
Function receives a page number to add to a frame. If the page already exists in the buffer pool manager, it will increase the fixCount and update the page pointer to point to the area in the memory where the page is stored and the page number .


// Statistics Interface
               8. *getFrameContents ():
Send a buffer manager pool pointer to return 

               9. *getDirtyFlags ():
Return an array of dirty flags

               10. *getFixCounts ():
Return array of fix counts 

               11. getNumReadIO ();
Return the number of pages read from page file to the memory

               12. getNumWriteIO ():
Return the number of pages written from memory to page file


//Additional functions
                  13. fifoStrategy():
Swap the oldest frame in the memory with a page from disk.  

                  14. SortPageNumberByTimestamp(): 
Gets the page numbers of all the frames through FrameContents and sorts them based on the timestamps of the pages and returns the sorted page numbers in ascending order to the LRU strategy case in the pin page. This method uses the below two methods.

                  15. returnFrameNumberOfPage(): 
Method returns the index of the frame for a given page number.

                  16. orderPageNumber(): 
Swaps the position of the two elements sent to the method.


//Page replacement strategies 
FIFO strategy - First In First Out. When the memory buffer is full and a request for a page comes in, we need to perform a swamp (assuming the page is not cached in memory).
According to fifo, we will replace the first frame that was stored in the memory in the memory with a page from the disk. To keep a track of the first first page we implement a global variable name fifoIndex. fifoIndex points to the oldes frame in the buffer, so when we need to perform a swap, we'll take the frame points to and replace it with the new frame and increment fifoIndex by 1. We operate under the assumption that we keep the pages in the order of their arrival, so the first one will be stored in frame 0, second in frame 2 and so on. 


LRU strategy - The Least Recently Used (LRU) strategy is a case for replacement strategy used in pin page for replacing the pages when the buffer is full. The first element from SortPageNumberByTimestamp() gives the page number of the LRU page in the buffer. The frame of this page is checked if the page is pinned (fixcount > 0) and then if the page is modified (dirty flag set to true). If the page is not pinned and is dirty, the page in the frame is written back to disk and evicted from the frame. The new page is then read into this empty frame which completes the replacement.