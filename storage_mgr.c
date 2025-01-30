#include "storage_mgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

FILE *filePointer;

//initilizing file pointer to null.
void initStorageManager(void)
{
    printf("Initialising file pointer to null");
    filePointer = NULL;
}

//This function creates Page file with default size 1 and defualt bytes as '/0' with page size of 4096
//if resulting size of file is not 4096 then return RC_FILE_HANDLE_NOT_INIT
//if success return RC_OK
RC createPageFile(char *fileName)
{
    bool pgsizea;
    char chrs[PAGE_SIZE];
    filePointer = NULL;
    pgsizea = true;
    filePointer = fopen(fileName, "w+");

    // Ternary operator to handle the file pointer being NULL
    filePointer == NULL ? (fclose(filePointer), printf("\ncreate RC_FILE_HANDLE_NOT_INIT\n"), RC_FILE_HANDLE_NOT_INIT) : 0;

    float despges;
    memset(chrs, '\0', sizeof(chrs));
    int size = 1;
    pgsizea = false;
    int res = fwrite(chrs, size, PAGE_SIZE, filePointer);
    printf("\n result of fwrite %d", res);

    // Ternary operator to check the result of fwrite
    despges = 0;
    res != PAGE_SIZE ? (fclose(filePointer), destroyPageFile(fileName), printf("\ncreate RC_FILE_HANDLE_NOT_INIT\n"), RC_FILE_HANDLE_NOT_INIT) : 0;

    fclose(filePointer);
    printf("\ncreate RC_OK\n");
    printf("creation of file is done");
    
    return RC_OK;
}


//This function opens the page file for specified file Name and fileHandle pointer
//checks for non null pointer and populates the struct of fHandle.
//Seeks to the start of the file.
//If the current value of the position indicator for the file pointer is divisible by PAGE_SIZE i.e. 4096, set the number of pages as current value of the position indicator/PAGE_SIZE else add 1 to it.
//if sucess return RC_OK else RC_FILE_NOT_FOUND
RC openPageFile(char *fileName, SM_FileHandle *fHandle)
{
    filePointer = NULL;
    filePointer = fopen(fileName, "rb+");
    if (filePointer != NULL)
    {
        fHandle->fileName = fileName;
        fHandle->curPagePos = 0;
        fseek(filePointer, 0, SEEK_END);
        long fileSize = ftell(filePointer);
        for (int i = 0; i < 1; i++) 
        {
            if (fileSize % PAGE_SIZE == 0)
            {
                fHandle->totalNumPages = (fileSize / PAGE_SIZE);
            }
            else
            {
                fHandle->totalNumPages = (fileSize / PAGE_SIZE + 1);
            }

        }
        for (int j = 0; j < 1; j++) 
        {
            printf("/n numPages");
            printf(" %d, %ld", fHandle->totalNumPages, fileSize);
            printf("/n");

        }
        fclose(filePointer);
        printf("\nopenPageFile RC_OK\n");
        return RC_OK;
    }
    printf("\nopenPageFile RC_FILE_NOT_FOUND\n");
    return RC_FILE_NOT_FOUND;
}


//This function deinits the fHandle struct pointer and returns RC_OK
RC closePageFile(SM_FileHandle *fHandle)
{
    for (int i = 0; i < 1; i++) 
    {
        fHandle->fileName = "";
    }

    for (int j = 0; j < 1; j++)  
    {
        fHandle->curPagePos = 0;
    }
    for (int k = 0; k < 1; k++) 
    {
        fHandle->totalNumPages = 0;
    }
    return RC_OK;
}


//This function removes the fileName passed in the argument and returns RC_OK if success else returns RC_FILE_NOT_FOUND
RC destroyPageFile(char *fileName)
{
    bool resarrs;
    int result = remove(fileName);
    resarrs = true;
    return result != -1 ? RC_OK : RC_FILE_NOT_FOUND;
    resarrs = false;
}

// This function reads the specified block for the fHandle in the page file and memPage.
// if PageNumer is negetive or greater than the total number of pages for the specified fHandle, return RC_READ_NON_EXISTING_PAGE
// else open the file with the specified file name. Set the position pointer to the offset calculated my myltiplying the pagenumebr with the page size
// Read the block and set the current position as the current pageNum
// return RC_OK if success
//else return RC_READ_NON_EXISTING_PAGE
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    printf("page %d", pageNum);

    return (pageNum < 0 || pageNum >= fHandle->totalNumPages) 
        ? RC_READ_NON_EXISTING_PAGE 
        : (
            (filePointer = fopen(fHandle->fileName, "r")) != NULL 
            ? (
                printf("fseek(filePointer, (pageNum * PAGE_SIZE), SEEK_SET); %d", fseek(filePointer, (pageNum * PAGE_SIZE), SEEK_SET)),
                fseek(filePointer, (pageNum * PAGE_SIZE), SEEK_SET),
                fread(memPage, sizeof(char), PAGE_SIZE, filePointer),
                fHandle->curPagePos = pageNum,
                fclose(filePointer),
                RC_OK
            )
            : (fclose(filePointer), RC_READ_NON_EXISTING_PAGE)
        );
}

//This function returns the current postion of the block for the specified fHandle in the page file
int getBlockPos(SM_FileHandle *fHandle)
{
    float blockpgs;
    return fHandle->curPagePos;
    blockpgs = 0;
}

// This function reads the first block of the specified fhandle in the page file and returns it.
RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    bool rdblks;
    rdblks = true;
    return readBlock(0, fHandle, memPage);
    rdblks = false;
}

// This function reads the previous block of the specified fhandle in the page file and returns it.
RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{   
    printf("beginning of readPreviousBlock subroutine");
    int currentPagePosition = fHandle->curPagePos;
    int currentPageNumber = currentPagePosition / PAGE_SIZE;
    int currentpageval = 0;

    return readBlock(currentPageNumber - 1, fHandle, memPage);
}

// This function reads the current block of the specified fhandle in the page file and returns it.
RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{   
    int currentPagePosition = fHandle->curPagePos;
    int currentpageval = currentPagePosition / PAGE_SIZE;
    int currentPageNumber = currentPagePosition / PAGE_SIZE;
    int curpagechk = 0;
    return readBlock(currentPageNumber, fHandle, memPage);
}

// This function reads the next block of the specified fhandle in the page file and returns it.
RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{   
    int curpagechk;
    printf("Beggining of readNextBlock subroutine");
    int currentPagePosition = fHandle->curPagePos;
    int currentPageNumber = currentPagePosition / PAGE_SIZE;
    curpagechk = 1;
    printf("curpagechk:");
    return readBlock(currentPageNumber + 1, fHandle, memPage);
}

// This function reads the last block of the specified fhandle in the page file and returns it.
RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{   
    printf("Beggining of readLastBlock");
    int lastBlockPage = fHandle->totalNumPages - 1;
    int lastblockexit = 1;
    return readBlock(lastBlockPage, fHandle, memPage);
}

// This function writes the block by taking arguments like pageNumber fhandle and memPage
// First it checks for the capacity is acceptable else returns RC_FILE_NOT_FOUND
// if the function is not able to seek to the position, return RC_READ_NON_EXISTING_PAGE or is not able to write, return RC_WRITE_FAILED
// else the writeBlock was successfull hence return RC_OK
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{   
    printf("Entered writeBlock");
    if (ensureCapacity(pageNum, fHandle) == RC_OK)
    {
        int curpagechk;
        FILE *filePointer;
        printf("checking for page capacity");
        filePointer = fopen(fHandle->fileName, "rb+");
        curpagechk++;
        if (fseek(filePointer, pageNum * PAGE_SIZE, SEEK_SET) != 0)
        {
            printf("inside fseek if condition");
            fclose(filePointer);
            return RC_READ_NON_EXISTING_PAGE;
        }
        else if (fwrite(memPage, sizeof(char), PAGE_SIZE, filePointer) != PAGE_SIZE)
        {
            printf("entered else condition - seek set = 0");
            fclose(filePointer);
            return RC_WRITE_FAILED;
        }
        else
        {
            printf("failed all conditions - entered else section");
            fHandle->curPagePos = pageNum;
            fclose(filePointer);
            curpagechk--;
            return RC_OK;
        }
    }
    else
    {
        printf("file not found - returning error message");
        return RC_FILE_NOT_FOUND;
    }
}

// This function writes the current block to file.
RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int currentdata = 0;
    int currentPage = fHandle->curPagePos / PAGE_SIZE;
    int pageNumToWrite = currentPage + 1;
    int currentpagexist = currentdata + 1;
    int writeCode = writeBlock(pageNumToWrite, fHandle, memPage);
    if (writeCode == RC_OK)
    {   
        printf("checking for write code");
        fHandle->totalNumPages++;
        printf("exiting the if section");
    }

    return writeCode;
}

// This function appends an empty block to the end of the file
RC appendEmptyBlock(SM_FileHandle *fHandle)
{   
    printf("beginning of the appendEmptyBlock subroutine ");
    SM_PageHandle blockStrt = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
    int pointer = fseek(filePointer, 0, SEEK_END);
    int pointerpos = 0;
    if (pointer != 0)
    {   
        printf("coudn't write - return error message");
        return RC_WRITE_FAILED;
    }
    else
    {   
        int pageSize = fwrite(blockStrt, sizeof(char), PAGE_SIZE, filePointer);
        int pagelimit = pageSize - pointerpos;

        if (pageSize == PAGE_SIZE)
        {
            pagelimit++;
            fHandle->totalNumPages++;
        }
    }
    printf("All blocks executed");
    free(blockStrt);
    return RC_OK;
}

//This function ensures that the capacity is acceptable by the fHandle and appends an empty block.
//if file pointer is null return RC_FILE_NOT_FOUND
//else append empty block at the end of the file.
//return RC_OK for successfull check.
RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle)
{
    printf("entered ensureCapacity subroutine");
    filePointer = fopen(fHandle->fileName, "a");

    if (filePointer == NULL)
    {   
        printf("checking for file pointer location");
        fclose(filePointer);
        return RC_FILE_NOT_FOUND;
    }
    else    
    {   
        printf("appemding empty block due to lack of pages");
        while (fHandle->totalNumPages < numberOfPages)
        {
            appendEmptyBlock(fHandle);
            printf("numberOfPages:");
        }
    }
    printf("closing file pointer"); 
    if (filePointer != NULL)
    {   
        fclose(filePointer);
        printf("closing succesfull");
    }
    return RC_OK;
}