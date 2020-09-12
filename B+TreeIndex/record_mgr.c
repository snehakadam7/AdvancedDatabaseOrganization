#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

// Define a structure
typedef struct Rec_Mgmt_Data
{
    int freePageNumber;
    int recordCount;
    int numAttr;
    int keySize;
    char attrName[5];
    BM_PageHandle *pg;
    BM_BufferPool *bf;
} Rec_Mgmt_Data;

//scanManager metadata
typedef struct RM_ScanManager
{
    Record rec; //rec id
    Expr *expr; //save condition
    int totalScaned;
    int totalTuples;

} RM_ScanManager;

Schema *new_schema;
SM_FileHandle fh;
Rec_Mgmt_Data *mgmt_data;
RM_ScanManager *scManager;

//This function initialize record manager.
RC initRecordManager(void *mgmtData)
{
    //printf("inside initRecordManager method.\n");
    initStorageManager();
    mgmt_data = (Rec_Mgmt_Data *)malloc(sizeof(Rec_Mgmt_Data));
    mgmt_data->bf = MAKE_POOL();
    mgmt_data->pg = MAKE_PAGE_HANDLE();

    return RC_OK;
}

// This function creates a table and stores schema at the beginning of the table.
RC createTable (char *name, Schema *schema)
{
    //printf("inside createTable method.\n");
    int i,j;
    int pos=0;

    char *Page;
    char pageData[PAGE_SIZE];

    memset(pageData, 0, PAGE_SIZE);

    //We will store schema in header page of the table.
    //store the first free page number of the table.
    pageData[pos]='0';
    pos+=sizeof(int);
    mgmt_data->freePageNumber = 0;

    //store the intial number of records in the table.
    pageData[pos]='0';
    pos+=sizeof(int);
    mgmt_data->recordCount=0;

    //store all the attributes of the schema to the table.
    char samp[30];
    sprintf(samp,"%d",schema->numAttr);
    pageData[pos]=*samp;
    pos+=sizeof(int);
    mgmt_data->numAttr=schema->numAttr;

    for(i=0; i<schema->numAttr; i++)
    {
        sprintf(samp,"%d",schema->typeLength[i]);
        pageData[pos]=*samp;
        pos+=sizeof(int);
    }

    for(i=0; i<schema->numAttr; i++)
    {
        sprintf(samp,"%d",schema->dataTypes[i]);
        pageData[pos]=*samp;
        pos+=sizeof(int);
    }

    sprintf(samp,"%d",schema->keySize);
    pageData[pos]=*samp;
    pos+=sizeof(int);
    mgmt_data->keySize=schema->keySize;

    for(i=0; i<1; i++)
    {
        sprintf(samp,"%d",schema->keyAttrs[i]);
        pageData[pos]=*samp;
        pos+=sizeof(int);
    }

    for(i=0; i<3; i++)
    {
        for(j=0; j<strlen(schema->attrNames[i]); j++)
            pageData[pos] = schema->attrNames[i][j];
        char *samp = &mgmt_data->attrName[i];
        strncpy(samp,schema->attrNames[i],1);
        pos+=5;
    }

    pageData[pos]='\0';

    SM_FileHandle fh;
    int message;
    message = createPageFile(name);             // Create file
    if(message != RC_OK)
        return message;

    message = openPageFile(name, &fh);  // Open File
    if(message != RC_OK)
        return message;

    message=writeBlock(-1, &fh, pageData); // we are considering header page as -1.
    if(message != RC_OK)
        return message;

    message=closePageFile(&fh);                 // Close File
    if(message != RC_OK)
        return message;

    return RC_OK;
}

//Creating schema
Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
    //printf("inside createSchema method.\n");
    new_schema=(Schema *)malloc(sizeof(Schema));

    new_schema->numAttr=numAttr;
    new_schema->attrNames=attrNames;
    new_schema->dataTypes=dataTypes;
    new_schema->typeLength=typeLength;
    new_schema->keySize=keySize;
    new_schema->keyAttrs=keys;
    return new_schema;
}

// This function deallocates all the memory space allocated to the schema.
RC freeSchema (Schema *schema)
{
    //printf("inside freeSchema method.\n");
    free(schema->attrNames);
    free(schema->dataTypes);
    free(schema->keyAttrs);
    free(schema->typeLength);
    free(schema);
    return RC_OK;
}

//This function stores metadata from header page of disk to RM_TableData structure in memory.
RC openTable (RM_TableData *rel, char *name)
{
    //printf("inside openTable method.\n");
    int message =  initBufferPool(mgmt_data->bf, name, 1, RS_FIFO, NULL);
    if (message != RC_OK)
        return message;

    rel->name = name;
    rel->schema = new_schema;
    rel->mgmtData = mgmt_data;
    return RC_OK;
}

//This function close the table by calling shutdownBufferPool method to write all the changes to the disk.
RC closeTable (RM_TableData *rel)
{
    //printf("inside closeTable method.\n");
    shutdownBufferPool(mgmt_data->bf);
    rel->mgmtData = NULL;

    return RC_OK;
}

// This function shuts down the Record Manager
RC shutdownRecordManager ()
{
    //printf("inside shutdownRecordManager method.\n");
    freeSchema(new_schema);
    free(mgmt_data->pg);
    free(mgmt_data->bf);
    free(mgmt_data);
    return RC_OK;
}

// This function creates a new record.
RC createRecord (Record **record, Schema *schema)
{
    (*record) = calloc(1, sizeof(Record));
    (*record)->data = (char *)calloc(1, getRecordSize(schema));

    return RC_OK;
}

// This function sets the attribute value in the record.
RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{
    Record *new_record = calloc(1, sizeof(Record));
    char *res = record->data + getOffset(schema, attrNum);

    switch(value->dt)
    {
    case DT_INT:
        new_record->data = calloc(1, sizeof(char*));
        sprintf(new_record->data, "%d", value->v.intV);
        memcpy(res, new_record->data, sizeof(int));
        free(new_record->data);
        break;

    case DT_FLOAT:
        new_record->data = calloc(1, sizeof(float));
        sprintf(new_record->data, "%f,", value->v.floatV);
        memcpy(res, new_record->data, sizeof(float *));
        free(new_record->data);
        break;

    case DT_BOOL:
        new_record->data = calloc(1, sizeof(bool));
        sprintf(new_record->data, "%s", value->v.boolV ? "true" : "false");
        memcpy(res, new_record->data, sizeof(bool *));
        free(new_record->data);
        break;

    case DT_STRING:
        new_record->data = calloc(1, (schema->typeLength[attrNum])) ;
        sprintf(new_record->data, "%s", value->v.stringV);
        memcpy(res, new_record->data, (schema->typeLength[attrNum]));
        free(new_record->data);
        break;
    }
    free(new_record);
    return RC_OK;
}


// This function gets the current offset for the attribute to get/set the attribute.
int getOffset(Schema *schema, int attrNum)
{
    int offset = 0;
    for (size_t i = 0; i < attrNum; i++)
    {
        switch(schema->dataTypes[i])
        {
        case DT_INT:
            offset = offset + sizeof(int);
            break;
        case DT_FLOAT:
            offset = offset + sizeof(float);
            break;
        case DT_BOOL:
            offset = offset + sizeof(bool);
            break;
        case DT_STRING:
            offset = offset + schema->typeLength[i];
            break;
        }
    }
    return offset;
}

// This function retrieves an attribute from the given record
RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)
{
    char *str;
    char *add = record->data+getOffset(schema, attrNum);

    *value = calloc(1,sizeof(Value));

    switch(schema->dataTypes[attrNum])
    {
    case DT_INT:
        (*value)->dt = DT_INT;
        str = calloc(1,sizeof(int));
        memcpy(str, add, sizeof(int));
        (*value)->v.intV = atoi(str);
        free(str);
        break;

    case DT_FLOAT:
        (*value)->dt = DT_FLOAT;
        str = (char *)calloc(1,sizeof(float));
        memcpy(str, add, sizeof(float));
        (*value)->v.floatV = atof(str);
        free(str);
        break;

    case DT_BOOL:
        (*value)->dt = DT_BOOL;
        str = (char *)calloc(1,sizeof(bool));
        memcpy(str, add, sizeof(bool));
        (*value)->v.boolV = (str == "true") ? 1 : 0;
        free(str);
        break;

    case DT_STRING:
        (*value)->dt = DT_STRING;
        (*value)->v.stringV = (char *)calloc(1,(schema->typeLength[attrNum]) + 1);
        memcpy((*value)->v.stringV, add, (schema->typeLength[attrNum]));
        (*value)->v.stringV[schema->typeLength[attrNum]] = '\0';
        break;
    }
    return RC_OK;
}


// This function returns the record size of the schema.
int getRecordSize (Schema *schema)
{
    //printf("inside getRecordSize method.\n");
    int size = 0;

    // Iterate through all the attributes in the schema.
    for(int i = 0; i < schema->numAttr; i++)
    {
        if(schema->dataTypes[i] == DT_INT)
            size = size + sizeof(int);
        else if(schema->dataTypes[i] == DT_STRING)
            size = size + schema->typeLength[i];
        else if(schema->dataTypes[i] == DT_FLOAT)
            size = size + sizeof(float);
        else if(schema->dataTypes[i] == DT_BOOL)
            size = size + sizeof(bool);
    }
    //printf("record size is:%d\n",size);
    return size;
}

//This function deletes the table.
RC deleteTable(char *name)
{
    //printf("inside deleteTable method.\n");
    int message = destroyPageFile(name);    // Destroy page file from memory.
    if(message != RC_OK)
    {
        printf("deleteTable method failed.\n");
        return message;
    }
    return RC_OK;
}

// This function inserts a record into the table taking data from the record structure.
RC insertRecord (RM_TableData *rel, Record *record)
{
    //printf("inside insertRecord\n");
    struct RID rid;
    Schema *schema = rel->schema;
    Rec_Mgmt_Data *recMgmtData = (Rec_Mgmt_Data *) rel->mgmtData;
    int totalRecords = getNumTuples(rel);
    int recordSize = getRecordSize(schema);
    int recsPerPage = totalRecords - ((PAGE_SIZE/recordSize)*(recMgmtData->freePageNumber));
    int freeSpace = PAGE_SIZE - (recsPerPage*recordSize); //total used space in current page

    // Creating rid for the record
    if(freeSpace >= recordSize)
    {
        rid.slot = recsPerPage;
    }
    else
    {
        recMgmtData->freePageNumber = recMgmtData->freePageNumber +1;
        rid.slot = 0;
    }
    rid.page = recMgmtData->freePageNumber;
    record->id = rid;

    int message = pinPage(recMgmtData->bf, recMgmtData->pg, rid.page); // getting the page into memory
    if (message != RC_OK)
        return message;

    char *res = recMgmtData->pg->data + (rid.slot*recordSize);
    memcpy(res, record->data, recordSize);
    message = markDirty(recMgmtData->bf, recMgmtData->pg); // marking page as dirty
    if (message != RC_OK)
        return message;

    recMgmtData->recordCount = recMgmtData->recordCount + 1;
    message = unpinPage(recMgmtData->bf, recMgmtData->pg);
    if (message != RC_OK)
        return message;

    return RC_OK;
}

// This function returns the number of tuples in the table.
int getNumTuples (RM_TableData *rel)
{
    if(rel->mgmtData == NULL)
        return RC_OBJECT_NULL;

    int tuplesCount;
    Rec_Mgmt_Data *recMgmtData = (Rec_Mgmt_Data *) rel->mgmtData;
    tuplesCount = recMgmtData->recordCount;

    return tuplesCount;
}

// This function retrieves the record from the memory.
RC getRecord (RM_TableData *rel, RID id, Record *record)
{
    printf("inside getRecord method.\n");
    int record_size=getRecordSize(rel->schema);
    Rec_Mgmt_Data *recMgmtData = (Rec_Mgmt_Data *) rel->mgmtData;
    record->id = id;
    int recordSize = getRecordSize(rel->schema);

    pinPage(recMgmtData->bf, recMgmtData->pg, id.page);

    int offset = id.slot*recordSize;
    char *add = recMgmtData->pg->data + offset;
    memcpy(record->data, add, recordSize);

    unpinPage(recMgmtData->bf,recMgmtData->pg);
    return RC_OK;
}

// This function deallocates memory space allocated to record and free up that space.
RC freeRecord (Record *record)
{
    if (record->data == NULL)
        return RC_NOT_OK;
    free(record->data);
    free(record);

    return RC_OK;
}

// This function deletes record from table having using record id.
RC deleteRecord(RM_TableData *rel, RID id)
{
    //printf("inside deleteRecord method.\n");
    Rec_Mgmt_Data *recMgmtData = (Rec_Mgmt_Data *) rel->mgmtData;
    pinPage(recMgmtData->bf,recMgmtData->pg,id.page);

    char *page_data=recMgmtData->pg->data;
    int record_size=getRecordSize(rel->schema);

    page_data= page_data + id.slot*record_size;
    for(int i=0; i<record_size; i++)
    {
        page_data[i]='*';   //'*' denotes that the record is deleted.
    }
    markDirty(recMgmtData->bf,recMgmtData->pg);
    unpinPage(recMgmtData->bf,recMgmtData->pg);
    forcePage(recMgmtData->bf,recMgmtData->pg);
    return RC_OK;
}

// This function updates a record using record id in the table.
RC updateRecord (RM_TableData *rel, Record *record)
{
    //printf("inside updateRecord method.\n");
    char *page_data;
    Rec_Mgmt_Data *recMgmtData = (Rec_Mgmt_Data *) rel->mgmtData;
    int record_size=getRecordSize(rel->schema);

    pinPage(recMgmtData->bf,recMgmtData->pg,record->id.page);
    page_data=recMgmtData->pg->data + record->id.slot*record_size;

    memcpy(page_data,record->data,record_size);

    markDirty(recMgmtData->bf,recMgmtData->pg);
    unpinPage(recMgmtData->bf,recMgmtData->pg);
    forcePage(recMgmtData->bf,recMgmtData->pg);
    return RC_OK;
}


// This function initializes the scan.
RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
    scManager = malloc(sizeof(RM_ScanManager));
    scManager->expr = malloc(sizeof(Expr));

    if (cond == NULL)
    {
        return RC_NOT_OK;
    }
    //save metadata to RM_ScanManager
    scan->rel = rel;
    scManager->rec.id.slot = 0;
    scManager->rec.id.page = 0;
    scManager->expr = cond;
    scManager->totalScaned = 0;
    scan->mgmtData = scManager;
    scManager->totalTuples = getNumTuples(scan->rel);

    return RC_OK;
}

// This function scans each record in the table and retrieves the record satisfying the condition.
RC next (RM_ScanHandle *scan, Record *record)
{


    scManager = scan -> mgmtData;
    Value *value;

    if (scManager->totalTuples == 0) //check if the page in emoty
        return RC_RM_NO_MORE_TUPLES;

    if (scManager->totalScaned < scManager->totalTuples)  //check if we didn't read all records
    {

        record->id.slot = scManager->rec.id.slot;

        printf("Total scaned recoreds: %d\n", scManager->totalScaned );

        getRecord(scan->rel, scManager->rec.id,record);  //retrive the record sent to function
        evalExpr(record, scan->rel->schema, scManager->expr, &value);
        record->id.slot = scManager->rec.id.slot++; //increment slot by one to point the next record
        scManager->totalScaned += 1;

        if (value->v.boolV == 1) //if the condition met, v.bool equals to 1
        {
            scan->mgmtData = scManager;
            printf("inside Next - satisfy the condition\n");
            return RC_OK;
        }
        else  //record not found, continue to the next record
        {

            if(scManager->totalScaned < scManager->totalTuples) //ensuring we didn't reach the top of list
                //calling the recurtion
                return next(scan, record);
            else //reach the end of the record list
            {
                return RC_RM_NO_MORE_TUPLES;
            }

        }

    }

}

// This function closes the scan operation.
RC closeScan (RM_ScanHandle *scan)
{
    //free the the memory allocated for RM_scanHandle
    scManager = scan->mgmtData;
    free(scan->mgmtData);
    scan ->mgmtData = NULL;
    return RC_OK;
}






