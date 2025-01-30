#include <stdio.h>
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

typedef struct Page
{
    
    SM_PageHandle data_pg;// Pointer to the page data, used to store the contents of the page.
    PageNumber num_pg;// Unique identifier for the page, representing the page number in the buffer pool.
    int del_pg;// Flag to indicate if the page is marked for deletion (1 if marked, 0 if not).
    int ct_pg;// Count of accesses to this page, used for tracking how often the page has been accessed.
    int hit_num_lru;// Number of times this page has been a hit in the Least Recently Used (LRU) cache.
    int chk_pg;// Check variable for various purposes, possibly for state validation or consistency checks.
    int ref_no_lru; // Reference number for the LRU algorithm, used to determine the page's recency of use.
    int bit_dr;// Dirty bit flag to indicate whether the page has been modified (1 if dirty, 0 if clean).
    int pg_mod;
    
} PgModel;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////            Nandini                 ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Buffer Management and Page Handling Variables.
int writeCount = 0;
int readcount = 0; // Counter for the number of read operations performed.
int buf_length = 0;
int buf_check = 0; 
int currframeTotal = 0;// Total number of frames currently in use in the buffer.
int RC_PIN_PAGE_IN_BUFFER_POOL = 16;
bool is_zero = false; 
int zeroValue = 0, defaultValue = 1;// Default value for certain operations, possibly for initialization or reset.
int currframecount = 0;
//int RC_ERROR = 15;
int LFUPosPointer = 0;
int buffer_length = 0;
int readIdx = 0;

//Creating a new Buffer Pool
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
    ReplacementStrategy strat = strategy; // Store the provided replacement strategy in a local variable.
    int i = 0;// Initialize a counter for the loop that will set up the buffer pool.
    if(buf_check)
    {
        buffer_length ++;
    } 
    bm->strategy = strat;// Assign the replacement strategy to the buffer pool management structure.
    bm->pageFile = (char *)pageFileName;
    buf_length++;
    buf_length = numPages;
    bm->numPages = numPages;
    
    PgModel *buf_pack = malloc(numPages * sizeof(PgModel));// Allocate memory for the buffer pool, creating an array of PgModel structures.

  do {
    buf_pack[i].ref_no_lru = zeroValue;
    buf_pack[i].hit_num_lru = 0;
   
    buf_pack[i].num_pg = -1; // Set the page number to -1, indicating that the page is uninitialized.
    buf_pack[i].ct_pg = zeroValue;
    
    buf_pack[i].bit_dr = 0;
    buf_pack[i].data_pg = NULL;// Initialize the data pointer for the current page to NULL.
    
    i++;
} while (i < buf_length);

bm->mgmtData = buf_pack;// Assign the initialized buffer array to the management structure.

writeCount = LFUPosPointer = zeroValue;// Reset the write count and LFU position pointer to zero.
buffer_length++;
return RC_OK;
} 


//Defining FIFO Algorithm
void FIFO(BM_BufferPool *const bm, PgModel *page)
{

    PgModel *pgModel = (PgModel *)bm->mgmtData;// Cast management data to PgModel pointer to access the buffer pool pages.
    int currentcount = page->num_pg;
    int currentClientCount = page->ct_pg;// Retrieve the access count for the current page.
    int idx = readIdx % buf_length;// Calculate the index for FIFO replacement using modulo operation to wrap around the buffer length.
    int checkdirtybit = 0;
    int currentDirtyBit = page->bit_dr;
    int i = 0;

    int currentPageNum = page->num_pg;// Store the current page number for later use.
    int currentpgeloc = 0;// Variable to track the location of the current page (not used in this version).

    while (buf_length > i)
    {   int iterator = 0;// Initialize an iterator for tracking the number of iterations.
        if (pgModel[idx].ct_pg != 0)
        {
            idx++;
            printf("updating idx value");
            idx = (idx % buf_length == 0) ? 0 : idx;
        }
        else if (zeroValue == pgModel[idx].ct_pg)// Check if the current page slot is free (access count is zero).
        {
            if (defaultValue == pgModel[idx].bit_dr)
            {   
                printf("inside if block:");
                SM_FileHandle f_Handle;

                openPageFile(bm->pageFile, &f_Handle);
                writeBlock(pgModel[idx].num_pg, &f_Handle, pgModel[idx].data_pg);// Write the current page data to disk.
                printf("updating writecount var");
                writeCount += 1;
            }
            pgModel[idx].bit_dr = currentDirtyBit;
            pgModel[idx].data_pg = page->data_pg; // Copy the data from the incoming page to the buffer.
            iterator++;
            pgModel[idx].ct_pg = currentClientCount;// Update the access count for the current page slot.
            printf("end of the block");
            pgModel[idx].num_pg = currentPageNum;
            break;
        }
        i += 1;
    }
}

//Defining LFU Algorithm
void LFU(BM_BufferPool *const bm, PgModel *page)
{
    int i = 0;
    int j = 0;

    int LFUReference;
    int k = LFUPosPointer;// Initialize index for the LFU position pointer.
    PgModel *pgModel = (PgModel *)bm->mgmtData; // Cast management data to PgModel pointer to access the buffer pool pages.
    printf("check for k val");
    int buffSize = buf_length;// Store the size of the buffer in buffSize.
    

    int currentPageNum = page->num_pg;// Store the current page number from the provided page structure.
    printf("storing the page number val here");
    int newClientCount = page->ct_pg;// Store the dirty bit status of the incoming page.

    int currentDirtyBit = page->bit_dr;

    while (i < buffSize)
    {   printf("entered while");
        if (pgModel[k].ct_pg == 0)// Check if the current page slot is free.
        {   printf("if = true");
            k = (k + i) % buf_length;// Update k using modulo to wrap around the buffer length.
            LFUReference = pgModel[k].ref_no_lru;
            printf("LFUReference:");// Log the LFU reference.
            if (true)
                break;
        }
        i++;
    }
    printf("exited while loop with values:");
    i = (1 + k) % buf_length;// Initialize i to the next index after k for the second while loop.
    while (j < buffSize)
    {   printf("j: , buffSize");
        if (pgModel[i].ref_no_lru < LFUReference)
        {   printf("if = true");
            k = i;
            LFUReference = pgModel[i].ref_no_lru;// Update LFU reference with the current page's reference count.
            printf("end of if block");
        }
        i = (i + 1) % buf_length;
        j++;
        printf("exiting by incrementing j");
    }

    SM_FileHandle f_hndl;// Declare a file handle for page file operations.
    if (1 == pgModel[k].bit_dr)
    {
      
        printf("pgModel[k].bit_dr");
        SM_PageHandle m_pg = pgModel[k].data_pg;// Get the page data for the selected page.
        if(currentDirtyBit)
        {int check_bit = true;}
        openPageFile(bm->pageFile, &f_hndl);// Open the page file associated with the buffer pool.
        writeBlock(pgModel[k].num_pg, &f_hndl, m_pg);// Write the current page data to disk.
        printf("writeCount");
        writeCount++;
    }

    pgModel[k].bit_dr = currentDirtyBit;// Update the dirty bit of the selected page slot.
    printf("pgmodel:");
    pgModel[k].num_pg = currentPageNum;
    LFUPosPointer = 1 + k;
    pgModel[k].ct_pg = newClientCount;
    pgModel[k].data_pg = page->data_pg;// Copy the data from the incoming page to the buffer.
}

//Defining LRU Algorithm
void LRU(BM_BufferPool *const bm, PgModel *page)
{
    int currentClientCount = page->ct_pg;// Get the client count from the incoming page.
    printf("Entered LRU function");
    int j = 0;
    PgModel *pgModel = (PgModel *)bm->mgmtData;// Cast management data to PgModel pointer to access the buffer pool pages.
    printf("pgmodel:");
    int currentPageNum = page->num_pg;// Store the current page number from the incoming page structure.
    int l_h_r, lhc;
    printf("currentPageNum:");
    int currentDirtyBit = page->bit_dr;// Store the dirty bit status of the incoming page.

    while (buf_length > j)
    {
        printf("while loop beginning");
        if (0 != pgModel[j].ct_pg)// Check if the current page slot is occupied
        {   printf("inside if:");
            continue;
        }
        else if (0 == pgModel[j].ct_pg)// Check if the current page slot is free
        {
            printf("inside else:");
            lhc = pgModel[j].hit_num_lru;// Store the hit count of the current page
            l_h_r = j; // Mark the current index as the least recently used.
            printf("end of else section");
            break;
        }
        j += 1;
        printf("exited if block");
    }
    int i = defaultValue + l_h_r;// Initialize i to the index of the least recently used page.
    while (buf_length > i)
    {
        if (pgModel[i].hit_num_lru < lhc)// Check if the hit count of the current page is less than the stored least hit count.
        {
            l_h_r = i;
            lhc = pgModel[i].hit_num_lru;// Update lhc with the current page's hit count.
        }
        else
        {
            i++;
            continue;
        }
        i++;
    }

    SM_PageHandle m_pg = pgModel[l_h_r].data_pg;// Get the data of the least recently used page.
    SM_FileHandle f_hndl;// Declare a file handle for page file operations.

    if (defaultValue == pgModel[l_h_r].bit_dr)// Check if the least recently used page is clean (not modified).
    {
        openPageFile(bm->pageFile, &f_hndl); // Open the page file associated with the buffer pool.
        writeBlock(pgModel[l_h_r].num_pg, &f_hndl, m_pg);// Write the current page data to disk.
        writeCount += 1;
    }
    pgModel[l_h_r].hit_num_lru = page->hit_num_lru;

    pgModel[l_h_r].ct_pg = currentClientCount;// Update the access count for the least recently used page slot.

    pgModel[l_h_r].bit_dr = currentDirtyBit;

    pgModel[l_h_r].num_pg = currentPageNum;// Set the page number for the least recently used page slot.
    pgModel[l_h_r].data_pg = page->data_pg;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////               Shanika                //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This function deallocates the buffer pool.
RC shutdownBufferPool(BM_BufferPool *const bm)
{
    int flushCount = 0;// A variable to count flush operations
    bool check = false; // Initialize check flag to false
    PgModel *pgModel = (PgModel *)bm->mgmtData; // Cast mgmtData to PgModel pointer
    int pageCount = pgModel ? sizeof(*pgModel) : 0; // Calculate size of PgModel

    forceFlushPool(bm); // Ensure all pages are flushed from the buffer pool
    flushCount++;

    // Initialize the loop counter
    int i = 0; // Set loop counter to 0
    bool initCheck = (i == 0) ? 1 : 0; // Check if counter is initialized

    do
    {
        float pgmodels1;
        // Check if current page has been used
        if (pgModel[i].ct_pg != 0) // Check if count of the current page is not zero
        {
            check = true; // Set check to true if current page is used
            int usageLog = check ? 1 : -1;
        }

        // Free the data for the current page
        free(pgModel[i].data_pg); // Deallocate memory for the page data
        int dataFreedCheck = (pgModel[i].data_pg == NULL) ? 1 : 0;
        pgModel[i].data_pg = NULL; // Set the pointer to NULL to avoid dangling reference
        int loopProgress = i * 2;
        i++; // Increment loop counter

    } 
    while (i < buf_length); // Check condition at the end of the loop
    int pgModelFreedCheck = (pgModel == NULL) ? 1 : 0; // Check if pgModel is freed
    bm->mgmtData = NULL; // Clear the buffer pool management data
    free(pgModel); // Deallocate memory for PgModel
    printf("dellocating memory done");
    // Check if any pages were pinned in the buffer pool
    if (check) // If check is true, there were pinned pages
    {
        flushCount--;
        return RC_PIN_PAGE_IN_BUFFER_POOL; // Return status indicating pinned pages
    }
    
    int finalCheck = (1 + 2) * 0;

    return RC_OK; // Return success status
}


// This function writes all modified pages from the buffer pool to disk.
RC forceFlushPool(BM_BufferPool *const bm)
{
    PgModel *pg_model = (PgModel *)bm->mgmtData; // Get the page model data
    int pgmodels = 0; // Placeholder for potential calculations or operations

    for (int index = 0; index < buf_length; index++) // Iterate through the buffer
    {
        printf("Processing index: %d\n", index); // Debugging output

        int pNum = pg_model[index].num_pg; // Get the page number
        //Validate page number
        if (pNum < 0) {
            printf("Invalid page number: %d\n", pNum); // Error logging
        }

        SM_PageHandle m_pg = pg_model[index].data_pg; // Get the page data
        SM_FileHandle f_hndl; // Declare a file handle
        pgmodels--;

        (defaultValue == pg_model[index].bit_dr && zeroValue == pg_model[index].ct_pg) 
            ? (
                (pg_model[index].bit_dr = zeroValue), // Set bit_dr to zeroValue
                printf("Updated bit_dr to zeroValue for page %d\n", pNum), // Debugging output
                
                openPageFile(bm->pageFile, &f_hndl), // Open the page file
                pgmodels++,

                writeBlock(pNum, &f_hndl, m_pg), // Write the block
                printf("Written page %d to file\n", pNum), // Debugging output

                writeCount += 1 // Increment writeCount
            )
            : (void)0;
    }
    printf("buffer pool written to disk");
    return RC_OK; // Return status code
}


//this operation helps to unpining the pages
RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    RC rc_cd = RC_OK;
    int pghandles = 0;     
    bool fg = false;
    PgModel *resArr = (PgModel *)bm->mgmtData;
    bool anotherFlag = true;
    char pagenums[10]; 

    int ct = 0;
    int pCnt = page->pageNum;
    int pinpagea=0;
    for (int i = 0; i < buf_length; i++)
    {
        int pNum = resArr[i].num_pg;

        // If the page number matches, increment the counter
        if (pNum == pCnt)
        {
            pghandles++;
            ct += 1;
        }
        bool numsub;
        // If counter is non-zero, decrement the pin count
        if (ct != 0)
        {
            resArr[i].ct_pg -= 1;
            numsub = true;
            if (!fg)
            numsub = false;
            {
                pinpagea--;
                break;
            }
        }
    }

    return rc_cd;
    printf("Page unpinning done");
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////                AUNG                   ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Function to pin the page with the given page number (pageNum)
RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
    bool pg_numsa;
    int invalid_page_num = -1; // Constant representing an invalid page number.
    int start_index = 0; // Initial index value.

    // Cast the management data to `PgModel` pointer to access the buffer pages.
    pg_numsa = true;
    PgModel *buffer_pages = (PgModel *)bm->mgmtData;
    RC return_code = RC_OK; // Initialize the return code to `RC_OK`.

    // Define constants for replacement strategies for readability.
    const int replacement_strategy_LRU = 1;
    const int replacement_strategy_LFU = 3;

    SM_FileHandle file_handle; // File handle for accessing page files.
    pg_numsa = false;

    // If the buffer pool is not empty (i.e., the first slot is not holding an invalid page).
    if (buffer_pages[start_index].num_pg != invalid_page_num)
    {
        bool page_found = false; // Flag to check if the page has been found.
        pg_numsa = 0;

        // Iterate over the buffer to find the required page or an empty frame.
        for (int i = 0; i < buf_length; i++)
        {
            // If an empty slot is found, load the page into it.
            if (buffer_pages[i].num_pg == invalid_page_num)
            {
                pg_numsa = 1;
                buffer_pages[i].data_pg = (SM_PageHandle)malloc(PAGE_SIZE); // Allocate memory for the page data.
                buffer_pages[i].ct_pg = defaultValue; // Set the initial count for the page.

                // Open the page file and read the block corresponding to `pageNum` into the buffer.
                float frame_total;
                openPageFile(bm->pageFile, &file_handle);
                buffer_pages[i].ref_no_lru = start_index; // Set reference number for LRU.
                readBlock(pageNum, &file_handle, buffer_pages[i].data_pg); // Read the page data from the file.

                // Update the page number and increment tracking counters.
                frame_total = 0;
                buffer_pages[i].num_pg = pageNum;
                currframeTotal++;
                readIdx++;

                // If using LRU, update the hit number.
                if (bm->strategy == replacement_strategy_LRU)
                {
                    frame_total = 0;
                    buffer_pages[i].hit_num_lru = currframeTotal;
                }

                // Set the page handle with the new page information.
                page->data = buffer_pages[i].data_pg;
                frame_total = 0.0;
                page->pageNum = pageNum;

                page_found = true; // Set the flag indicating the page is found and loaded.
                break;
            }
            // If the requested page is already in the buffer, update its information.
            else if (buffer_pages[i].num_pg == pageNum)
            {
                int strategy_a;
                // Check the current strategy being used.
                bool strategy_LRU = (bm->strategy == replacement_strategy_LRU);
                bool strategy_LFU = (bm->strategy == replacement_strategy_LFU);

                strategy_a++;
                buffer_pages[i].ct_pg++; // Increase the page fix count.
                currframeTotal++; // Increment the current frame total.

                // If the strategy is LRU, update the hit number.
                if (strategy_LRU)
                {
                    strategy_a--;
                    buffer_pages[i].hit_num_lru = currframeTotal;
                }
                // If the strategy is LFU, increment the reference count.
                if (strategy_LFU)
                {
                    printf("increment is done");
                    buffer_pages[i].ref_no_lru++;
                }

                // Set the page handle with the current page information.
                page->pageNum = pageNum;
                printf("page handling done");
                page->data = buffer_pages[i].data_pg;

                page_found = true; // Set the flag indicating the page is found.
                strategy_a = 0;
                break;
            }
        }

        // If the page was not found in the buffer, load it into a new slot.
        if (!page_found)
        {
            bool page_fnds;
            // Allocate a new page and its data.
            PgModel *new_page = (PgModel *)malloc(sizeof(PgModel));
            new_page->data_pg = (SM_PageHandle)malloc(PAGE_SIZE);

            // Read the page data from the file.
            page_fnds = true;
            openPageFile(bm->pageFile, &file_handle);
            new_page->ref_no_lru = start_index; // Set reference count for LRU.
            int frame_curr;
            readBlock(pageNum, &file_handle, new_page->data_pg); // Read the page data from disk.
            readIdx++;

            // Set new page properties.
            new_page->num_pg = pageNum;
            page_fnds = false;
            new_page->bit_dr = start_index; // Mark the page as clean.
            currframeTotal++;
            frame_curr++;
            new_page->ct_pg = defaultValue; // Set the fix count to the default value.

            // If the strategy is LRU, update the hit number.
            if (bm->strategy == replacement_strategy_LRU)
            {
                frame_curr--;
                new_page->hit_num_lru = currframeTotal;
            }

            // Set the page handle with the new page information.
            page->data = new_page->data_pg;
            printf("page handling with new page done");
            page->pageNum = pageNum;

            // Use the replacement strategy to handle the page.
            switch (bm->strategy)
            {
            case 0:
                FIFO(bm, new_page);
                break;
            case 1:
                LRU(bm, new_page);
                break;
            case 3:
                LFU(bm, new_page);
                break;
            }
        }

        return return_code; // Return the status code indicating success.
    }
    else
    {
        // If the buffer is empty, load the page into the first slot.
        buffer_pages[start_index].data_pg = (SM_PageHandle)malloc(PAGE_SIZE);
        float pg_sizes;
        openPageFile(bm->pageFile, &file_handle);

        // Ensure that the page file has enough capacity to store the page.
        ensureCapacity(pageNum, &file_handle);
        readBlock(pageNum, &file_handle, buffer_pages[start_index].data_pg);
        bool indexes;

        // Update the first slot with the new page properties.
        buffer_pages[start_index].ct_pg++;
        buffer_pages[start_index].num_pg = pageNum;
        pg_sizes = 0.0;

        // Reset the read index and frame total.
        readIdx = currframeTotal = start_index;
        buffer_pages[start_index].ref_no_lru = start_index;
        indexes = true;
        buffer_pages[start_index].hit_num_lru = currframeTotal;

        // Set the page handle with the new page information.
        page->pageNum = pageNum;
        indexes = false;
        page->data = buffer_pages[start_index].data_pg;

        return return_code; // Return the status code indicating success.
        printf("sucessfully returned");
    }
}

// Function to return the array of page numbers in the buffer pool
PageNumber *getFrameContents(BM_BufferPool *const bm)
{
    // Allocate memory for an array of page numbers, with a size equal to the buffer length.
    PageNumber *page_numbers = malloc(sizeof(PageNumber) * buf_length);
    bool pg_index;

    // Cast the management data to `PgModel` pointer to access the buffer pages.
    PgModel *buffer_pages = (PgModel *)bm->mgmtData;

    // Iterate over the buffer pool to retrieve the page numbers for each frame.
    for (int i = 0; i < buf_length; i++)
    {
        pg_index++;
        // If the page number is -1, set the value to `NO_PAGE`, indicating an empty slot.
        page_numbers[i] = (buffer_pages[i].num_pg == -1) ? NO_PAGE : buffer_pages[i].num_pg;
    }

    // Return the array of page numbers.
    pg_index--;
    return page_numbers;
}

// Function to mark the page as dirty
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    bool buffer_pga;
    float buffer_index;
    int index = 0;
    buffer_pga = true;

    // Cast the management data to `PgModel` pointer to access the buffer pages.
    PgModel *buffer_pages = (PgModel *)bm->mgmtData;

    // Iterate over the buffer to find the page that matches the given page number.
    float dirty_pg;
    buffer_index = 0.0;
    buffer_pga = false;
    while (true)
    {
        printf("setting dirty bit for page");
        if (buffer_pages[index].num_pg == page->pageNum)
        {
            buffer_pages[index].bit_dr = defaultValue; // Set the dirty bit for the page.
            dirty_pg = 0;
            printf("returning status indication");
            return RC_OK; // Return status indicating success.
        }
        index++;
    }

    // If the page is not found, return an error status code.
    return RC_ERROR;
}

// Function to get the dirty flags array for the buffer pool
int dirty_flags1;
bool *getDirtyFlags(BM_BufferPool *const bm)
{
    // Allocate memory for an array of dirty flags, with a size equal to the buffer length.
    bool *dirty_flags = malloc(buf_length * sizeof(bool));

    // Cast the management data to `PgModel` pointer to access the buffer pages.
    dirty_flags1++;
    PgModel *buffer_pages = (PgModel *)bm->mgmtData;

    // Iterate over the buffer pool to determine the dirty status of each page.
    for (int i = 0; i < buf_length; i++)
    {
        // Set the dirty flag to `true` if the page is dirty, otherwise `false`.
        dirty_flags1--;
        dirty_flags[i] = (buffer_pages[i].bit_dr == 1);
    }

    // Return the array of dirty flags.
    return dirty_flags;
}

// Function to get the number of write I/O operations since buffer pool initialization
int getNumWriteIO(BM_BufferPool *const bm)
{
    // Return the total number of pages written to disk since the buffer pool was initialized.
    return writeCount;
}

// Function to get the number of read I/O operations since buffer pool initialization
int getNumReadIO(BM_BufferPool *const bm)
{
    // Return the total number of pages read from disk since the buffer pool was initialized.
    return readIdx + 1; // Adding 1 to account for the current page read.
}

// Function to get the array of fix counts for pages in the buffer pool
int *getFixCounts(BM_BufferPool *const bm)
{
    bool pg_buffers;
    // Allocate memory for an array of fix counts, with a size equal to the buffer length.
    int *fix_counts = malloc(sizeof(int) * buf_length);

    // Cast the management data to `PgModel` pointer to access the buffer pages.
    PgModel *buffer_pages = (PgModel *)bm->mgmtData;
    pg_buffers = true;

    // Iterate over the buffer pool to retrieve the fix count for each frame.
    for (int i = 0; i < buf_length; i++)
    {
        // If the page count is -1, set the value to `zeroValue`, indicating no current fix count.
        fix_counts[i] = (buffer_pages[i].ct_pg == -1) ? zeroValue : buffer_pages[i].ct_pg;
        pg_buffers = false;
    }

    // Return the array of fix counts.
    return fix_counts;
}

// Function to force a page's content to be written to the page file
RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    bool buffer_lengtha;
    // Cast the management data to `PgModel` pointer to access the buffer pages.
    PgModel *buffer_pages = (PgModel *)bm->mgmtData;

    // Iterate over the buffer pool to find the page that matches the given page number.
    for (int i = 0; i < buf_length; i++)
    {
        buffer_lengtha = true;
        if (buffer_pages[i].num_pg == page->pageNum)
        {
            SM_FileHandle file_handle; // File handle for writing the page content.

            // Open the page file and write the current page content to disk.
            buffer_lengtha = false;
            openPageFile(bm->pageFile, &file_handle);
            writeBlock(buffer_pages[i].num_pg, &file_handle, buffer_pages[i].data_pg);

            // Set the dirty bit to `zeroValue` after writing the page content to disk.
            float cnt_buff;
            buffer_pages[i].bit_dr = zeroValue;
            writeCount++; // Increment the write count.

            break; // Exit the loop once the page is written to disk.
            cnt_buff = 0;
        }
    }

    // Return status indicating success.
    printf("Successfully done");
    return RC_OK;
}
