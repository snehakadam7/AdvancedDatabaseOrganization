#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <unistd.h>

#include "btree_mgr.h"
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

// Structure for B+ Tree meta-data
typedef struct BTreeManager_Metadata
{
    int entriesPerNode;
    int totalNumNodes;      // total number of nodes in B+ tree
    int totalNumEntries;    // total number of entries in B+ tree
    int nextFreePage;       // page number of next free page
    int firstLeafNode;      // page number of first leaf node
    DataType keyType;       // data-type of key
    BM_BufferPool *bf;
    BM_PageHandle *pg;
    struct Node *rootNode;
} BTreeManager_Metadata;

// Structure for a Node in B+ Tree
typedef struct Node
{
    int numKeys;    // total number of keys in node
    bool isLeafNode;        // indicate whether node is leaf node or non-leaf node
    struct Node *parent;
    struct Node *next;
    int nextPageNumber;
    int *keys;
    void **pointers;
    int nodeNumber;
} Node;

//Structure for scan operation on the B+ Tree
typedef struct ScanManager
{
    int totalKeysScanned;
    int numKeys;
    Node *node;
} ScanManager;


BTreeManager_Metadata *metadata;

int curKeyIndex;
int curPtrIndex;

int minKeysPerNode(BTreeManager_Metadata *metadata, Node *node);
int writePageData(Node *node);
int searchNodes(struct Node *pgHeader, int key, RID *result);

// find the key in the tree
RC findKey (BTreeHandle *tree, Value *key, RID *result)
{
    Node *node = metadata->rootNode;
    curKeyIndex = 0;
    curPtrIndex = 0;
    printf("*****Inside find key*****\n");

    int search_result = searchNodes(node, key->v.intV, result);

    if(search_result)
        return RC_OK;
    else
        return RC_IM_KEY_NOT_FOUND;
}

// recursively searches nodes of the B+ tree
int searchNodes(struct Node *pgHeader, int key, RID *result)
{
    RID *rid;
    int ptr, index;

    if(pgHeader != NULL)
    {
        if (!pgHeader->isLeafNode)
        {
            for (size_t i = 0; i < pgHeader->numKeys; i++)
            {
                if (i == pgHeader->numKeys - 1)
                {
                    if (pgHeader->pointers[pgHeader->numKeys] != NULL)
                    {
                        if (key < pgHeader->keys[i])
                        {

                            curPtrIndex = i;
                            pgHeader = pgHeader->pointers[i];
                        }

                        else
                        {

                            curPtrIndex = i+1;
                            pgHeader = pgHeader->pointers[i + 1];
                        }

                        if(searchNodes(pgHeader, key, result))
                            return 1;
                        else
                            return 0;
                    }
                    else
                    {
                        printf("Pointer is NULL!");
                        return 0;
                    }
                }
                else
                {
                    if (key < pgHeader->keys[i])
                    {
                        if (pgHeader->pointers[i] != NULL)
                        {
                            pgHeader = pgHeader->pointers[i];
                            if(searchNodes(pgHeader, key, result))
                                return 1;
                            else
                                return 0;
                        }
                        else
                        {
                            printf("Pointer is NULL!");
                            return 0;
                        }
                    }
                }
            }
        }
        else
        {
            for (size_t i = 0; i < pgHeader->numKeys; i++)
            {
                if (key == pgHeader->keys[i])
                {
                    curKeyIndex = i;
                    rid = (RID *)pgHeader->pointers[i];
                    *result = *rid;
                    printf("Key found %.d\n", key);
                    //status = RC_OK;
                    return 1;
                }
            }
            printf("Reached end of the tree. Key %d NOT found.\n", key);
            return 0;
        }
    }
}

// This method prints the B+ Tree
char *printTree (BTreeHandle *tree)
{
    char strTree[100];
    char *str = calloc(100, sizeof(char));
    BTreeManager_Metadata *treeData = (BTreeManager_Metadata *) tree->mgmtData;
    Node *node = treeData->rootNode;

    for (size_t i = 0; i < treeData->totalNumNodes; i++)
    {
        writePageData(node);
        sprintf(str, "%s(%d)[", str, i);

        int entries = 2*node->numKeys + 1;
        for (size_t j = 0; j < entries; j++)
        {
            if(j%2 == 0) //even (pointers)
            {
                if(!node->isLeafNode)
                {
                    if(node->pointers[j/2] != NULL)
                    {
                        Node *tmp_node = (Node *)node->pointers[j/2];
                        sprintf(str,"%s%d,", str, tmp_node->nodeNumber);
                        if(j == entries - 1)
                            node = node->pointers[i];
                    }
                }
                else
                {
                    if(node->pointers[j/2] != NULL)
                    {
                        if(j == entries - 1)
                        {
                            if(node->nextPageNumber > 0)
                                sprintf(str, "%s%d,", str, node->nextPageNumber);
                            node = node->pointers[j/2];
                        }
                        else
                        {
                            RID *rid;
                            rid = (RID *)node->pointers[j/2];
                            sprintf(str, "%s%d.%d,", str, rid->page, rid->slot);
                        }
                    }
                }
            }
            else //odd (keys)
            {
                sprintf(str, "%s%d,", str, node->keys[j/2]);
            }
        }
        str[strlen(str)-1] = '\0';
        sprintf(str, "%s]\n", str);
    }
    strcpy(strTree, str);
    free(str);
    printf(strTree);
    return '\0';
}

// Print page-header data into index file
int writePageData(Node *new)
{
    int pos=0, message;
    char pageData[PAGE_SIZE];
    if ((message = pinPage(metadata->bf, metadata->pg, new->nodeNumber)) != RC_OK)
        return message;

    memset(pageData, 0, PAGE_SIZE);

    char samp[30];
    sprintf(samp,"%d",new->nodeNumber);
    pageData[pos]=*samp;
    pos+=sizeof(int);

    sprintf(samp,"%d",new->isLeafNode);
    pageData[pos]=*samp;
    pos+=sizeof(int);

    sprintf(samp,"%d",new->numKeys);
    pageData[pos]=*samp;
    pos+=sizeof(int);

    for (int i=0; i<new->numKeys; i++)
    {
        sprintf(samp,"%d",new->keys[i]);

        for(int j=0; j<strlen(samp); j++)
            pageData[pos+j] = samp[j];

        pos+=sizeof(int);

    }

    memcpy(metadata->pg->data, pageData, sizeof(int*) * 5);

    if ((message = markDirty(metadata->bf, metadata->pg)) != RC_OK)
        return message;
    if ((message = unpinPage(metadata->bf, metadata->pg)) != RC_OK)
        return message;

    return RC_OK;

}

//return the total number of nodes
RC getNumNodes (BTreeHandle *tree, int *result)
{
    BTreeManager_Metadata *treeData = (BTreeManager_Metadata *) tree->mgmtData;
    *result = treeData->totalNumNodes;
    printf("The total number of nodes in bTree is: %d\n", *result);

    return RC_OK;
}

//return the total number of nodes
RC getNumEntries (BTreeHandle *tree, int *result)
{
    BTreeManager_Metadata *treeData = (BTreeManager_Metadata *) tree->mgmtData;
    *result = treeData->totalNumEntries;
    printf("The total number of entries in bTree is: %d\n", *result);

    return RC_OK;
}

//return key data-type
RC getKeyType (BTreeHandle *tree, DataType *result)
{

    BTreeManager_Metadata *treeData = (BTreeManager_Metadata *) tree->mgmtData;
    *result = treeData->keyType;
    printf("Key type: %s\n", result);
    return RC_OK;
}

// This method initializes our Index Manager.
RC initIndexManager (void *mgmtData)
{
    printf("inside init function\n");
    initStorageManager();
    metadata = malloc(sizeof(BTreeManager_Metadata));
    metadata->bf = MAKE_POOL();
    metadata->pg = MAKE_PAGE_HANDLE();
    return RC_OK;
}

// This function creates a new B+ Tree and stores meta-data in index file.
RC createBtree(char *idxId, DataType keyType, int n)
{
    printf("inside createBtree method\n");
    // Initialize the B+ Tree meta-data structure.
    metadata->entriesPerNode = n ;
    metadata->totalNumNodes = 0;
    metadata->totalNumEntries = 0;
    metadata->rootNode = NULL;
    metadata->nextFreePage= 0;
    metadata->firstLeafNode= 0;
    metadata->keyType = keyType;

    int pos=0;
    char pageData[PAGE_SIZE];

    memset(pageData, 0, PAGE_SIZE);

    char samp[30];
    sprintf(samp,"%d",metadata->entriesPerNode);
    pageData[pos]=*samp;
    pos+=sizeof(int);

    sprintf(samp,"%d",metadata->totalNumEntries);
    pageData[pos]=*samp;
    pos+=sizeof(int);

    sprintf(samp,"%d",metadata->totalNumNodes);
    pageData[pos]=*samp;
    pos+=sizeof(int);

    sprintf(samp,"%d",metadata->rootNode);
    pageData[pos]=*samp;
    pos+=sizeof(int);

    sprintf(samp,"%d",metadata->nextFreePage);
    pageData[pos]=*samp;
    pos+=sizeof(int);

    sprintf(samp,"%d",metadata->firstLeafNode);
    pageData[pos]=*samp;
    pos+=sizeof(int);

    sprintf(samp,"%d",metadata->keyType);
    pageData[pos]=*samp;
    pos+=sizeof(int);

    SM_FileHandle fh;
    int message;

    // Create page file. Return error code if error occurs.
    if ((message = createPageFile(idxId)) != RC_OK)
        return message;

    // Open page file. Return error code if error occurs.
    if ((message = openPageFile(idxId, &fh)) != RC_OK)
        return message;

    // Write B+ tree meta-data into file. Return error code if error occurs.
    if ((message = writeBlock(-1, &fh, pageData)) != RC_OK)
        return message;

    // Close page file.  Return error code if error occurs.
    if ((message = closePageFile(&fh)) != RC_OK)
        return message;

    return RC_OK;
}

// This method opens an existing B+ Tree.
RC openBtree (BTreeHandle **tree, char *idxId)
{
    printf("inside openBtree method\n");
    int message = initBufferPool(metadata->bf, idxId, 4, RS_FIFO, NULL);
    if(message!=RC_OK)
    {
        printf("Error starting buffer pool manager\n");
        return message;
    }
    (*tree) = (BTreeHandle *)malloc(sizeof(BTreeHandle));

    (*tree)->idxId = idxId;
    (*tree)->mgmtData = metadata;
    return RC_OK;
}

// flushes all modified index pages back to disk and closes Btree
/*RC closeBtree (BTreeHandle *tree)
{
    printf("inside closeBtree method\n");
    BTreeManager_Metadata *treeData = (BTreeManager_Metadata*) tree->mgmtData;
    
    Node *node = treeData->rootNode;
 
    for (int i = 0; i < treeData->totalNumNodes; i++)
    {
        int entries = 2*node->numKeys + 1;
        for (int j = 0; j < entries; j++)
        {
            if(j%2 == 0) //even (pointers)
            {
                if(node->pointers[j/2] != NULL)
                {
                    if(j == entries - 1)
                    {
                        Node *freeNode = node;
                        node = node->pointers[(j/2)];
                        free(freeNode);
                    }
                    else
                        free(node->pointers[j/2]);
                }
            }
                   }
        //if(node != NULL)
            //free(node);
    }
    free(tree);
    int statusID = shutdownBufferPool(metadata->bf);
    if(statusID == RC_OK)
        printf("closed Btree.\n");
    else
    {
        printf("Error in closing Btree.\n");
        return statusID;
    }
    
    return RC_OK;
}*/

RC closeBtree (BTreeHandle *tree)
{
    printf("inside closeBtree method\n");
    int statusID = shutdownBufferPool(metadata->bf);
    if(statusID == RC_OK)
        printf("closed Btree.\n");
    else
    {
        printf("Error in closing Btree.\n");
        return statusID;
    }
    free(tree);
    return RC_OK;
}


// deletes the btree index an removes the corresponding page file
RC deleteBtree (char *idxId)
{
    int statusID = destroyPageFile(idxId);
    if(statusID == RC_OK)
        printf("Btree Deleted successfully.\n");
    else
    {
        printf("Error in deleting Btree.\n");
        return statusID;
    }
    return RC_OK;
}

// Frees all the allocated resources
RC shutdownIndexManager()
{
    //shutdownBufferPool(metadata->bf);
    free(metadata->bf);
    free(metadata->pg);
    free(metadata);
    printf("Freed all resources and shutdown Btree.\n");
    return RC_OK;
}

// This method initializes the scan structure which is used to scan the entries in the B+ Tree.
RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle)
{
    printf("inside openTreeScan method\n");
    (*handle) = calloc(1, sizeof(BT_ScanHandle));
    ScanManager *scManager = malloc(sizeof(ScanManager));
    //(*handle)->tree = tree;

    Node *node = metadata->rootNode;

    if (metadata->rootNode == NULL)
    {
        return RC_NOT_OK;
    }
    else
    {
        while (!node->isLeafNode)
        {
            node = node->pointers[0];
        }
        scManager->totalKeysScanned = 0;
        scManager->numKeys = node->numKeys;
        scManager->node = node;
        (*handle)->mgmtData = scManager ;
    }

    return RC_OK;
}

// This function is used to scan the entries in the B+ Tree.
RC nextEntry(BT_ScanHandle *handle, RID *result)
{
    printf("inside nextEntry method\n");
    ScanManager * scManager = (ScanManager *) handle->mgmtData;
    RID *rid;
    Node * node = scManager->node;

    if (node == NULL)
    {
        return RC_IM_NO_MORE_ENTRIES;
    }

    if (scManager->totalKeysScanned < scManager->numKeys)
    {
        rid = ((RID *)node->pointers[scManager->totalKeysScanned]);
        scManager ->totalKeysScanned++;
    }
    else
    {
        if (node->pointers[metadata->entriesPerNode] != NULL)
        {
            node = node->pointers[metadata->entriesPerNode];
            scManager ->totalKeysScanned = 1;
            scManager ->numKeys = node->numKeys;
            scManager ->node = node;
            rid = ((RID *)node->pointers[0]);
        }
        else
        {
            return RC_IM_NO_MORE_ENTRIES;
        }
    }
    *result = *rid;
    printf("scan successful.\n");
    return RC_OK;
}

// This function closes tree scan and frees up allocated space.
RC closeTreeScan (BT_ScanHandle *handle)
{
    printf("inside closeTreeScan method\n");
    free(handle->mgmtData);
    free(handle);
    return RC_OK;
}


/******** Methods implemented for insertKey ********************/
RID *createNewRecord(RID *rid);
Node *createNewTree(BTreeManager_Metadata *metadata, int key, RID *record);
Node *createLeaf(BTreeManager_Metadata *metadata);
Node *createNode(BTreeManager_Metadata *metadata);
Node *findLeaf(Node *root, int key);
Node *insertIntoLeaf(BTreeManager_Metadata *metadata, Node *leaf, int key, RID *record);
Node *insertIntoLeafAfterSplitting(BTreeManager_Metadata *metadata,Node *leaf, int key, RID *record);
Node *insertIntoParent(BTreeManager_Metadata *metadata, Node *left, int key, Node *right);
Node *insertIntoNode(BTreeManager_Metadata *metadata, Node *parent, int left_index, int key, Node *right);
Node *insertIntoNodeAfterSplitting(BTreeManager_Metadata *metadata, Node *parent, int left_index, int key, Node *right);
Node *insertIntoNewRoot(BTreeManager_Metadata *metadata, Node *left, int key, Node *right);
int getLeftIndex(Node *parent, Node *left);

// This method inserts a new record with the specified key and RID.
RC insertKey(BTreeHandle *tree, Value *key, RID rid)
{
    printf("inside insertKey method\n");
    //BTreeManager_Metadata *metadata = (BTreeManager_Metadata *) tree->mgmtData;
    RID *record= NULL;
    Node *leaf;

    // Check if key already exists.
    if (findKey(tree, key, &rid) == RC_OK)
    {
        printf("Key already exists\n");
        return RC_IM_KEY_ALREADY_EXISTS;
    }

    // Create a new record for key.
    record = createNewRecord(&rid);

    // Create a new tree if the tree does not exist.
    if (metadata->rootNode == NULL)
    {
        printf("tree does not exist.creating new tree.\n");
        metadata->rootNode = createNewTree(metadata, key->v.intV, record);
        printf("first key inserted\n");
    }
    else
    {
        // Find a leaf where the key can be inserted.
        leaf = findLeaf(metadata->rootNode, key->v.intV);

        if (leaf->numKeys < metadata->entriesPerNode)
        {
            // Insert the new key into leaf.
            printf("Insert the new key into leaf\n");
            leaf = insertIntoLeaf(metadata, leaf, key->v.intV, record);
        }
        else
        {
            // Split leaf and then insert key.
            printf("splitting leaf\n");
            metadata->rootNode = insertIntoLeafAfterSplitting(metadata, leaf, key->v.intV, record);
        }
    }
    //free(record);
    printf("total number of keys in B+ tree:%d\n",metadata->totalNumEntries);
    printf("total number of nodes in B+ tree:%d\n",metadata->totalNumNodes);
    printf("insertKey successful.......................................\n");

    return RC_OK;
}

// Creates a new record for key.
RID *createNewRecord(RID *rid)
{
    printf("Inside createNewRecord method\n");
    RID *record = (RID *) malloc(sizeof(RID));
    if (record == NULL)
    {
        printf("Record not created\n");
    }
    else
    {
        record->page = rid->page;
        record->slot = rid->slot;
    }
    printf("createNewRecord complete\n");
    return record;
}

// Creates a new tree.
Node *createNewTree(BTreeManager_Metadata *metadata, int key, RID *record)
{
    printf("Inside createNewTree method\n");
    Node *root = createLeaf(metadata);
    root->keys[0] = key;
    root->pointers[0] = record;
    root->pointers[metadata->entriesPerNode] = NULL;
    root->parent = NULL;
    root->next = NULL;
    root->numKeys++;
    metadata->totalNumEntries++;
    printf("createNewTree complete\n");
    return root;
}

// Creates a new leaf by creating a node.
Node *createLeaf(BTreeManager_Metadata *metadata)
{
    printf("Inside createLeaf method\n");
    Node *leaf = createNode(metadata);
    leaf->isLeafNode = true;
    printf("createLeaf complete\n");
    return leaf;
}

// Creates a new node(leaf or non-leaf node).
Node *createNode(BTreeManager_Metadata * metadata)
{
    printf("Inside createNode method\n");
    metadata->totalNumNodes++;

    Node *new_node = malloc(sizeof(Node));
    if (new_node == NULL)
    {
        printf("new node allocation failed\n");
    }
    new_node->keys = malloc((metadata->entriesPerNode) * sizeof(int));
    if (new_node->keys == NULL)
    {
        printf("New node keys allocation failed\n.");
    }

    new_node->pointers = malloc((metadata->entriesPerNode+1)* sizeof(void *));
    if (new_node->pointers == NULL)
    {
        printf("New node pointers allocation failed\n.");
    }
    new_node->isLeafNode = false;
    new_node->numKeys = 0;
    new_node->parent = NULL;
    new_node->next = NULL;

    printf("createNode complete\n");
    return new_node;
}

// find leaf node where key can be added.
Node *findLeaf(Node *root, int key)
{
    printf("Inside findLeaf method\n");
    int i = 0;
    Node *new_node = root;
    if (new_node == NULL)
    {
        printf("tree not available.\n");
        return new_node;
    }
    while (!new_node->isLeafNode)
    {
        i = 0;
        while (i < new_node->numKeys)
        {
            if (key >= new_node->keys[i])
            {
                i++;
            }
            else
                break;
        }
        new_node = (Node *)new_node->pointers[i];
    }
    return new_node;
}

// Insert into leaf when leaf has space.
Node *insertIntoLeaf(BTreeManager_Metadata *metadata, Node *leaf, int key, RID *record)
{
    printf("inside insertIntoLeaf method\n");
    metadata->totalNumEntries++;

    int pos = 0;
    while (pos < leaf->numKeys && leaf->keys[pos] < key)
        pos++;

    for (int i = leaf->numKeys; i > pos; i--)
    {
        leaf->keys[i] = leaf->keys[i - 1];
        leaf->pointers[i] = leaf->pointers[i - 1];
    }
    leaf->keys[pos] = key;
    leaf->pointers[pos] = record;
    leaf->numKeys++;
    leaf->nodeNumber = (metadata->totalNumEntries%2 != 0) ? (metadata->totalNumEntries/2) +1 : metadata->totalNumEntries/2;
    printf("inserting into leaf done\n");
    return leaf;        // return updated leaf
}

// Split the leaf into two and insert a key into the new leaf node.
Node *insertIntoLeafAfterSplitting(BTreeManager_Metadata *metadata,Node *leaf, int key, RID *record)
{
    printf("inside insertIntoLeafAfterSplitting method.\n");
    Node *new_leaf;
    int *temp_keys;
    void **temp_pointers;
    int pos, split, new_key;
    int i, j;

    new_leaf = createLeaf(metadata);

    temp_keys = malloc((metadata->entriesPerNode+1)*sizeof(int));
    if (temp_keys == NULL)
    {
        printf("keys allocation failed.\n");
    }

    temp_pointers = malloc((metadata->entriesPerNode+1)* sizeof(void *));
    if (temp_pointers == NULL)
    {
        printf("pointers allocation failed.\n");
    }

    pos = 0;
    while (pos < metadata->entriesPerNode && leaf->keys[pos] < key)
        pos++;

    for (i = 0, j = 0; i < leaf->numKeys; i++, j++)
    {
        if (j == pos)
            j++;
        temp_keys[j] = leaf->keys[i];
        temp_pointers[j] = leaf->pointers[i];
    }

    temp_keys[pos] = key;
    temp_pointers[pos] = record;

    leaf->numKeys = 0;

    if ((metadata->entriesPerNode) % 2 == 0)
    {
        split = (metadata->entriesPerNode) / 2+1;
    }
    else
    {
        split = (metadata->entriesPerNode) / 2;
    }

    for (i = 0; i < split; i++)
    {
        leaf->pointers[i] = temp_pointers[i];
        leaf->keys[i] = temp_keys[i];
        leaf->numKeys++;
    }

    for (i = split, j = 0; i < metadata->entriesPerNode+1; i++, j++)
    {
        new_leaf->pointers[j] = temp_pointers[i];
        new_leaf->keys[j] = temp_keys[i];
        new_leaf->numKeys++;
    }

    free(temp_pointers);
    free(temp_keys);

    new_leaf->pointers[metadata->entriesPerNode] = leaf->pointers[metadata->entriesPerNode];
    leaf->pointers[metadata->entriesPerNode] = new_leaf;


    for (i = leaf->numKeys; i < metadata->entriesPerNode ; i++)
        leaf->pointers[i] = NULL;
    for (i = new_leaf->numKeys; i <  metadata->entriesPerNode; i++)
        new_leaf->pointers[i] = NULL;

    new_leaf->parent = leaf->parent;
    new_key = new_leaf->keys[0];
    metadata->totalNumEntries++;

    new_leaf->nodeNumber = (metadata->totalNumEntries%2 != 0) ? (metadata->totalNumEntries/2) +1 : metadata->totalNumEntries/2;
    if(leaf->pointers[metadata->entriesPerNode] != NULL)
        leaf->nextPageNumber = new_leaf->nodeNumber;


    return insertIntoParent(metadata, leaf, new_key, new_leaf);
}

// Insert a new node (leaf or non-leaf node) into the B+ tree and returns the root of the tree after insertion.
Node *insertIntoParent(BTreeManager_Metadata *metadata, Node *left, int key, Node *right)
{
    printf("Inside insertIntoParent method\n");
    int left_index;
    Node *parent = left->parent;

    // Checking if it is the new root.
    if (parent == NULL)
        return insertIntoNewRoot(metadata, left, key, right);

    // In case its a leaf or node, find the parent's pointer to the left node.
    left_index = getLeftIndex(parent, left);

    // If the new key can accommodate in the node.
    if (parent->numKeys < metadata->entriesPerNode)
    {
        //insert into node
        return insertIntoNode(metadata, parent, left_index, key, right);
    }
    // else split the node
    return insertIntoNodeAfterSplitting(metadata, parent, left_index, key, right);
}

// Creates a new root and inserts the key into the new root.
Node *insertIntoNewRoot(BTreeManager_Metadata *metadata, Node *left, int key, Node *right)
{
    printf("inside insertIntoNewRoot method\n");
    Node *root = createNode(metadata);
    root->keys[0] = key;
    root->pointers[0] = left;
    root->pointers[1] = right;
    root->nodeNumber = 0;
    root->numKeys++;
    root->parent = NULL;
    left->parent = root;
    right->parent = root;
    return root;
}

// Find index of the parent's pointer to the node to the left of the key to be inserted.
int getLeftIndex(Node *parent, Node *left)
{
    printf("inside getLeftIndex method.\n");
    int left_index = 0;
    while (left_index <= parent->numKeys && parent->pointers[left_index] != left)
        left_index++;
    return left_index;
}

// Inserts a new key and pointer to a node into node.
Node * insertIntoNode(BTreeManager_Metadata *metadata, Node *parent, int left_index, int key, Node *right)
{
    printf("inside insertIntoNode method\n");
    int i;
    for (i = parent->numKeys; i > left_index; i--)
    {
        parent->pointers[i + 1] = parent->pointers[i];
        parent->keys[i] = parent->keys[i - 1];
    }

    parent->pointers[left_index + 1] = right;
    parent->keys[left_index] = key;
    parent->numKeys++;

    return metadata->rootNode;
}

// Split node insert a new key and pointer to a node into a node.
Node * insertIntoNodeAfterSplitting(BTreeManager_Metadata *metadata, Node *old_node, int left_index, int key, Node *right)
{
    printf("inside insertIntoNodeAfterSplitting method\n");
    int i, j;
    int split, new_key;
    Node *new_node, *child;
    int *temp_keys;
    Node **temp_pointers;

    temp_pointers = malloc((metadata->entriesPerNode + 2) * sizeof(Node *));
    if (temp_pointers == NULL)
    {
        printf("pointers allocation failed.\n\n");
    }
    temp_keys = malloc((metadata->entriesPerNode+1) * sizeof(int));
    if (temp_keys == NULL)
    {
        printf("keys allocation failed.\n");
    }

    for (i = 0, j = 0; i < old_node->numKeys + 1; i++, j++)
    {
        if (j == left_index + 1)
            j++;
        temp_pointers[j] = old_node->pointers[i];
    }

    for (i = 0, j = 0; i < old_node->numKeys; i++, j++)
    {
        if (j == left_index)
            j++;
        temp_keys[j] = old_node->keys[i];
    }

    temp_pointers[left_index + 1] = right;
    temp_keys[left_index] = key;

    // Create the new node and copy half the keys to the old and half to the new.
    if ((metadata->entriesPerNode) % 2 == 0)
        split = (metadata->entriesPerNode) / 2+1;
    else
        split = (metadata->entriesPerNode) / 2;

    new_node = createNode(metadata);
    old_node->numKeys = 0;
    for (i = 0; i < split; i++)
    {
        old_node->pointers[i] = temp_pointers[i];
        old_node->keys[i] = temp_keys[i];
        old_node->numKeys++;
    }
    old_node->pointers[i] = temp_pointers[i];
    new_key = temp_keys[split - 1];
    for (++i, j = 0; i < metadata->entriesPerNode+1; i++, j++)
    {
        new_node->pointers[j] = temp_pointers[i];
        new_node->keys[j] = temp_keys[i];
        new_node->numKeys++;
    }
    new_node->pointers[j] = temp_pointers[i];

    free(temp_pointers);
    free(temp_keys);

    new_node->parent = old_node->parent;

    for (i = 0; i < new_node->numKeys; i++)
    {
        child = new_node->pointers[i];
        child->parent = new_node;
    }
    return insertIntoParent(metadata, old_node, new_key, new_node);
}


//Get a key from user program to be deleted from the B+tree
RC deleteKey (BTreeHandle *tree, Value *key){
    RID *resRid = malloc(sizeof(RID));

    bool isLeftSib=0;
    bool isRightSib=0;
   
    int deleteCase; 
    Node *resNode;
    Node *leafNode;
    Node *siblingNode;
    Node *rightSiblingNode;
    int i = 0, minKeys;
    
    resNode = metadata->rootNode;
    printf("inside delete method\n");

    if (key == NULL){
        printf("Key not found\n");
        return RC_NOT_OK;
    }

    //searchNodes(resNode, key->v.intV, resRid);
    if (findKey(tree, key, resRid) != RC_OK){
        printf("inside delete key - key not found\n");
        return RC_NOT_OK;
    }
    

    leafNode = resNode->pointers[curPtrIndex];

    if(!leafNode){
        printf("Node is empty");
            return RC_NOT_OK;

    }
   
    //find the min number of keys per node
    minKeys = minKeysPerNode(metadata, leafNode);

    printf("*****Inside delete key - checking delete methode*****\n");
    printTree(tree);


    //First heck if key can be deleted without causing node underflow - simple case
    
    if ((leafNode->numKeys-1) >= minKeys){
        printf("*****Inside delete key - befor delete - simple case*****\n");

    
        //move all keys one seat to the left, starting from the deleted key index
        printf("Ptr: %d and Key index number: %d\n", curPtrIndex, curKeyIndex);

        //Check the index of the key to be deleted, align all keys and pointer in the node accordingly
        if (curKeyIndex + 1 == leafNode->keys[leafNode->numKeys]){
            leafNode->keys[curKeyIndex] = leafNode->keys[curKeyIndex+1];
            leafNode->pointers[curKeyIndex] = leafNode->pointers[curKeyIndex+1];
        }

        else{
            i = curKeyIndex;
            while (i < metadata->entriesPerNode - curKeyIndex){
                leafNode->keys[i] = leafNode->keys[i+1];
                leafNode->pointers[i] = leafNode->pointers[i+1];
                i++;

        }
        //copy the pointer to sibling
        leafNode->pointers[i] = leafNode->pointers[i+1];

        }
        
        printf("i value %d\n",i);
        
        metadata->totalNumEntries--;
        leafNode->numKeys = leafNode->numKeys -1;
        printf("*****Inside delete key - after delete*****\n");
        printTree(tree);
        printf("key %d in location: %d and key index number: %d is deleted. Current number of keys: %d and min # of keys: %d\n", key->v.intV, curPtrIndex, curKeyIndex, leafNode->numKeys, minKeys );
        return RC_OK;
    }

    printf("Ptr: %d and Key index number: %d\n", curPtrIndex, curKeyIndex);
    
    //Ensure current node is not the last / first, than get its siblings
    if (curPtrIndex != 0){ 
        siblingNode = resNode->pointers[curPtrIndex-1];
        printf("Left sib1\n");
        isLeftSib=1;
    }
    if (curPtrIndex < resNode->numKeys){
        rightSiblingNode = resNode->pointers[curPtrIndex+1];
        printf("Right sib1\n");
        isRightSib=1;
    }
    

    //Delete method is based on a few assumptions: 
    //1. At least one sibling exists in a tree
    //2. For both merge and keys restribute, we'll prefer tje left sibling. If doesn't exists or has min # of keys, then we'll go to the right sibling
    
    if (!isLeftSib){ //No left sib
        if(rightSiblingNode->numKeys-1 >= minKeys && leafNode->numKeys > minKeys){ //right sib more than minimume number of keys
            deleteCase = 2;
            printf("Case 2 - re-dist. key with the right\n");
        }
        else{ // right sibling has less than minimum # of keys -> merge 
            deleteCase = 4;
            printf("Case 4 - right merge\n");
        }
    }
    else if(!isRightSib){ //no right sib
        if(siblingNode->numKeys-1 >= minKeys && leafNode->numKeys > minKeys){ //left sib has more than minimume number of keys
                deleteCase = 1;
                printf("Case 1 - re-dist. key with the left\n");
            }
        else{ // right sibling has less than minimum # of keys -> merge 
            deleteCase = 3;
            printf("Case 3 - left merge\n");
        }
    }
    else{
        if(siblingNode->numKeys-1 >= minKeys){
            deleteCase = 1;
            printf("Else - Case 1 - re-dist. key with the left\n");

        }
            
        else if(rightSiblingNode->numKeys-1 >= minKeys){
            deleteCase = 2;
            printf("Else - Case 2 - re-dist. key with the right\n");
        }
            
        else{
            deleteCase = 3;
            printf("Else - Case 3 - left merge\n");

        } 
            

    }
        
    printf("Num of keys leafNode: %d, num of keys in left sib %d and right: %d\n", leafNode->numKeys, siblingNode->numKeys, rightSiblingNode->numKeys);

    

    // TA's comment: the following codes are not organized and lack comments. 
    // I'm afraid I don't have the time to find the issue that causes test failure.
    switch (deleteCase)
    {
    case 1:
                printf("*****Inside delete key - befor delete - key re-distribution - checking left sib*****\n");
                if(curKeyIndex == 0){
                    printf("*****Inside delete key - befor delete - key re-distribution - checking left sib - curKeyIndex = 0 case*****\n");

                    leafNode->keys[curKeyIndex] = siblingNode->keys[siblingNode->numKeys-1];
                    leafNode->pointers[curKeyIndex] = siblingNode->pointers[siblingNode->numKeys-1];
                    siblingNode->pointers[siblingNode->numKeys-1] = siblingNode->pointers[siblingNode->numKeys];
                    siblingNode->keys[siblingNode->numKeys-1] = 0;
                    resNode->keys[0] = leafNode->keys[0];
                    siblingNode->numKeys = siblingNode->numKeys - 1;
                    metadata->totalNumEntries--;
                    if(curPtrIndex == 1 && resNode->keys[0] < leafNode->keys[0] )
                        resNode->keys[1] = leafNode->keys[0];
                    else if(curPtrIndex == 2)
                        resNode->keys[1] = leafNode->keys[0];
                    else    
                        resNode->keys[0] = leafNode->keys[0];
                    

    
                    printf("*****Inside delete key - after delete*****\n");
                    printTree(tree);
                    printf("key %d in location: %d and key index number: %d is deleted\n", key->v.intV, curPtrIndex, curKeyIndex);
                }

                else{
                    printf("*****Inside delete key - befor delete - key re-distribution - checking left sib - curKeyIndex != 0 case*****\n");
                    
                    leafNode->keys[curKeyIndex] = siblingNode->keys[siblingNode->numKeys-1];
                    for (i = curKeyIndex; i < 0 ; i--){
                        leafNode->keys[curKeyIndex-i] = leafNode->keys[curKeyIndex-i-1];
                        leafNode->pointers[curKeyIndex-i] = leafNode->pointers[curKeyIndex-i-1];
                    }
                    leafNode->keys[curKeyIndex] = siblingNode->keys[siblingNode->numKeys-1];
                     
                    leafNode->pointers[curPtrIndex] = resRid;
                    siblingNode->pointers[siblingNode->numKeys-1] = NULL;
                    siblingNode->numKeys = siblingNode->numKeys - 1;

                    if(curPtrIndex == 1 && resNode->keys[0] < leafNode->keys[0] )
                        resNode->keys[1] = leafNode->keys[0];
                    else if(curPtrIndex == 2)
                        resNode->keys[1] = leafNode->keys[0];
                    else    
                        resNode->keys[0] = leafNode->keys[0];
                    
                    metadata->rootNode = resNode;
                    printf("*****Inside delete key - after delete*****\n");
                    printTree(tree);
                    printf("key %d in location: %d and key index number: %d is deleted\n", key->v.intV, curPtrIndex, curKeyIndex);
                }
        
        break;

    case 2:
        printf("*****Inside delete key - befor delete - key re-distribution - checking right sib*****\n");
                    leafNode->keys[leafNode->numKeys-1] = rightSiblingNode->keys[0];
                    leafNode->pointers[leafNode->numKeys-1] = rightSiblingNode->pointers[0];
                    rightSiblingNode->pointers[0] = rightSiblingNode->pointers[1];
                    rightSiblingNode->pointers[1] = rightSiblingNode->pointers[2];
                    rightSiblingNode->keys[0] = rightSiblingNode->keys[1];

                    
                    rightSiblingNode->keys[rightSiblingNode->numKeys-1] = 0;
                    rightSiblingNode->numKeys = rightSiblingNode->numKeys -1;
                    metadata->totalNumEntries--;

                    
                    if(curPtrIndex == 1 && resNode->keys[0] < leafNode->keys[0] )
                        resNode->keys[1] = rightSiblingNode->keys[0];
                    else if(curPtrIndex == 2)
                        resNode->keys[1] = rightSiblingNode->keys[0];
                    else    
                        resNode->keys[0] = rightSiblingNode->keys[0];


               
                    printf("*****Inside delete key - after delete*****\n");
                    printTree(tree);
                    printf("key %d in location: %d and key index number: %d is deleted\n", key->v.intV, curPtrIndex, curKeyIndex);

        break;
    case 3:
        printf("*****Inside delete key - befor delete - key merge - checking left sib*****\n");
               
                    resNode->keys[resNode->numKeys-1] = 0;
                    for (i = curPtrIndex; i < -1; i--){
                        resNode->pointers[curKeyIndex-i] = resNode->pointers[curKeyIndex-i-1];
                        i++;
                    }
                    resNode->numKeys = resNode->numKeys-1;
                    metadata->totalNumEntries --;
                    metadata->totalNumNodes --;

                    printf("*****Inside delete key - after delete*****\n");
                    printTree(tree);
                    printf("key %d in location: %d and key index number: %d is deleted\n", key->v.intV, curPtrIndex, curKeyIndex);
        break;

    case 4:
    printf("*****Inside delete key - befor delete - key merge - checking right sib*****\n");
        
        leafNode->keys[curKeyIndex] = 0;
        for (i = curPtrIndex; i<resNode->numKeys+1; i++){
            resNode->pointers[curKeyIndex+i] = resNode->pointers[curKeyIndex+i+1];
            i++;
        }
        resNode->keys[0] = resNode->keys[1];   
        resNode->numKeys = resNode->numKeys-1;
        metadata->totalNumEntries --;
        metadata->totalNumNodes --;

    printf("After delete:\n");
    printTree(tree);
        break;
    
    default:
        printf("Default case, error\n");
        return RC_NOT_OK;
    }
            
 
    free(resRid);
    return RC_OK;

}
int minKeysPerNode(BTreeManager_Metadata *metadata, Node *node)
{

    int min;
    if (node == NULL)
    {
        printf("Node is empty\n");
        return -1;

    }
    //the result is an odd number
    if (((metadata->entriesPerNode+1)%2) != 0)
    {
        min = (metadata->entriesPerNode+1)/2;
        return min;

    }

    if(node->isLeafNode)
        return min;

    else
        return min-1;
}
