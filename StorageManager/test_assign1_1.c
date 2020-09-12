#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"

/* prototypes for test functions */
static void testCreateOpenClose(void);
static void testSinglePageContent(void);
static void testNonExistingBlock(void);
static void testMultiplePageContent(void);

/* main function running all tests */
int
main (void)
{
  testName = "";

  initStorageManager();
  testCreateOpenClose();
  testSinglePageContent();
  testNonExistingBlock();
  testMultiplePageContent();

  return 0;
}


/* check a return code. If it is not RC_OK then output a message, error description, and exit */
/* Try to create, open, and close a page file */
void
testCreateOpenClose(void)
{
  SM_FileHandle fh;

  testName = "test create open and close methods";

  TEST_CHECK(createPageFile (TESTPF));

  TEST_CHECK(openPageFile (TESTPF, &fh));
  ASSERT_TRUE(strcmp(fh.fileName, TESTPF) == 0, "filename correct");
  ASSERT_TRUE((fh.totalNumPages == 1), "expect 1 page in new file");
  ASSERT_TRUE((fh.curPagePos == 0), "freshly opened file's page position should be 0");

  TEST_CHECK(closePageFile (&fh));
  TEST_CHECK(destroyPageFile (TESTPF));

  // after destruction trying to open the file should cause an error
  ASSERT_TRUE((openPageFile(TESTPF, &fh) != RC_OK), "opening non-existing file should return an error.");

  TEST_DONE();
}

/* Try to create, open, and close a page file */
void
testSinglePageContent(void)
{
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test single page content";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  // create a new page file
  TEST_CHECK(createPageFile (TESTPF));
  TEST_CHECK(openPageFile (TESTPF, &fh));
  printf("created and opened file\n");


  // read first page into handle
  TEST_CHECK(readFirstBlock (&fh, ph));
  // the page should be empty (zero bytes)
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page of freshly initialized page");
  printf("first block was empty\n");

  // change ph to be a string and write that one to disk
  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = (i % 10) + '0';
  TEST_CHECK(writeBlock (0, &fh, ph));
  printf("writing first block\n");

  // read back the page containing the string and check that it is correct
  TEST_CHECK(readFirstBlock (&fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("reading first block\n");

  TEST_CHECK(closePageFile (&fh));

  // destroy new page file
   TEST_CHECK(destroyPageFile (TESTPF));

  TEST_DONE();
  free(ph);
}

void testNonExistingBlock(void){
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  testName = "test read, write and deleting a non-existing block";

  TEST_CHECK(createPageFile(TESTPF));
  printf("File created succesfully!\n");

  TEST_CHECK(openPageFile (TESTPF, &fh));
  printf("New file has been create\n");

  ASSERT_TRUE((fh.totalNumPages == 1), "expect 1 page in new file");
  ASSERT_TRUE((fh.curPagePos == 0), "freshly opened file's page position should be 0");
  TEST_CHECK(readFirstBlock (&fh, ph));
  TEST_CHECK(appendEmptyBlock (&fh));
  printf("appending an empty block at the end of the file\n");
  ASSERT_TRUE((fh.totalNumPages == 2), "expect 2 page in new file");

  //Filling the second block (block number 1) with sequential numbers 0...9
  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = (i % 10) + '0';
  printf("writing second block\n");
  TEST_CHECK(writeBlock (1, &fh, ph)); //write to file
  printf("reading second block\n");
  ASSERT_TRUE((readBlock(1, &fh, ph) == RC_OK), "reading second block of a file");

  //Attempt to read and write to a non existing block to ensure proper validation
  ASSERT_TRUE((readBlock(2, &fh, ph) != RC_OK), "reading a non existing block should return an error.");
  ASSERT_TRUE((writeBlock(2, &fh, ph) != RC_OK), "writing a non existing block should return an error.");

  TEST_CHECK(closePageFile (&fh));
  ASSERT_TRUE((destroyPageFile (TESTPF) == RC_OK), "destroyed TESTPF successfully");
  //Calling destroy method with non existing file
  ASSERT_TRUE((destroyPageFile (TESTPF)!= RC_OK), "destroying non existing file should return an error.");
  TEST_DONE();
  free(ph);
}

void testMultiplePageContent(void){

  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test multiple page content";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  TEST_CHECK(createPageFile (TESTPF));
  TEST_CHECK(openPageFile (TESTPF, &fh));
  printf("created and opened file\n");

  getBlockPos(&fh);// getBlockPos method returns the current page position.

  ASSERT_TRUE((readPreviousBlock(&fh, ph) == RC_READ_NON_EXISTING_PAGE), "trying to open non-existing block of file.");
  ASSERT_TRUE((readNextBlock(&fh, ph) == RC_READ_NON_EXISTING_PAGE), "trying to open non-existing block of file.");
  ASSERT_TRUE((readCurrentBlock(&fh, ph) == RC_OK), "current block");

  // creating empty pages with ensureCapacity and then writing to that block
  TEST_CHECK(ensureCapacity (5,&fh));
  ASSERT_TRUE((fh.totalNumPages == 5), "5 pages in the file");

  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = '1'; //filling the entire block with 1's
  TEST_CHECK(writeBlock (1, &fh, ph));
  printf("writing block to second page (i.e. Page1)\n");

  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = '2'; //filling the entire block with 2's
  TEST_CHECK(writeBlock (2, &fh, ph));
  printf("writing block to third page (i.e. Page2)\n");

  TEST_CHECK(readPreviousBlock(&fh, ph)); // method must read 2nd block (or Page1) since the cursor is at the end of 3rd block
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == '1'), "character in page read from disk is the one we expected.");
  printf("reading previous block\n");

  TEST_CHECK(readNextBlock(&fh, ph)); // method must read 3rd block (or Page2) since the cursor is at the end of 2nd block
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == '2'), "character in page read from disk is the one we expected.");
  printf("reading next block\n");

  TEST_CHECK(readLastBlock(&fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == 0), "expected zero byte in lasts page");
  printf("last block was empty\n");

  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = '4'; //filling the entire block with 4's
  TEST_CHECK(writeCurrentBlock (&fh, ph));
  printf("writing block to fifth page (i.e. Page4)\n");

  TEST_CHECK(readCurrentBlock(&fh, ph)); // method must read page of index 4 (i.e. 5th page, since the cursor is at the end of that page)
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == '4'), "character in page read from disk is the one we expected.");
  printf("reading current block\n");

  // close file
  TEST_CHECK(closePageFile (&fh));

  // destroy new page file
   TEST_CHECK(destroyPageFile (TESTPF));

  TEST_DONE();
  free(ph);
}
