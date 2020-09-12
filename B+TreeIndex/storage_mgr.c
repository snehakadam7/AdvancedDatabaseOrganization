#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include "storage_mgr.h"

FILE *filepointer;

void initStorageManager (void)
{
    // Initiate storage manager.
    printf("initiate storage manager!\n");
    filepointer=NULL;
}

RC createPageFile(char *fileName)
{
    printf("filename is %s\n",fileName);
    filepointer=fopen(fileName,"w+"); //open file to perform write operation

    if(filepointer == NULL)
    {
        printf("File not created\n");
        return RC_FILE_NOT_FOUND;
    }
    else
    {
        char *pagefile=malloc(PAGE_SIZE); //allocating space in memory of size PAGE_SIZE
        fseek(filepointer,header_offset,SEEK_SET);
        memset(pagefile,'\0',PAGE_SIZE);
        fwrite(pagefile, 1, PAGE_SIZE, filepointer); //fill the first block after the header with '\0'
        printf("%d bytes of space reserved at the beginning of a file.\n",header_offset);
        free(pagefile); // after writing to file we free allocated memory
        fclose(filepointer);
        return RC_OK;
    }
}

RC openPageFile(char *fileName, SM_FileHandle *fHandle)
{
    printf("inside openPageFile method\n");
    filepointer = fopen(fileName, "r+"); //open file to perform read operation
    if(filepointer == NULL)  //Condition to check if file exists
    {
        return RC_FILE_NOT_FOUND;
    }
    else
    {
        fseek(filepointer,0L, SEEK_END);
        long size = ftell(filepointer); //size of the file

        //Initializing the file handle for the opened file
        fHandle->fileName = fileName; //initialize file handle
        fHandle->totalNumPages = size/PAGE_SIZE - 1;
        fHandle->curPagePos = 0;
        printf("File has %d page.\n", fHandle->totalNumPages);
        return RC_OK;
    }
}

RC closePageFile (SM_FileHandle *fHandle)
{
    if(filepointer != NULL)
    {
        if (fclose(filepointer) == 0) //removing the file successfully should return 0
        {
            printf("file closed.\n");
            return RC_OK;
        }
        else
        {
            return RC_FILE_NOT_FOUND;
        }
    }

}

RC destroyPageFile (char *fileName)
{
    if(filepointer != NULL) //checking file exists
    {
        if (remove(fileName) != 0) //removing the file successfully should return 0
        {
            return RC_DESTROY_FAILED;
        }
        else
        {
            printf("file destroyed successfully\n");
            return RC_OK;
        }
    }
}


RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    //Check if page number is a valid number
    if (pageNum < 0 || fHandle->totalNumPages < pageNum + 1)
    {
        return RC_READ_NON_EXISTING_PAGE;
    }
    int offset = (pageNum)*PAGE_SIZE+header_offset;     //Calculate the offset
    if (fseek(filepointer, offset, SEEK_SET) == 0) //place the cursor at the beginning of the block
    {
        fread(memPage, sizeof(char), PAGE_SIZE, filepointer); //read from a file and store in memory
    }
    else
    {
        printf("failed to read block number:%d\n", pageNum);
        return RC_READ_NON_EXISTING_PAGE;
    }
    fHandle->curPagePos = pageNum; //update curPagePos to store current read page
    printf("Read block %d sucessfully!\n", pageNum);
    return RC_OK;
}

int getBlockPos (SM_FileHandle *fHandle)
{
    printf("inside method getBlockPos.\n");
    int position = fHandle->curPagePos;
    printf("Current page position: %d\n",position);
    return position;    //Return the current page position in a file.
}

RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    //Read first block of the file.
    printf("inside method readFirstBlock\n");
    int page_number=0;
    printf("page number to read:%d\n",page_number);
    return readBlock(page_number, fHandle, memPage);
}

RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    //Read previous block relative to the current page position of the file
    printf("inside method readPreviousBlock\n");
    printf("Current page position :%d\n",fHandle->curPagePos);
    printf("page number to read:%d\n",fHandle->curPagePos - 1);
    return readBlock(fHandle->curPagePos - 1, fHandle, memPage);
}

RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    //Read current block relative to the current page position of the file
    printf("inside method readCurrentBlock\n");
    printf("Current page position :%d\n",fHandle->curPagePos);
    return readBlock(fHandle->curPagePos, fHandle, memPage);
}

RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    //Read next block relative to the current page position of the file
    printf("inside method readNextBlock\n");
    printf("Current page position :%d\n",fHandle->curPagePos);
    printf("page number to read:%d\n",fHandle->curPagePos + 1);
    return readBlock(fHandle->curPagePos + 1, fHandle, memPage);
}

RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    //Read last block of the file.
    printf("inside method readLastBlock\n");
    printf("Current page position :%d\n",fHandle->curPagePos);
    int page_number = (fHandle->totalNumPages)-1;
    printf("page number to read:%d\n",page_number);
    return readBlock(page_number,fHandle,memPage);
}

RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    //Check if page number is a valid number
    if (pageNum < -1 || fHandle->totalNumPages < pageNum + 1)
    {
        return RC_READ_NON_EXISTING_PAGE;
    }

    if (pageNum == -1)
    {
        printf("header page\n"); // consider header page as pageNum=-1
        fseek(filepointer, 0L, SEEK_SET);
        fwrite(memPage, sizeof(char), PAGE_SIZE, filepointer);
    }
    if (pageNum >=0)
    {
        int offset = pageNum*PAGE_SIZE + header_offset;
        if (fseek(filepointer, offset, SEEK_SET) != 0) //place the cursor at the beginning of the block
        {
            return RC_READ_NON_EXISTING_PAGE;
        }
        printf("inside writeBlock - about to write block.\n");
        //write returns the number of read bytes from a file, any value different than PAGE_SIZE means we didn't write the entire block to file
        if (fwrite(memPage, sizeof(char), PAGE_SIZE, filepointer) != PAGE_SIZE)
        {
            return RC_WRITE_FAILED;
        }
    }
    fHandle->curPagePos = pageNum; //update curPagePos to store current written page
    printf("writing done successfully!\n");
    return RC_OK;
}

RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    //Write current block relative to the current page position of the file
    printf("Current page position :%d\n",fHandle->curPagePos);
    printf("page number to write:%d\n",fHandle->curPagePos);
    return writeBlock(fHandle->curPagePos, fHandle, memPage);
}

RC appendEmptyBlock (SM_FileHandle *fHandle)
{
    // Append an empty block (of size=PAGE_SIZE) at the end of the file.
    char *pagefile=malloc(PAGE_SIZE);
    memset(pagefile,'\0',PAGE_SIZE);
    if(filepointer == NULL)
    {
        return RC_FILE_NOT_FOUND;
    }
    else
    {
        fseek(filepointer, 0L, SEEK_END);
        fwrite(pagefile, 1, PAGE_SIZE, filepointer);
        long size = ftell(filepointer);
        fHandle->totalNumPages = size/PAGE_SIZE -1  ;
        free(pagefile);
        printf("Empty block appended!\n");
        return RC_OK;
    }
}

RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle)
{
    printf("inside ensureCapacity method\n");
    int actual_pages=fHandle->totalNumPages;
    int i;
    //if current #of pages is smaller then numberOfPages, we append to the end of the file the difference
    if(actual_pages < numberOfPages)
    {
        int extra_pages= numberOfPages - actual_pages;
        printf("%d extra_pages added!\n",extra_pages);
        for(i=0; i < extra_pages; i++)            //add extra pages to attain the required capacity
        {
            appendEmptyBlock(fHandle);       //call to appendEmptyBlock()
        }
    }
    return RC_OK;
}
