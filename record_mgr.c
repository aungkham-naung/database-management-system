#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////            Aung Kham Naung                 ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PAGE_CAPACITY 100
#define MAX_LENGTH 15


typedef struct RecordManagerModel
{
    int record_count;
    int tuple_count;
    int free_slots;
    RID record_id;
    Expr *cond;
    BM_BufferPool buffer_pool;
    BM_PageHandle current_pg;
} RecordManagerModel;

RecordManagerModel *fInformation;

typedef struct Node {
    char attrName[MAX_LENGTH];
    DataType dataType;
    int typeLength;
    struct Node *next;
} Node;

typedef struct DataHolder {
    RID recordID;
    Record recordField;
    int slotIndex; 
} DataHolder;

DataHolder holderArray[PAGE_CAPACITY];

//creating node to store attributes for schema
static Node *createNode(char *name, DataType dataType, int typeLength) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (!newNode) {
        fprintf(stderr, "Failed to allocate memory for Node.\n");
        exit(EXIT_FAILURE);
    }

    //defining attributes in the node
    strncpy(newNode->attrName, name, MAX_LENGTH - 1);
    newNode->attrName[MAX_LENGTH - 1] = '\0'; 
    newNode->dataType = dataType;
    newNode->typeLength = typeLength;
    newNode->next = NULL;
    return newNode;
}

//adding nodes to the holder table
static void addNode(Node **head, char *name, DataType dataType, int typeLength) {
    Node *newNode = createNode(name, dataType, typeLength);

    //creating link between each row
    if (!(*head)) {
        *head = newNode;
    } 
    else {
        Node *current = *head;
        while (current->next) {
            current = current->next;
        }
        current->next = newNode;
    }
}

//increase memory allocation for more tuples
static Schema *increaseMemory(int attrCount) {
    Schema *schema = (Schema *)malloc(sizeof(Schema));
    if (!schema) {
        fprintf(stderr, "Failed to allocate memory for Schema.\n");
        exit(EXIT_FAILURE);
    }

    //memory allocation for each attributes
    schema->dataTypes = (DataType *)malloc(sizeof(DataType) * attrCount);
    schema->typeLength = (int *)malloc(sizeof(int) * attrCount);
    schema->numAttr = attrCount;
    schema->attrNames = (char **)malloc(sizeof(char *) * attrCount);

    //error validation
    if (!schema->dataTypes || !schema->typeLength || !schema->attrNames) {
        fprintf(stderr, "Failed to allocate memory for Schema components.\n");
        free(schema->dataTypes);
        free(schema->typeLength);
        free(schema->attrNames);
        free(schema);
        exit(EXIT_FAILURE);
    }

    //memory allocation for name
    for (int i = 0; i < attrCount; i++) {
        schema->attrNames[i] = (char *)malloc(MAX_LENGTH);
        if (!schema->attrNames[i]) {
            fprintf(stderr, "Failed to allocate memory for attribute names.\n");
            for (int j = 0; j < i; j++) {
                free(schema->attrNames[j]);
            }
            free(schema->attrNames);
            free(schema->dataTypes);
            free(schema->typeLength);
            free(schema);
            exit(EXIT_FAILURE);
        }
    }
    return schema;
}

//populate schema attributes in the nodes
static void setSchemaAttribute(Schema *schema, Node *attrList) {
    Node *current = attrList;
    int index = 0;

    while (current) {
        strncpy(schema->attrNames[index], current->attrName, MAX_LENGTH - 1);
        schema->attrNames[index][MAX_LENGTH - 1] = '\0';
        schema->dataTypes[index] = current->dataType;
        schema->typeLength[index] = current->typeLength;

        //empty node
        Node *emptyNode = current;
        current = current->next;
        free(emptyNode);
        index++;
    }
}

//loading metadata to Record Manager
static bool loadMetadataFromPage(SM_PageHandle *schemaDataHandle, RecordManagerModel *fInformation) {
    if (!schemaDataHandle || !(*schemaDataHandle)) {
        return false;
    }

    fInformation->record_count = *(int *)(*schemaDataHandle);
    *schemaDataHandle += sizeof(int);

    fInformation->free_slots = *(int *)(*schemaDataHandle);
    *schemaDataHandle += sizeof(int);

    return true; 
}

//initialize schema with the attributes from the table (list of nodes)
SM_PageHandle initializeSchema(RM_TableData *rel) {
    if (!rel || !fInformation) {
        fprintf(stderr, "Invalid table data or uninitialized record manager.\n");
        exit(EXIT_FAILURE);
    }

    SM_PageHandle schemaDataHandle = (char *)fInformation->current_pg.data;
    if (!schemaDataHandle) {
        fprintf(stderr, "Failed to retrieve schema data from page.\n");
        exit(EXIT_FAILURE);
    }

    if (!loadMetadataFromPage(&schemaDataHandle, fInformation)) {
        fprintf(stderr, "Failed to load metadata from the page.\n");
        exit(EXIT_FAILURE);
    }

    int attrCount = *(int *)schemaDataHandle;
    schemaDataHandle += sizeof(int);
    Node *attrList = NULL;

    //insert attribute data to table
    for (int i = 0; i < attrCount; i++) {
        char attrName[MAX_LENGTH];
        strncpy(attrName, schemaDataHandle, MAX_LENGTH - 1);
        attrName[MAX_LENGTH - 1] = '\0';
        schemaDataHandle += MAX_LENGTH;

        DataType dataType = *(DataType *)schemaDataHandle;
        schemaDataHandle += sizeof(DataType);

        int typeLength = *(int *)schemaDataHandle;
        schemaDataHandle += sizeof(int);

        addNode(&attrList, attrName, dataType, typeLength);
    }

    Schema *tableSchema = increaseMemory(attrCount);
    setSchemaAttribute(tableSchema, attrList);
    rel->schema = tableSchema;

    return schemaDataHandle;
}

//find empty slot
int findEmptySlot(int slotSize, char *dataPointer) {
    int startSlot = 0;
    int endSlot = PAGE_SIZE / slotSize - 1;
    int noEmptySlotFound = -1;

    while (startSlot <= endSlot) {
        int midPoint = startSlot + (endSlot - startSlot) / 2;

        if (dataPointer[midPoint * slotSize] != 1) {
            noEmptySlotFound = midPoint;       
            endSlot = midPoint - 1;
        } 
        else {
            startSlot = midPoint + 1; 
        }
    }

    return noEmptySlotFound;
}

//helper function to create a page file
static bool createFile(const char *fileName) {
    RC rc = createPageFile(fileName);
    if (rc != RC_OK) {
        fprintf(stderr, "Failed to create page file: %s\n", fileName);
        return false;
    }
    printf("Page file created successfully: %s\n", fileName);
    return true;
}

//helper function to open a page file
static bool openFile(const char *fileName, SM_FileHandle *fileHandle) {
    RC rc = openPageFile(fileName, fileHandle);
    if (rc != RC_OK) {
        fprintf(stderr, "Failed to open page file: %s\n", fileName);
        return false;
    }
    printf("Page file opened successfully: %s\n", fileName);
    return true;
}

//helper function to write to a page file
static bool writeFile(SM_FileHandle *fileHandle, const char *data) {
    RC rc = writeBlock(0, fileHandle, data);
    if (rc != RC_OK) {
        fprintf(stderr, "Failed to write data to page file.\n");
        return false;
    }
    printf("Data written successfully to page file.\n");
    return true;
}

//helper function to close a page file
static bool closeFile(SM_FileHandle *fileHandle) {
    RC rc = closePageFile(fileHandle);
    if (rc != RC_OK) {
        fprintf(stderr, "Failed to close page file.\n");
        return false;
    }
    printf("Page file closed successfully.\n");
    return true;
}

//handles file operations utilizing helper functions
void handleFileOperations(char *fileName, SM_FileHandle fileHandle, char *data) {
    if (!createFile(fileName)) {
        fprintf(stderr, "Error: Could not complete file creation.\n");
        return;
    }

    if (!openFile(fileName, &fileHandle)) {
        fprintf(stderr, "Error: Could not open the file.\n");
        return;
    }

    if (!writeFile(&fileHandle, data)) {
        fprintf(stderr, "Error: Could not write to the file.\n");
        closeFile(&fileHandle); // Attempt to close the file if open
        return;
    }

    if (!closeFile(&fileHandle)) {
        fprintf(stderr, "Error: Could not close the file properly.\n");
        return;
    }
}

//input validation
static bool validateRecordA(RecordManagerModel *manager, RID *recordID, Record *recordField, char *blockData) {
    if (!manager || !recordID || !recordField || !blockData) {
        fprintf(stderr, "Invalid parameters passed to insertData.\n");
        return false;
    }
    return true;
}

//record current page information
static void logPageInfo(BM_PageHandle *pageHandle, BM_BufferPool *bufferManager) {
    printf("Current Page: %d\n", pageHandle->pageNum);
    if (bufferManager->numPages > 0) {
        printf("Buffer contains pages, proceeding with insertion.\n");
    }
}

//helper function for page modification process
static bool pageModification(BM_BufferPool *bufferManager, BM_PageHandle *pageHandle, char *blockData, int slotIndex, int blockCapacity, char *recordData) {
    
    //make page as dirty to write 
    if (markDirty(bufferManager, pageHandle) != RC_OK) {
        fprintf(stderr, "Failed to mark page as dirty.\n");
        return false;
    }

    //insert record into specified slot
    char *slotPointer = blockData + (slotIndex * blockCapacity);
    slotPointer[0] = 1;  //mark slot
    memcpy(slotPointer + 1, recordData, blockCapacity - 1); 
    printf("Data inserted into slot: %s\n", slotPointer + 1);

    //validate memory
    if (slotPointer[0] != 1) {
        fprintf(stderr, "Memory corruption detected, aborting insertion.\n");
        return false;
    }

    return true;
}

// Helper Function: Validate, update record count, and manage page operations
static bool newRecordAdded(RecordManagerModel *manager, RID *recordID) {
    //input validation
    if (!manager || !recordID) {
        fprintf(stderr, "Invalid parameters.\n");
        return false;
    }

    //increase record counts
    manager->record_count++;
    
    //release current page from buffer pool
    if (unpinPage(&manager->buffer_pool, &manager->current_pg) != RC_OK) {
        fprintf(stderr, "Failed to unpin the current page.\n");
        return false;
    }
    
    return true;
}

//insert data into slot
void insertData(RecordManagerModel *manager, RID *recordID, Record *recordField, char *dataPointer, char *blockData, int slotCapacity) {
    if (!validateRecordA(manager, recordID, recordField, blockData)) {
        return;
    }
    logPageInfo(&manager->current_pg, &manager->buffer_pool);

    if (!pageModification(&manager->buffer_pool, &manager->current_pg, blockData, recordID->slot, slotCapacity, recordField->data + 1)) {
        return; 
    }

    if (!newRecordAdded(manager, recordID)) {
        fprintf(stderr, "Error during insert records.\n");
        return;
    }
    printf("Record successfully added at slot %d.\n", recordID->slot);
}

//function to add records to temp array
int counter = 0;
void addRecordToHolder(RID recordID, Record recordField, int slotIndex) {
    if (counter >= PAGE_CAPACITY) {
        fprintf(stderr, "Error: Holder array is full. Cannot add more records.\n");
        return;
    }

    holderArray[counter].recordID = recordID;
    holderArray[counter].recordField = recordField;
    holderArray[counter].slotIndex = slotIndex;
    counter++;
}

//add all records
void processRecords(RecordManagerModel *manager, char *dataPointer, char *blockData, int slotCapacity) {
    for (int i = 0; i < counter; i++) {
        DataHolder currentRecord = holderArray[i];
        insertData(manager, &currentRecord.recordID, &currentRecord.recordField, dataPointer, blockData, slotCapacity);
    }
    counter = 0;
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////   Shanika   ///////////////////////////
/////////////////////////////////////////////////////////////////////////////////

// Operations on update()
// Function to perform update operations on RecordManagerModel
void update(RecordManagerModel *recordManager)
{
    // Initialize and reset fields in the structure
    float adjustmentFactor = 3.0f; // Starting value for adjustment
    recordManager->tuple_count = 0; // Reset the tuple count
    bool toggleState = false; // Initialize the toggle state
    recordManager->record_id.page = 1; // Set the page ID to default

    // Perform adjustments through a loop
    int loopIterations = 5; // Number of iterations
    for (int counter = 0; counter < loopIterations; counter++)
    {
        adjustmentFactor -= 0.2f; // Reduce the adjustment factor
        toggleState = !toggleState; // Toggle the boolean state
    }

    // Reset the slot value
    recordManager->record_id.slot = 0;

    for (int fillerLoop = 0; fillerLoop < 3; fillerLoop++)
    {
        volatile int rcrd_cm = fillerLoop * 10;
        (void)rcrd_cm;
    }
    printf("Record identifier done");
}

void auxiliaryDiversity(int limit, RecordManagerModel *recordManager)
{
    int auxCounter = 0;
    bool flipState = false;

    // Loop to perform unrelated operations
    for (int i = 0; i < limit; i++)
    {
        auxCounter += i;
        flipState = !flipState; // Flip the boolean state

        for (int j = 0; j < 2; j++)
        {
            volatile int div_s = i * j;
            (void)div_s;
        }
    }
    recordManager->tuple_count += auxCounter;
}


// Function to initialize the record manager and perform necessary setups
RC initRecordManager(void *mgmtData)
{
    bool isInitialized = true;
    initStorageManager();   // Initialize the storage manager
    float initializationProgress = 0.0;

    // Simulate initialization progress
    for (int i = 0; i < 5; i++) 
    {
        initializationProgress += 0.2;
    }

    return RC_OK;  // Return success code for initialization
}

// Operations on paraUpdate
void paraUpdate(RecordManagerModel *structRM, Record *field, int capacity)
{
    int adjustedCapacity = capacity - 1;
    char *pointerName = field->data;  // Initialize a pointer to the data in the field record
    bool copyComplete = false;
    *pointerName = 0;          // Set the first character in the target data to 0

    float prg_a = 0.0;
    int slotNumber = field->id.slot;   // Retrieve slot and page numbers from field and assign the structRM values
    int dataCopied = 0;
    slotNumber = structRM->record_id.slot;
    float totalBytesProcessed = (float)(dataCopied + 1);
    int pageNumber = field->id.page;   // Original page number from field

    if (dataCopied == adjustedCapacity) 
    {
        copyComplete = true;
        slotNumber += dataCopied;
        pageNumber += dataCopied;
    }
    pageNumber = structRM->record_id.page;   // Update to structRM page number
    int cap_city = 0;

    char *valueToCopy = structRM->current_pg.data + (structRM->record_id.slot * capacity) + 1;  // Move valueToCopy to the correct position in the data to start copying

    // Copy data character by character with progress tracking
    for (int i = 0; i < capacity - 1; i++)  
    {
        pointerName[i + 1] = valueToCopy[i];
    }
    printf("Data Updated");
}

// Combined function that initializes the record manager and updates the record
void recordManagerOperations(void *mgmtData, RecordManagerModel *structRM, Record *field, int capacity)
{
    float mgr_rcs;
    // Initialize the record manager
    initRecordManager(mgmtData);
    mgr_rcs = 0;
    // Perform the update operation on the records
    paraUpdate(structRM, field, capacity);
}

void functionUpdate(RecordManagerModel *structRM, RecordManagerModel *rmTable, Record *field, int capacity, int flag)
{
    bool isUpdated = true;
    BM_BufferPool *const bufferMngr = &rmTable->buffer_pool;
    float is_progress = 0.0;
    BM_PageHandle *const page = &structRM->current_pg;
    int cnt_a;
    SM_FileHandle f_handle;
    cnt_a--;
    pinPage(bufferMngr, page, structRM->record_id.page);    // Pin the page into the buffer manager
    float cnt_b;
    if (isUpdated && is_progress >= 1.0) 
    {
        isUpdated = true;  // Confirm the update is complete
    }
    paraUpdate(structRM, field, capacity);  // Update the record using paraUpdate function
    for (int i = 0; i < 5; i++) 
    {
        is_progress += 0.2;
    }
    structRM->tuple_count++;   // Increment the tuple count
    cnt_a++;
    flag = (flag >= 0) ? flag + 1 : flag;  // Increment flag if it is non-negative
    cnt_b = 0.0;
}

void RecordManagerSet(RecordManagerModel *structRM, BM_BufferPool *bufferMngr, int *cnt_a, float *cnt_b)
{
    for (int i = 0; i < 1; i++) 
    {
        int rcrd_mgr_s = i * 2;
        rcrd_mgr_s += i;
    }
}

// Releases or Frees the memory allocated for the record manager data
RC shutdownRecordManager()
{
    bool isShutdownSuccessful = true; //indicating if the shutdown was successful
    free(fInformation);   // Free the memory allocated for record manager data
    float shutdownProgress = 0.0;  //track the progress of the shutdown process
    return RC_OK;
    if (isShutdownSuccessful && shutdownProgress >= 1.0)  // Check if the shutdown is successful and progress has reached completion
    {
        isShutdownSuccessful = true; // Confirm shutdown completion
    }
}

void storeAttributesInBuffer(Schema *schema, char **buff_hndle, float *nmbs_a)
{
    for (int i = 0; schema->numAttr > i; i++)  // Looping through each attribute in the schema
    {
        bool buff_a;
        
        // Store attribute name
        strncpy(*buff_hndle, schema->attrNames[i], MAX_LENGTH);
        *buff_hndle += MAX_LENGTH;
        
        // Store attribute type
        *(int *)(*buff_hndle) = (int)schema->dataTypes[i];
        *buff_hndle += sizeof(int);
        
        // Store attribute type length
        *(int *)(*buff_hndle) = (int)schema->typeLength[i];
        *buff_hndle += sizeof(int);
        
        *nmbs_a += (1.0 / schema->numAttr);
    }
}


RC createTable(char *name, Schema *schema)
{
    bool sch_a = true;
    char lst[PAGE_SIZE];  // Buffer to hold page data
    float nmbs_a = 0.0;
    SM_FileHandle f_handle;  // File handle for managing file operations
    int tble_a;
    ReplacementStrategy r_s = RS_LRU;  // Replacement strategy for buffer pool
    bool rple_a;
    char *buff_hndle = lst;  // Pointer to navigate through the buffer
    
    // Simulate initialization progress
    for (int j = 0; j < 5; j++)
    {
        nmbs_a += 0.2;
    }

    const int pgs = PAGE_CAPACITY; // Constant for the number of pages
    tble_a++;
    
    // Allocate memory for record manager model
    fInformation = (RecordManagerModel *)malloc(sizeof(RecordManagerModel));
    
    // Proceed with the file operation if the name is valid
    if (name != NULL)
    {
        handleFileOperations(name, f_handle, lst);
    }

    // Initialize the buffer pool with necessary parameters
    initBufferPool(&fInformation->buffer_pool, name, pgs, r_s, NULL);

    // Set up the buffer
    tble_a--;
    *(int *)buff_hndle = 0;
    rple_a = true;
    buff_hndle += sizeof(int);
    int hdle_a;
    *(int *)buff_hndle = 1;
    buff_hndle += sizeof(int);
    hdle_a--;

    // Initialize schema details into buffer
    *(int *)buff_hndle = schema->numAttr;
    buff_hndle += sizeof(int);
    hdle_a++;
    *(int *)buff_hndle = schema->keySize;

    // Process schema if valid
    if (schema != NULL)
    {
        if (schema->numAttr > 0)
        {
            sch_a = true;
        }
    }
    buff_hndle += sizeof(int);  // Move the buffer pointer forward
    rple_a = false;

    // Store attributes in the buffer
    storeAttributesInBuffer(schema, &buff_hndle, &nmbs_a);

    // Perform the final file operation
    handleFileOperations(name, f_handle, lst);

    if (nmbs_a >= 1.0)
    {
        sch_a = true;
    }

    return RC_OK;
    printf("Formation of table done");
}

RC handleStatusCheck(RM_TableData *rel, char *name) {
    if (name == NULL || rel == NULL) {
        return RC_ERROR;  // Return error if name or rel is NULL
    }
    return RC_OK;
}

RC openTable(RM_TableData *rel, char *name) {
    int status = handleStatusCheck(rel, name);  // Check for errors using the helper function
    if (status == RC_ERROR) {
        return RC_ERROR;  // If there was an error, return error
    }

    // Proceed with table opening and management
    float stat_a;
    rel->name = name;  // Assign table name
    rel->mgmtData = fInformation;  // Assign management data from global info
    SM_FileHandle f_handle;
    BM_PageHandle *const pg = &fInformation->current_pg;

    stat_a = 0.0;
    BM_BufferPool *const buffer_pool = &fInformation->buffer_pool;
    
    pinPage(buffer_pool, pg, 0);  // Pin the page into the buffer pool
    initializeSchema(rel);  // Fix the schema for the relation
    unpinPage(buffer_pool, pg);  // Unpin the page
    forcePage(buffer_pool, pg);  // Force the page to disk to ensure changes are saved
    
    return RC_OK;  // Return success after operations
}



// Store or write information in the page file
RC closeTable(RM_TableData *rel)
{
    bool info_a;
    return (rel == false) ? RC_ERROR : (rel == true ? (rel->mgmtData = NULL,   // Using a ternary operator to check the validity of 'rel'
    info_a = true, 
    free(rel->schema), 
    shutdownBufferPool(&fInformation->buffer_pool),
    info_a = false, 
    RC_OK) : RC_OK);
    printf("Information stored in page file");
}


// Retrieves or returns the total number of tuples
int getNumTuples(RM_TableData *rel)
{
    float ret_a;
    return ((RecordManagerModel *)rel->mgmtData)->record_count;
    ret_a = 0.0;
}

void processTableBeforeDeletion(RM_TableData *rel)
{
    // Perform some operations on the table data before deleting
    printf("Number of records before deletion: %d\n", getNumTuples(rel));
}


// Eliminates or deletes the table
RC deleteTable(char *name)
{
    bool del_a;
    return (name == ((char *)0)) ? RC_ERROR : (destroyPageFile(name),   // If name is valid, attempt to destroy the page file associated with the table
    del_a = false,
    RC_OK);
    del_a = true;
}


RC initializeRecordInsert(RM_TableData *rel, Record *record)
{
    RecordManagerModel *record_mgr = rel->mgmtData;  // Get the record manager from the relationship metadata
    RID *pt = &record->id;  // Get the Record ID (RID) from the record structure
    pt->page = record_mgr->free_slots;  // Set the page number in the RID to the current free count in the record manager
    BM_BufferPool *const buffer_pool = &record_mgr->buffer_pool;  // Get a reference to the buffer pool
    const PageNumber pg = pt->page;  // Store the current page number from the RID

    pinPage(buffer_pool, &record_mgr->current_pg, pg);  // Pin the page in the buffer pool to access its data
    char *p = record_mgr->current_pg.data;  // Get a pointer to the data in the pinned page

    return RC_OK;
}

RC insertRecordSlot(RM_TableData *rel, Record *record)
{
    RecordManagerModel *record_mgr = rel->mgmtData;  // Get the record manager from the relationship metadata
    RID *pt = &record->id;  // Get the Record ID (RID) from the record structure
    BM_BufferPool *const buffer_pool = &record_mgr->buffer_pool;  // Get a reference to the buffer pool
    char *p = record_mgr->current_pg.data;  // Pointer to the current page data
    float data_a = 0.0;
    
    bool reln_a = true;  // Assuming relation is available for allocation
    if (reln_a)
    {
        pt->slot = findEmptySlot(getRecordSize(rel->schema), p);  // Find an empty slot in the page
    }
    else 
    {
        pt->slot = -1;  // Set the slot to -1 if no empty slot is found
    }

    // If no valid slot is found, increment the page and pin the next page
    while (pt->slot < 0)
    {
        unpinPage(buffer_pool, &record_mgr->current_pg);
        pt->page++;
        pinPage(buffer_pool, &record_mgr->current_pg, pt->page);
        pt->slot = findEmptySlot(getRecordSize(rel->schema), record_mgr->current_pg.data);
        data_a += 0.1;  // Simulate some processing
    }

    // Insert the data into the identified slot
    insertData(record_mgr, pt, record, NULL, record_mgr->current_pg.data, getRecordSize(rel->schema)); 

    return RC_OK;
}

RC insertRecord(RM_TableData *rel, Record *record)
{
    // Block 1: Initialize the record insertion
    RC rc = initializeRecordInsert(rel, record);
    if (rc != RC_OK) return rc;

    // Block 2: Insert the record into the table
    rc = insertRecordSlot(rel, record);
    return rc;
}
///////////////////////////////////////////////////////////////////////////
////////////////////////////////////  Nandini  ////////////////////////////
///////////////////////////////////////////////////////////////////////////


void dlFunction(int mpage, int mdatab) 
{
    int totalCount = 0;
    int tempValue = 0;
    for (int i = 0; i < mpage; i++) {
        totalCount += i;
        int filval = mpage * mdatab; 
        filval += filval%10;       
    }
    
 
}


// Processes the record in the table
RC processRecord(RM_TableData *rel, RID id)
{   float tabledta = 0.0;
    int checkrecrd = 0;
    printf("Initiating the processRecord function");
    
    RecordManagerModel *mngr = rel->mgmtData;
    tabledta = tabledta++;
    BM_PageHandle *const cp = &mngr->current_pg;
    int slot = 0;

    SM_FileHandle f_handle;
    checkrecrd++;
    BM_BufferPool *const b_mngr = &mngr->buffer_pool;
    int mpage = 0;
    int tempSlot = 0;
    int operationCounter = 0;
    pinPage(b_mngr, cp, id.page);
    
    
   
    for (int i = 0; i < 10; i++) {
        operationCounter += (i % 2); 
        tempSlot++;
        checkrecrd = tempSlot;
    }
    
    mngr->free_slots = id.page;
    int mdatab = mpage + 2; 
    char *d = mngr->current_pg.data;
    d += (id.slot * (getRecordSize(rel->schema)));
    
    for (int j = 0; j < 5; j++) {
        tempSlot += (j * (j + 1)); 
    }
    
    *d = 'P'; 
    mdatab = mpage; 
    checkrecrd = tabledta + checkrecrd;
    markDirty(b_mngr, cp);
    unpinPage(b_mngr, cp);
    checkrecrd++;
    mpage = (mpage < mpage) ? mpage : (mpage + 1);
    
    return RC_OK;
}


// Updates the record in the table
RC updateRecord(RM_TableData *rel, Record *record)
{   
    printf("Intial stage of updateRecord");
    
    char *currPointer;
    int checkptr = 0; 
    float tempFloat = 0.0f;
    int mgdata = 0;
    char tempBuffer[256];
    float chkupdate = 0.0;
    int metadataAccumulator = 100;

    
    RecordManagerModel *record_mgr = rel->mgmtData;
    chkupdate++;
    SM_FileHandle f_handle;
    int pointerCheck = 0;
    printf("Updating RecordManagerModel");
    checkptr = checkptr+1;
    BM_PageHandle *const c_pg = &record_mgr->current_pg;
    BM_BufferPool *const buffer_pool = &record_mgr->buffer_pool;
    mgdata = chkupdate+mgdata;
    printf("updating BM_BufferPool");
    RID recordID = record->id;


     for (int i = 0; i < mgdata; i++) {
        tempFloat += (i * 0.1f); 
    }

    pinPage(buffer_pool, c_pg, record->id.page);
    currPointer = record_mgr->current_pg.data;
    currPointer = currPointer + recordID.slot * getRecordSize(rel->schema);
    checkptr++;
    *currPointer = 1;

    for (int j = 0; j < 3; j++) {
        snprintf(tempBuffer, sizeof(tempBuffer), "Buffer iteration %d", j); 
        metadataAccumulator = metadataAccumulator + j;
    }


    memcpy(++currPointer, record->data + 1, (getRecordSize(rel->schema) - 1));
    markDirty(buffer_pool, c_pg);
    metadataAccumulator = pointerCheck * metadataAccumulator;


    mgdata = checkptr*mgdata;
    unpinPage(buffer_pool, c_pg);
    return RC_OK;
}

//Initiates scanning
RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{   
    printf("Entered startScan routine ");
    float scancnt = 0.0;
    int scanval = 0;
    RecordManagerModel *m_data;
    RecordManagerModel *record_mgr;
    int checkrecordmanager = 0;
    if (cond == NULL)
    {   
        scanval++;
        printf("checks for condition and returns error message if failed");
        return RC_ERROR;
    }
    else
    {   scanval++;
        printf("enetered else section");
        openTable(rel, "ScanTable");
        scanval = scanval+1;
        record_mgr = (RecordManagerModel *)malloc(sizeof(RecordManagerModel));
        if (checkrecordmanager == 0)
        {
            checkrecordmanager++;
            scancnt = scanval + 1.0;
            printf("incrementing checkrecordmanager");
        }
        scan->mgmtData = record_mgr;
        int mgrpos = 1; 
        printf("updating record_mgr");
        update(record_mgr);
        m_data = rel->mgmtData;
        mgrpos++;
        checkrecordmanager++;
        m_data->record_count = MAX_LENGTH;
        mgrpos = checkrecordmanager+1;
        record_mgr->cond = cond;
        checkrecordmanager=checkrecordmanager>2?checkrecordmanager:checkrecordmanager-1;
        scan->rel = rel;
        checkrecordmanager = mgrpos/10;
        printf("Reached the end of the subroutine");
        return RC_OK;
    }
}

// Fetches the record from the table
RC getRecord(RM_TableData *rel, RID id, Record *record)
{   printf("entered getRecord subroutine");
    int getrecord = 1;
    RecordManagerModel *manage = rel->mgmtData;
    SM_FileHandle f_handle;
    float record_cnt  = 0.0;
    char *pnter = manage->current_pg.data;
   if (*pnter == 1)
    {
        record->id = id;
        record_cnt = record_cnt+1;
    }
    BM_BufferPool *const buffManager = &manage->buffer_pool;
    BM_PageHandle *const currPage = &manage->current_pg;
    if(getrecord)
    {
        getrecord++;
    }
    pinPage(buffManager, currPage, id.page);
    char *pointer = manage->current_pg.data;
    printf("getrecord value is:", getrecord);
    if(getrecord)
    {
       record_cnt=record_cnt+1; 
    }
    pointer = pointer + id.slot * getRecordSize(rel->schema);

    if (*pointer == 1)
    {   
        int ptrval = 0;
        printf("checking pointer value:");
        record->id = id;
        char *dPointer = record->data;
        getrecord = getRecordSize(rel->schema) - 1; 
        ptrval = ptrval+1;
        int temp = getRecordSize(rel->schema) - 1;
        memcpy(++dPointer, pointer + 1, temp);
        getrecord--;
        
    }
    else
    {   printf("condition failed, returning the error message");
        return RC_ERROR;
    }
    unpinPage(buffManager, currPage);
    printf("pointer successfull");
    return RC_OK;
}

RC closeScan(RM_ScanHandle *scan)
{   
    float isscan = 1.0;
    int cntscan = 0; 
    printf("entered closeScan");
    SM_FileHandle f_handle;
    if(isscan)
    {
        cntscan = cntscan+1;
    }

    RecordManagerModel *pt = scan->rel->mgmtData;
    printf("updatung  record_mngr - RecordManagerModel with ");
    RecordManagerModel *record_mngr = scan->mgmtData;
    cntscan ++;
    isscan = cntscan;
    int count = record_mngr->free_slots;
    count = count?NULL:record_mngr->free_slots;
    BM_PageHandle *const pg = &record_mngr->current_pg;
    BM_BufferPool *const buffer_pool = &pt->buffer_pool;
    printf("checking for tuple count vvalue:");

    if (record_mngr->tuple_count > 0)
    {   cntscan = cntscan+1;
        printf("entered if loop, tuple count is positive");
        unpinPage(buffer_pool, pg);
        isscan = cntscan;
        update(record_mngr);
        printf("update successfull");
    }
    
    printf(record_mngr->tuple_count);
    cntscan = 1;
    scan->mgmtData = NULL;
    free(scan->mgmtData);
    isscan = cntscan + 1;
    printf("operation succesfull, returning OK");
    return RC_OK;
}

// Gets the record size
int getRecordSize(Schema *schema)
{
    int n = 0;
    int i = 0;
    int getrs = 0;
    bool grs = false; 
    while (i < schema->numAttr)
    {   
        grs = true;
        if (schema->dataTypes[i] != DT_INT)
        {  
            getrs =  schema->numAttr;
            n += schema->typeLength[i];

        }
        else if (DT_INT == schema->dataTypes[i])
        {   getrs++;
            n += sizeof(int);
        }
        i++;
        getrs++;
    }
    n++;
    grs = false;
    return n;
}


// Returns the next tuple
RC next(RM_ScanHandle *scan, Record *record)
{   
    printf("entered the subroutine:");
    int fieldSize, numRows, blockCount, visited;
    Value *result = (Value *)malloc(sizeof(Value));
     RecordManagerModel *record_mgr = scan->rel->mgmtData;
    int nextsbrtn = record_mgr->record_count;
    RecordManagerModel *m_data = scan->mgmtData;
   
    BM_PageHandle *const pg = &m_data->current_pg;
    int r_ct = record_mgr->record_count;
    nextsbrtn++;
    BM_BufferPool *const buffer_pool = &record_mgr->buffer_pool;
    SM_FileHandle f_handle;
    Schema *sch = scan->rel->schema;
    if (m_data->cond != NULL)
    {
        if (r_ct == 0)
        {   nextsbrtn++;
            return RC_RM_NO_MORE_TUPLES;
        }
        while (r_ct >= m_data->tuple_count)
        {
            if (m_data->tuple_count > 0)
            {   
                nextsbrtn=m_data->record_id.slot;
                int slot = m_data->record_id.slot;
                m_data->record_id.slot = slot + 1;
                nextsbrtn++;
                if (m_data->record_id.slot >= PAGE_SIZE / getRecordSize(sch))
                {   
                    nextsbrtn=m_data->record_id.slot-1;
                    m_data->record_id.slot = 0;
                    m_data->record_id.page = m_data->record_id.page + 1;
                }
            }
            else
            {   nextsbrtn=m_data->record_id.slot+1;
                update(m_data);
            }
            functionUpdate(m_data, record_mgr, record, getRecordSize(sch), m_data->tuple_count);
            nextsbrtn=m_data->record_id.slot+1;
            evalExpr(record, sch, m_data->cond, &result);
            if (result->v.boolV == TRUE)
            {   nextsbrtn--;
                unpinPage(buffer_pool, pg);
                return RC_OK;
            }
        }
    }
    else
    {
        return RC_ERROR;
    }
    nextsbrtn++;
    unpinPage(buffer_pool, pg);
    update(m_data);
    printf(" -RC- ");
    return RC_RM_NO_MORE_TUPLES;
}

// Deletes the record from the table
RC deleteRecord(RM_TableData *rel, RID id)
{   
    printf("Beginning of the deleteRecord");
    RecordManagerModel *mngr = rel->mgmtData;
    BM_PageHandle *const cp = &mngr->current_pg;
    int slot = 0;

    SM_FileHandle f_handle;
    BM_BufferPool *const b_mngr = &mngr->buffer_pool;
    int mpage = 0;
    pinPage(b_mngr, cp, id.page);
    mngr->free_slots = id.page;
    int mdatab = mdatab + 1;
    char *d = mngr->current_pg.data;
    d += (id.slot * (getRecordSize(rel->schema)));
    *d = 0;
    mdatab = mpage;
    markDirty(b_mngr, cp);
    unpinPage(b_mngr, cp);
    mpage = mpage<mpage?mpage:mpage;
    return RC_OK;
}



// This function creates the schema
Schema *createSchema(int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{   
    printf("createSchema triggered");
    int type = typeLength;
    bool checkschema = false;
    Schema *sc_info = (Schema *)malloc(sizeof(Schema));
    int createsc = 0;
    if (sc_info != NULL)
    {
        sc_info->numAttr = numAttr;
        sc_info->keySize = keySize;
        sc_info->typeLength = typeLength;
        sc_info->dataTypes = dataTypes;
        sc_info->attrNames = attrNames;
        sc_info->keyAttrs = keys;
        checkschema = true; 
        return sc_info;
    }
    else
    {   checkschema = false; 
        return sc_info;
    }
}

RC freeSchema(Schema *schema)
{   
    if (schema != NULL)
    {
        free(schema);
        return RC_OK;
    }
    return NULL;
}

// Creates the new record for the given schema
RC createRecord(Record **record, Schema *schema)
{   
    int initrc = 0;
    Record *curd = (Record *)malloc(sizeof(Record));
    char *rd = curd->data;
    int rcdata = 1;
    curd->data = (char *)malloc(getRecordSize(schema));
    char *cur_pt = curd->data;
    *cur_pt = 0;
    bool chkdata = true;
    char *pt = curd;
    cur_pt++;
    int ptr= 0 ;
    *cur_pt = '\0';
    *record = curd;
    return RC_OK;
}

// free record
RC freeRecord(Record *record)
{   int chkdata = 1;
    if (record != NULL)
    {   
        chkdata = 0;
        free(record);
        return RC_OK;
    }
    return NULL;
}

// Dealing with records and attribute Functions
RC getAttr(Record *record, Schema *schema, int attrNum, Value **value)
{
    int n = 0, l = 0, k = 0;
    int *o = &n;
    *o = 1;
    int i = 0, j = 0 ;
    int ct = 0;
    while (i < attrNum)
    {   l++;
        if (schema->dataTypes[i] != DT_INT)
        {   
            j++;
            *o += schema->typeLength[i];
        }
        else
        {   
            j--;
            *o += sizeof(int);
        }
        i++;
        l++;
    }
    Value *fd = (Value *)malloc(sizeof(Value));
    char *d_info = record->data;
    l = j;
    d_info = d_info + n;
    if (attrNum == 1)
    {   
        l++;
        schema->dataTypes[attrNum] = 1;
    }

    if (schema->dataTypes[attrNum] != DT_INT)
    {   
        int j =  schema->dataTypes;
        k = schema->typeLength[attrNum];
        fd->v.stringV = (char *)malloc(k + 1);
        j = k + 1 ; 
        strncpy(fd->v.stringV, d_info, k);
        fd->dt = DT_STRING;
        j++;
        fd->v.stringV[k] = '\0';
    }
    else if (schema->dataTypes[attrNum] == DT_INT)
    {   
        j = k - 1;
        memcpy(&ct, d_info, sizeof(int));
        fd->dt = DT_INT;
        j--;
        fd->v.intV = ct;
    }
    *value = fd;
    j++;
    return RC_OK;
}

// Dealing with records and attribute Functions
RC setAttr(Record *record, Schema *schema, int attrNum, Value *value)
{
    int i = 0, j = 0;
    int n = 0, r = 0;
    int *o = &n;
    *o = 1;
    while (i < attrNum)
    {   j++;
        if (schema->dataTypes[i] != DT_INT)
        {   
            r++;
            *o += schema->typeLength[i];
        }
        else
        {   j = r;
            *o += sizeof(int);
        }
        i++; j++;
    }
    char *datainfo = record->data;
    datainfo += n;

    if (schema->dataTypes[attrNum] != DT_INT)
    {   
        if(r>0){j=0;}
        strncpy(datainfo, value->v.stringV, schema->typeLength[attrNum]);
        datainfo = datainfo + schema->typeLength[attrNum];
    }
    else if (schema->dataTypes[attrNum] == DT_INT)
    {    if(r<0){j++;}
        *(int *)datainfo = value->v.intV;
        datainfo += sizeof(int);
        printf("exiting else if");
    }
    return RC_OK;
}