#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <unistd.h>

#include "buffer_mgr.h"
#include "storage_mgr.h"

// Structure for one page frame in buffer pool.
typedef struct PageFrame
{
    SM_PageHandle smdata; // Actual data of the page
    PageNumber pagenumber;
    bool isDirty; // indicates whether the page has been modified by the client.
    int fixCount; // indicates the number of clients that have pinned the page at a time.
    long long int current_time;
} PageFrame;

void fifoStrategy(BM_BufferPool *const bm, PageFrame *newFrame, int pageNum);

int returnFrameNumberOfPage(int totalPages, PageFrame *frame, int pageNum);

// TA's comment: Too many global variables is not good for encapsulation.

int fifoIndex;  // used for FIFO replacement strategy;

// indicates total number of page frames that can be kept into the buffer pool.
int bufferSize = 0;

// keeps track of number of pages read from the disk.
int readCount;
// keeps track of number of pages written to the disk.
int writeCount;

SM_FileHandle fh;

struct timeval timer_usec;

//initialize buffer pool for an existing page file
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
    printf("inside initBufferPool method.\n");
    int message = openPageFile((char *)pageFileName, &fh); // page file should already exist.
    if(message != RC_OK)
    {
        printf("file not found\n");
        return RC_FILE_NOT_FOUND;
    }
    bm->pageFile = (char *)pageFileName;
    // TA's comment: Why do you need both bm->numPages and bufferSize?
    bm->numPages = numPages;
    bm->strategy = strategy;

    // Allocate memory to pageframe
    PageFrame *frame = malloc(sizeof(PageFrame) * numPages);

    // Buffer Size is the total number of pages in the buffer pool.
    bufferSize = numPages;
    printf("buffer size:%d\n",bufferSize);

    // Initialize all variables in a page frame.
    for(int i = 0; i < bufferSize; i++)
    {
        // TA's comment: you can allocate memory for smdata here. 
        frame[i].smdata = NULL;
        frame[i].pagenumber = NO_PAGE; //NO_PAGE has value of -1.
        frame[i].isDirty = false;
        frame[i].fixCount = 0;
        frame[i].current_time = (!gettimeofday(&timer_usec, NULL)) ? ((long long int) timer_usec.tv_sec) * 1000000ll + (long long int) timer_usec.tv_usec : -1;
    }
    bm->mgmtData = frame; // The page frame is stored in the buffer pool variable.
    printf("BufferPool initiated successfully!\n");
    readCount = 0;
    writeCount = 0;
    return RC_OK;
}

// This method removes all the pages from the memory to free up all resources and memory space.
RC shutdownBufferPool(BM_BufferPool *const bm)
{
    if(bm->mgmtData == NULL)
    {
        return RC_NOT_OK;
    }
    forceFlushPool(bm);
    closePageFile(&fh);
    PageFrame *page_frame = (PageFrame *)bm->mgmtData;

    int i;
    for(i = 0; i < bufferSize; i++)
    {
        if(page_frame[i].fixCount != 0)
        {
            return RC_PAGE_PINNED;
        }
    }

    free(page_frame);
    bm->mgmtData = NULL;
    printf("Completed BufferPool shutdown!\n");
    return RC_OK;
}

// This method writes all the dirty pages to disk.
RC forceFlushPool(BM_BufferPool *const bm)
{
    if(bm->mgmtData == NULL)
    {
        return RC_NOT_OK;
    }
    PageFrame *page_frame = (PageFrame *)bm->mgmtData;

    int i;
    for(i = 0; i < bufferSize; i++)
    {
        if(page_frame[i].fixCount == 0 && page_frame[i].isDirty == 1)
        {
            writeBlock(page_frame[i].pagenumber, &fh, page_frame[i].smdata);
            writeCount++;
            page_frame[i].isDirty = 0;
        }
    }
    return RC_OK;
}

// This method marks the page as dirty indicating that page has been modified.
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    PageFrame *frame = (PageFrame *)bm->mgmtData;
    for (size_t i = 0; i < bm->numPages; i++)
    {
        if(frame[i].pagenumber == page->pageNum) //getting the frame in which the page from disk is stored
        {
            //frame[i].current_time = (!gettimeofday(&timer_usec, NULL)) ? ((long long int) timer_usec.tv_sec) * 1000000ll + (long long int) timer_usec.tv_usec : -1;
            frame[i].isDirty = true;
            printf("marked modified page as Dirty\n");
            return RC_OK;
        }
    }
    return RC_READ_NON_EXISTING_PAGE;
}

// This method writes the current content of the page back to the page file on disk.
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    PageFrame *page_frame = (PageFrame *)bm->mgmtData;

    for(int i = 0; i < bm->numPages; i++)
    {
        if(page_frame[i].pagenumber == page->pageNum)
        {
            writeBlock(page_frame[i].pagenumber, &fh, page_frame[i].smdata);
            page_frame[i].isDirty = 0;
            writeCount++;
            return RC_OK;
        }
    }
    return RC_READ_NON_EXISTING_PAGE;
}

//This method returns an array of page numbers.
PageNumber *getFrameContents (BM_BufferPool *const bm)
{
    printf("inside getFrameContents method.\n");
    PageNumber *page_numbers = malloc(sizeof(PageNumber)*bufferSize);
    PageFrame *page_frame= (PageFrame *)bm->mgmtData;

    for(int i = 0; i < bufferSize; i++)
    {
        page_numbers[i] = page_frame[i].pagenumber ;
    }
    return page_numbers;
}

// This method returns an array of dirty flags.
bool *getDirtyFlags (BM_BufferPool *const bm)
{
    printf("inside getDirtyFlags method.\n");
    bool *dirty_flags = malloc(sizeof(bool)*bufferSize);
    PageFrame *page_frame= (PageFrame *)bm->mgmtData;

    for(int i=0; i < bufferSize; i++)
    {
        dirty_flags[i] = page_frame[i].isDirty;
    }
    return dirty_flags;
}

// This function returns an array of fix counts.
int *getFixCounts (BM_BufferPool *const bm)
{
    printf("inside getFixCounts method.\n");
    int *fix_counts = malloc(sizeof(int)*bufferSize);
    PageFrame *page_frame= (PageFrame *)bm->mgmtData;

    for(int i=0; i < bufferSize; i++)
    {
        fix_counts[i] = page_frame[i].fixCount;
    }
    return fix_counts;
}

// This function returns the number of pages that have been read from disk since a buffer pool has been initialized.
int getNumReadIO (BM_BufferPool *const bm)
{
    printf("number of pages that have been read:%d\n",readCount);
    return readCount;
}

// This function returns the number of pages written to the page file since the buffer pool has been initialized.
int getNumWriteIO (BM_BufferPool *const bm)
{
    printf("number of pages written to the page file:%d\n",writeCount);
    return writeCount;
}

// This function adds the page with page number pageNum to the buffer pool.
// If the buffer pool is full, then it uses page replacement strategy.
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
    if(pageNum < 0)
    {
        return RC_READ_NON_EXISTING_PAGE;
    }

    if(bm->mgmtData == NULL)
    {
        return RC_NOT_OK;
    }

    int replace=1;
    PageFrame *frame= (PageFrame *)bm->mgmtData;
    for(int i=0; i<bufferSize; i++)
    {
        if(frame[i].pagenumber != -1)
        {
            if(frame[i].pagenumber == pageNum)  //Page is already in memory
            {
                frame[i].fixCount++;
                page->pageNum = pageNum;
                page->data = frame[i].smdata;
                sleep(0.7);
                frame[i].current_time = (!gettimeofday(&timer_usec, NULL)) ? ((long long int) timer_usec.tv_sec) * 1000000ll + (long long int) timer_usec.tv_usec : -1;
                replace=0;
                break;
            }

        }
        else	//buffer has space to add a page
        {
            // TA's comment: ensureCapacity(pageNum, &fh); ?
            ensureCapacity(bufferSize,&fh);
            frame[i].smdata = (SM_PageHandle) malloc(PAGE_SIZE);
            // TA's comment: Error handling?
            readBlock(pageNum, &fh, frame[i].smdata);
            readCount++;
            frame[i].pagenumber = pageNum;
            frame[i].fixCount = 1;
            sleep(0.2);
            frame[i].current_time = (!gettimeofday(&timer_usec, NULL)) ? ((long long int) timer_usec.tv_sec) * 1000000ll + (long long int) timer_usec.tv_usec : -1;
            page->pageNum = pageNum;
            page->data = frame[i].smdata;
            replace=0;
            break;
        }
    }
    if (replace==1)  // if bufferpool is full, do replacement
    {
        PageFrame *frame= (PageFrame *)bm->mgmtData;
        int pages_pinned=0;
        for(int i=0; i<bufferSize; i++)
        {
            // TA's comment: frame[i].fixCount > 0? 
            if (frame[i].fixCount==1)
            {
                pages_pinned++;
            }
        }
        printf("pages_pinned:%d\n",pages_pinned);
        // TA's comment: pages_pinned == bufferSize or pages_pinned == bm->numPages ?
        if (pages_pinned>2)  // if all pages are pinned then no replacement
        {
            printf("all pages are pinned.\n");
            return RC_NOT_OK;
        }

        switch(bm->strategy)  // Perform replacement
        {
        case RS_FIFO: // Using FIFO strategy
        {
            PageFrame *newPage = (PageFrame *) malloc(sizeof(PageFrame));
            // TA's comment: Should not appendEmptyBlock.
            appendEmptyBlock(&fh);
            printf("do fifo\n");
            // TA's comment: You can malloc in init.
            newPage->smdata = (SM_PageHandle) malloc(PAGE_SIZE);
            readBlock(pageNum, &fh, newPage->smdata);
            newPage->pagenumber = pageNum;
            newPage->fixCount=1;
            newPage->isDirty=0;
            page->pageNum = pageNum;
            page->data = newPage->smdata;
            fifoStrategy(bm, newPage,pageNum);
        }
        break;

        case RS_LRU:  // Using LRU strategy
        {
            int index = 0;
            bool pageReplaced = false;
            PageNumber *sortedLRUPageNum = sortPageNumberbyTimestamp(bm);
            int LRUPage = sortedLRUPageNum[index];
            while(!pageReplaced)
            {
                if(index < bm->numPages-1)
                {
                    int LRUPage = sortedLRUPageNum[index];
                    for (size_t i = 0; i < bm->numPages; i++)
                    {
                        if(frame[i].pagenumber == LRUPage)
                        {
                            if(frame[i].fixCount == 0)
                            {
                                //check for dirty, clear and write to disk and then read the new page
                                if(frame[i].isDirty)
                                {
                                    writeBlock(frame[i].pagenumber, &fh, frame[i].smdata);
                                    writeCount = writeCount + 1;
                                    frame[i].isDirty=false;
                                }
                                // read the new block into that frame.
                                readBlock(pageNum, &fh, page->data);
                                page->pageNum = pageNum;
                                frame[i].smdata = page->data;
                                frame[i].pagenumber = pageNum;
                                sleep(0.7);
                                frame[i].current_time = (!gettimeofday(&timer_usec, NULL)) ? ((long long int) timer_usec.tv_sec) * 1000000ll + (long long int) timer_usec.tv_usec : -1;
                                frame[i].fixCount = frame[i].fixCount + 1;
                                readCount++;
                                pageReplaced = true;
                                printf("Replaced page by LRU strategy.\n");
                            }
                            else
                            {
                                index++;
                            }
                            break;
                        }
                    }
                }
            }
        }
        break;
        default:
            printf("%i", bm->strategy);
            break;
        }
    }
    return RC_OK;
}

// FIFO replacement strategy
void fifoStrategy(BM_BufferPool *const bm, PageFrame *newFrame,int pageNum)
{
    printf("inside fifoStrategy method.\n");
    fifoIndex=readCount%bufferSize;
    PageFrame *page_frame = (PageFrame *) bm->mgmtData;

    for(int i = 0; i < bufferSize; i++)
    {
        if(page_frame[fifoIndex].fixCount == 0)
        {
            if(page_frame[fifoIndex].isDirty == 1)
            {
                writeBlock(page_frame[fifoIndex].pagenumber, &fh, page_frame[fifoIndex].smdata);
                writeCount++; // increment writeCount once block written to the disk.
            }
            printf("updating page frame's content to new page's content.\n");
            page_frame[fifoIndex].smdata = newFrame->smdata;
            page_frame[fifoIndex].pagenumber = newFrame->pagenumber;

            page_frame[fifoIndex].isDirty = newFrame->isDirty;
            page_frame[fifoIndex].fixCount = newFrame->fixCount;
            sleep(0.5);
            page_frame[fifoIndex].current_time = (!gettimeofday(&timer_usec, NULL)) ? ((long long int) timer_usec.tv_sec) * 1000000ll + (long long int) timer_usec.tv_usec : -1;

            fifoIndex++;
            if (fifoIndex % bufferSize == 0)
            {
                fifoIndex = 0;
            }
            readCount++;
            break;
        }
        else
        {

            fifoIndex++;
            if (fifoIndex % bufferSize == 0)
            {
                fifoIndex = 0;
            }

        }
    }
}

// This method unpins a page from the memory.
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    // TA's comment: If you accidentally unpin twice, page->pageNum will be negative.
    PageFrame *frame = (PageFrame *)bm->mgmtData;
    int i;
    for (i=0 ; i < (bm->numPages); i++)
    {
        if(frame[i].pagenumber == page->pageNum)
        {
            frame[i].fixCount--;
            printf("unpinPage: %d\n", page->pageNum);
            return RC_OK;
        }
    }
    return RC_READ_NON_EXISTING_PAGE;
}

PageNumber *sortPageNumberbyTimestamp(BM_BufferPool *const bm)
{
    PageFrame *frame= (PageFrame *)bm->mgmtData;
    PageNumber *sorted_page_numbers = getFrameContents(bm);
    int i, j, min_idx;

    for (i = 0; i < bm->numPages; i++)
    {
        // Find the element with the least timestamp
        min_idx = i;
        for (j = i + 1; j < bm->numPages; j++)
        {
            int a = returnFrameNumberOfPage(bm->numPages, frame, sorted_page_numbers[j]);
            int b = returnFrameNumberOfPage(bm->numPages, frame, sorted_page_numbers[min_idx]);
            if (frame[a].current_time < frame[b].current_time)
                min_idx = j;
        }
        // Place the found minimum element in an order to form a sorted page numbers w.r.t timestamp
        orderPageNumber(&sorted_page_numbers[min_idx], &sorted_page_numbers[i]);
    }
    return sorted_page_numbers;
}

int returnFrameNumberOfPage(int totalPages, PageFrame *frame, int pageNum)
{
    for (size_t i = 0; i < totalPages; i++)
    {
        if(frame[i].pagenumber == pageNum)
        {
            return i;
        }
    }
}

void orderPageNumber(int *xp, int *yp)
{
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}
