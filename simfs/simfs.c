#include "simfs.h"

//////////////////////////////////////////////////////////////////////////
//
// allocation of the in-memory data structures
//
//////////////////////////////////////////////////////////////////////////
//Access Rights
// 0  Owner | Group | ALL
//    RWE   | RWE   | RWE

SIMFS_CONTEXT_TYPE *simfsContext; // all in-memory information about the system
SIMFS_VOLUME *simfsVolume;
SIMFS_INDEX_TYPE *currentWorkingDirectory;

//////////////////////////////////////////////////////////////////////////
//
// simfs function implementations
//
//////////////////////////////////////////////////////////////////////////

/*
 * Constructs in-memory directory of all files is the system.
 *
 * Starting with the file system root (pointed to from the superblock) traverses the hierarachy of directories
 * and adds en entry for each folder or file to the directory by hashing the name and adding a directory
 * entry node to the conflict resolution list for that entry. If the entry is NULL, the new node will be
 * the only element of that list.
 *
 * The function sets the current working directory to refer to the block holding the root of the volume. This will
 * be changed as the user navigates the file system hierarchy.
 *
 */

/*
 * Retuns a hash value within the limits of the directory.
 */
inline unsigned long hash(unsigned char *str) {
    register unsigned long hash = 5381;
    register unsigned char c;

    while ((c = *str++) != '\0')
        hash = ((hash << 5) + hash) ^ c; /* hash * 33 + c */

    return hash % SIMFS_DIRECTORY_SIZE;
}

/*
 * Find a free block in a bit vector.
 */
inline unsigned short simfsFindFreeBlock(unsigned char *bitvector) {
    unsigned short i = 0;
    while (bitvector[i] == 0xFF)
        i += 1;

    register unsigned char mask = 0x80;
    unsigned short j = 0;
    while (bitvector[i] & mask) {
        mask >>= 1;
        ++j;
    }

    return (i * 8) + j; // i bytes and j bits are all "1", so this formula points to the first "0"
}

/*
 * Three functions for bit manipulation.
 */
inline void simfsFlipBit(unsigned char *bitvector, unsigned short bitIndex) {
    unsigned short blockIndex = bitIndex / 8;
    unsigned short bitShift = bitIndex % 8;

    register unsigned char mask = 0x80;
    bitvector[blockIndex] ^= (mask >> bitShift);
}

inline void simfsSetBit(unsigned char *bitvector, unsigned short bitIndex) {
    unsigned short blockIndex = bitIndex / 8;
    unsigned short bitShift = bitIndex % 8;

    register unsigned char mask = 0x80;
    bitvector[blockIndex] |= (mask >> bitShift);
}

inline void simfsClearBit(unsigned char *bitvector, unsigned short bitIndex) {
    unsigned short blockIndex = bitIndex / 8;
    unsigned short bitShift = bitIndex % 8;

    register unsigned char mask = 0x80;
    bitvector[blockIndex] &= ~(mask >> bitShift);
}

/*
 * Allocates space for the file system and saves it to disk.
 */
SIMFS_ERROR simfsCreateFileSystem(char *simfsFileName) {

    FILE *file = fopen(simfsFileName, "wb");
    if (file == NULL)
        return SIMFS_ALLOC_ERROR;

    simfsContext = malloc(sizeof(SIMFS_CONTEXT_TYPE));
    if (simfsContext == NULL)
        return SIMFS_ALLOC_ERROR;

    simfsVolume = malloc(sizeof(SIMFS_VOLUME));
    if (simfsVolume == NULL)
        return SIMFS_ALLOC_ERROR;

    //initialize the globalOpenFileTableValues to emmpty
    for(int i = 0; i < SIMFS_MAX_NUMBER_OF_OPEN_FILES; i++){
        simfsContext->globalOpenFileTable[i].type = INVALID_CONTENT_TYPE; //this signals that it is empty
    }

    // initialize the superblock

    simfsVolume->superblock.attr.rootNodeIndex = 0;
    simfsVolume->superblock.attr.blockSize = SIMFS_BLOCK_SIZE;
    simfsVolume->superblock.attr.numberOfBlocks = SIMFS_NUMBER_OF_BLOCKS;

    // initialize the blocks holding the root folder

    // initialize the root folder

    simfsVolume->block[0].type = FOLDER_CONTENT_TYPE;
    simfsVolume->block[0].content.fileDescriptor.type = FOLDER_CONTENT_TYPE;
    strcpy(simfsVolume->block[0].content.fileDescriptor.name, "/");
    simfsVolume->block[0].content.fileDescriptor.accessRights = umask(00000);
    simfsVolume->block[0].content.fileDescriptor.owner = 0; // arbitrarily simulated
    simfsVolume->block[0].content.fileDescriptor.size = 0;

    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    simfsVolume->block[0].content.fileDescriptor.creationTime = time.tv_sec;
    simfsVolume->block[0].content.fileDescriptor.lastAccessTime = time.tv_sec;
    simfsVolume->block[0].content.fileDescriptor.lastModificationTime = time.tv_sec;

    // initialize the index block of the root folder

    // first, point from the root file descriptor to the index block
    simfsVolume->block[0].content.fileDescriptor.block_ref = 1;

    simfsVolume->block[1].type = INDEX_CONTENT_TYPE;

    // indicate that the blocks #0 and #1 are allocated

    // using the function to find a free block for testing purposes
    simfsFlipBit(simfsVolume->bitvector, simfsFindFreeBlock(simfsVolume->bitvector)); // should be 0
    simfsFlipBit(simfsVolume->bitvector, simfsFindFreeBlock(simfsVolume->bitvector)); // should be 1

    // sample alternative #1 - illustration of bit-wise operations
//    simfsVolume->bitvector[0] = 0;
//    simfsVolume->bitvector[0] |= 0x01 << 7; // set the first bit of the bit vector
//    simfsVolume->bitvector[0] += 0x80 >> 1; // flip the first bit of the bit vector

    // sample alternative #2 - less educational, but fastest
//     simfsVolume->bitvector[0] = 0xC0;
    // 0xC0 is 11000000 in binary (showing the root block and root's index block taken)

    fwrite(simfsVolume, 1, sizeof(SIMFS_VOLUME), file);

    fclose(file);

    return SIMFS_NO_ERROR;
}

/*
 * Loads the file system from a disk and constructs in-memory directory of all files is the system.
 *
 * Starting with the file system root (pointed to from the superblock) traverses the hierarachy of directories
 * and adds en entry for each folder or file to the directory by hashing the name and adding a directory
 * entry node to the conflict resolution list for that entry. If the entry is NULL, the new node will be
 * the only element of that list.
 *
 * The function sets the current working directory to refer to the block holding the root of the volume. This will
 * be changed as the user navigates the file system hierarchy.
 *
 */

SIMFS_ERROR simfsMountFileSystem(char *simfsFileName) {
    simfsContext = malloc(sizeof(SIMFS_CONTEXT_TYPE));
    for (int i = 0; i < SIMFS_DIRECTORY_SIZE; i++) {
        simfsContext->directory[i] = NULL;
    }

    if (simfsContext == NULL)
        return SIMFS_ALLOC_ERROR;

    simfsVolume = malloc(sizeof(SIMFS_VOLUME));
    if (simfsVolume == NULL)
        return SIMFS_ALLOC_ERROR;

    FILE *file = fopen(simfsFileName, "rb");
    if (file == NULL)
        return SIMFS_ALLOC_ERROR;

    //Mounting System into memory
    SIMFS_BLOCK_TYPE currentBlock = simfsVolume->block[simfsVolume->block[0].content.fileDescriptor.block_ref]; //gets us the contents of the initial block
    hashFileSystem(currentBlock, simfsVolume->block[0].content.fileDescriptor.size);
    strcpy(simfsContext->bitvector, simfsVolume->bitvector);


    fread(simfsVolume, 1, sizeof(SIMFS_VOLUME), file);
    fclose(file);
    return SIMFS_NO_ERROR;

    // TODO: complete

}

//does a depth first recursive search of all the files in the system and hashes the information into memory
void hashFileSystem(SIMFS_BLOCK_TYPE currentBlock, size_t numberOfFiles) {
    for (int i = 0; i < numberOfFiles; i++) {
        SIMFS_BLOCK_TYPE fileToHash = simfsVolume->block[currentBlock.content.index[i]];
        unsigned long hashedName = hash(fileToHash.content.fileDescriptor.name);
        if (fileToHash.type == FOLDER_CONTENT_TYPE) {
            hashFileSystem(simfsVolume->block[fileToHash.content.fileDescriptor.block_ref],
                           fileToHash.content.fileDescriptor.size);
        }
        SIMFS_DIR_ENT *conflictResList = (simfsContext->directory)[hashedName];
        addFileDescriptorToList(conflictResList, currentBlock.content.index[i]);
    }
}

void addFileDescriptorToList(SIMFS_DIR_ENT *conflictResList, SIMFS_INDEX_TYPE descriptorIndex) {
    SIMFS_DIR_ENT *currentList = conflictResList;

    while (currentList != NULL) {
        currentList = (currentList->next);
    }
    currentList = (SIMFS_DIR_ENT *) malloc(sizeof(SIMFS_DIR_ENT));
    currentList->nodeReference = descriptorIndex;
    currentList->next = NULL;
}

/*
 * Saves the file system to a disk and de-allocates the memory.
 *
 * Assumes that all synchronization has been done.
 *
 */
SIMFS_ERROR simfsUmountFileSystem(char *simfsFileName) {
    FILE *file = fopen(simfsFileName, "wb");
    if (file == NULL)
        return SIMFS_ALLOC_ERROR;

    fwrite(simfsVolume, 1, sizeof(SIMFS_VOLUME), file);
    fclose(file);

    free(simfsVolume);
    free(simfsContext);

    return SIMFS_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////

/*
 * Depending on the type parameter the function creates a file or a folder in the current directory
 * of the process. If the process does not have an entry in the processControlBlock, then the root directory
 * is assumed to be its current working directory.
 *
 * Hashes the file name and check if the file with such name already exists in the in-memory directory.
 * If it is then it return SIMFS_DUPLICATE_ERROR.
 * Otherwise:
 *    - finds an available block in the storage using the in-memory bitvector and flips the bit to indicate
 *      that the block is taken
 *    - initializes a local buffer for the file descriptor block with the block type depending on the parameter type
 *      (i.e., folder or file)
 *    - creates an entry in the conflict resolution list for the corresponding in-memory directory entry
 *    - copies the local buffer to the disk block that was found to be free
 *    - copies the in-memory bitvector to the bitevector blocks on the simulated disk
 *
 *  The access rights and the the owner are taken from the context (umask and uid correspondingly).
 *
 */
SIMFS_ERROR simfsCreateFile(SIMFS_NAME_TYPE fileName, SIMFS_CONTENT_TYPE type) {
    strcpy(simfsContext->bitvector,simfsVolume->bitvector);
    unsigned long hashedName;
    SIMFS_INDEX_TYPE currentDirectoryIndex;
    SIMFS_BLOCK_TYPE *currentDirectory;
    char *nameWithPath;

    if (simfsContext->processControlBlocks != NULL) {
        currentDirectoryIndex = simfsContext->processControlBlocks->currentWorkingDirectory;
        currentDirectory = &(simfsVolume->block[currentDirectoryIndex]);
        nameWithPath = (char*)malloc(sizeof(fileName) + sizeof(currentDirectory->content.fileDescriptor.name) + 10);
        strcpy(nameWithPath, currentDirectory->content.fileDescriptor.name);
    } else {
        nameWithPath = (char*)malloc(sizeof(fileName) + 10);
        currentDirectory = &(simfsVolume->block[0]); //root directory
    }
    strcat(nameWithPath, "/");
    strcat(nameWithPath, fileName); //creates filename with path prepended

    hashedName = hash(nameWithPath);
    size_t numberOfFilesInDirectory = currentDirectory->content.fileDescriptor.size;
    SIMFS_INDEX_TYPE *indexedFiles;

    SIMFS_DIR_ENT *headOfCollisionList = simfsContext->directory[hashedName];
    char *nameToCheck;
    while (headOfCollisionList != NULL) {
        nameToCheck = simfsVolume->block[headOfCollisionList->nodeReference].content.fileDescriptor.name;
        if (namesAreSame(nameToCheck, nameWithPath))
            return SIMFS_DUPLICATE_ERROR;
        headOfCollisionList = headOfCollisionList->next;
    }


    SIMFS_FILE_DESCRIPTOR_TYPE *descriptorBuffer = (SIMFS_FILE_DESCRIPTOR_TYPE *) malloc(
            sizeof(SIMFS_FILE_DESCRIPTOR_TYPE));
    SIMFS_FILE_DESCRIPTOR_TYPE *indexBuffer;
    time(&descriptorBuffer->creationTime);
    descriptorBuffer->lastAccessTime = descriptorBuffer->creationTime;
    descriptorBuffer->lastModificationTime = descriptorBuffer->creationTime;
    descriptorBuffer->size = 0; //always initialize to 0 which means empty file/folder(may change in the future)
    // 0  Owner | Group | ALL
    //    RWE   | RWE   | RWE
    unsigned short allAccessRights = 0777;

    descriptorBuffer->accessRights = allAccessRights;
    strcpy(descriptorBuffer->name, nameWithPath);
    descriptorBuffer->type = type;
    currentDirectory.content.fileDescriptor.size++;

    unsigned short freeBitIndex = simfsFindFreeBlock(simfsContext->bitvector);
    simfsFlipBit(simfsContext->bitvector, freeBitIndex);
    simfsVolume->block[freeBitIndex].content.fileDescriptor = *descriptorBuffer;
    SIMFS_DIR_ENT *newCollisionListEntry;
    headOfCollisionList = simfsContext->directory[hashedName];
    if(headOfCollisionList == NULL){
        simfsContext->directory[hashedName] = (SIMFS_DIR_ENT *) malloc(sizeof(SIMFS_DIR_ENT));
        simfsContext->directory[hashedName]->nodeReference=freeBitIndex;
        simfsContext->directory[hashedName]->next = NULL;
    }
    else{
        while(headOfCollisionList->next != NULL){
            headOfCollisionList = headOfCollisionList->next;
        }
        headOfCollisionList->next = (SIMFS_DIR_ENT *) malloc(sizeof(SIMFS_DIR_ENT));
        headOfCollisionList->next->nodeReference=freeBitIndex;
        headOfCollisionList->next = NULL;
    }

    simfsVolume->block[freeBitIndex].type = type;
    if (type == FILE_CONTENT_TYPE){
        descriptorBuffer->block_ref = SIMFS_INVALID_INDEX;
    }
    else if (type == FOLDER_CONTENT_TYPE) {
        freeBitIndex = simfsFindFreeBlock(simfsContext->bitvector);
        simfsFlipBit(simfsContext->bitvector, freeBitIndex);
        simfsVolume->block[freeBitIndex].type = INDEX_CONTENT_TYPE;
        newCollisionListEntry = (SIMFS_DIR_ENT *) malloc(sizeof(SIMFS_DIR_ENT));
        newCollisionListEntry->nodeReference = freeBitIndex;
        newCollisionListEntry->next = NULL;
        headOfCollisionList = newCollisionListEntry;
    }


    strcpy(simfsVolume->bitvector, simfsContext->bitvector);

    return SIMFS_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////

/*
 * Deletes a file from the file system.
 *
 * Hashes the file name and check if the file is in the directory. If not, then it returns SIMFS_NOT_FOUND_ERROR.
 * Otherwise:
 *    - finds the reference to the file descriptor block
 *    - if the referenced block is a folder that is not empty, then returns SIMFS_NOT_EMPTY_ERROR.
 *    - Otherwise: 
 *       - checks if the process owner can delete this file or folder; if not, it returns SIMFS_ACCESS_ERROR.
 *       - Otherwise:
 *          - frees all blocks belonging to the file by flipping the corresponding bits in the in-memory bitvector
 *          - frees the reference block by flipping the corresponding bit in the in-memory bitvector
 *          - clears the entry in the folder by removing the corresponding node in the list associated with
 *            the slot for this file
 *          - copies the in-memory bitvector to the bitvector blocks on the simulated disk
 */
SIMFS_ERROR simfsDeleteFile(SIMFS_NAME_TYPE fileName) {
    strcpy(simfsContext->bitvector,simfsVolume->bitvector);
    unsigned long hashedName = hash(fileName);
    SIMFS_DIR_ENT *listElement = simfsContext->directory[hashedName];
    SIMFS_DIR_ENT *prevListElement = listElement;
    SIMFS_FILE_DESCRIPTOR_TYPE matchedDescriptor;
    SIMFS_BLOCK_TYPE potentialMatch;
    //size_t numberOfFilesToCheck =

    while(listElement != NULL){
        potentialMatch = simfsVolume->block[listElement->nodeReference];
        if((potentialMatch.type == FILE_CONTENT_TYPE || potentialMatch.type == FOLDER_CONTENT_TYPE) &&
            namesAreSame(fileName,potentialMatch.content.fileDescriptor.name)){
            matchedDescriptor = potentialMatch.content.fileDescriptor;
            break;
        }
        prevListElement = listElement;
        listElement = listElement->next;
    }
    if(listElement == NULL)
        return SIMFS_NOT_FOUND_ERROR;
    if(matchedDescriptor.type == FOLDER_CONTENT_TYPE && matchedDescriptor.size > 0)
        return SIMFS_NOT_EMPTY_ERROR;
    unsigned char mask = 0200; //bitmask representing owners ability to write to file
    if((mask & matchedDescriptor.accessRights) != mask)
        return SIMFS_ACCESS_ERROR;

    prevListElement->next = listElement->next; //remove from conflict resolution list
    simfsFlipBit(simfsContext->bitvector, listElement->nodeReference);
    strcpy(simfsVolume->bitvector, simfsContext->bitvector);
    return SIMFS_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////

/*
 * Finds the file in the in-memory directory and obtains the information about the file from the file descriptor
 * block referenced from the directory.
 *
 * If the file is not found, then it returns SIMFS_NOT_FOUND_ERROR
 */
SIMFS_ERROR simfsGetFileInfo(SIMFS_NAME_TYPE fileName, SIMFS_FILE_DESCRIPTOR_TYPE *infoBuffer) {
    unsigned long hashedName = hash(fileName);
    SIMFS_DIR_ENT *currentDirectory = simfsContext->directory[hashedName];
    while (currentDirectory != NULL) {
        if (strcmp(simfsVolume->block[currentDirectory->nodeReference].content.fileDescriptor.name, fileName) == 0) {
            infoBuffer = &(simfsVolume->block[currentDirectory->nodeReference].content.fileDescriptor);
            return SIMFS_NO_ERROR;
        }
        currentDirectory = (currentDirectory->next);
    }

    return SIMFS_NOT_FOUND_ERROR;
}

//////////////////////////////////////////////////////////////////////////

/*
 * Hashes the name and searches for it in the in-memory directory. If the file does not exist,
 * the SIMFS_NOT_FOUND_ERROR is returned.
 *
 * Otherwise:
 *    - checks the per-process open file table for the process, and if the file has already been opened
 *      it returns the index of the openFileTable with the entry of the file through the parameter fileHandle, and
 *      returns SIMFS_DUPLICATE_ERROR as the return value
 *
 *    - otherwise, checks if there is a global entry for the file, and if so, then:
 *       - it increases the reference count for this file
 *
 *       - otherwise, it creates an entry in the global open file table for the file copying the information
 *         from the file descriptor block referenced from the entry for this file in the directory
 *
 *       - if the process does not have its process control block in the processControlBlocks list, then
 *         a file control block for the process is created and added to the list; the current working directory
 *         is initialized to the root of the volume and the number of the open files is initialized to 0
 *
 *       - if an entry for this file does not exits in the per-process open file table, the function finds an
 *         empty slot in the table and fills it with the information including the reference to the entry for
 *         this file in the global open file table.
 *
 *       - returns the index to the new element of the per-process open file table through the parameter fileHandle
 *         and SIMFS_NO_ERROR as the return value
 *
 * If there is no free slot for the file in either the global file table or in the per-process
 * file table, or if there is any other allocation problem, then the function returns SIMFS_ALLOC_ERROR.
 *
 */
//Access Rights
// 0  Owner | Group | ALL
//    RWE   | RWE   | RWE
SIMFS_ERROR simfsOpenFile(SIMFS_NAME_TYPE fileName, SIMFS_FILE_HANDLE_TYPE *fileHandle) {
    //Simulate FUSE processes
    struct fuse_context *context = malloc(sizeof(struct fuse_context));
    context->pid = 1;
    context->uid = 1;
    //

    strcpy(simfsContext->bitvector,simfsVolume->bitvector);
    unsigned long hashedName = hash(fileName);
    SIMFS_DIR_ENT *listElement = simfsContext->directory[hashedName];
    SIMFS_FILE_DESCRIPTOR_TYPE matchedDescriptor;
    SIMFS_BLOCK_TYPE potentialMatch;
    while(listElement != NULL){
        potentialMatch = simfsVolume->block[listElement->nodeReference];
        if((potentialMatch.type == FILE_CONTENT_TYPE || potentialMatch.type == FOLDER_CONTENT_TYPE) &&
           namesAreSame(fileName,potentialMatch.content.fileDescriptor.name)){
            matchedDescriptor = potentialMatch.content.fileDescriptor;
            break;
        }
        listElement = listElement->next;
    }
    if(listElement == NULL)
        return SIMFS_NOT_FOUND_ERROR;

    //check to see if our current process exists
    SIMFS_PROCESS_CONTROL_BLOCK_TYPE *processBlockEntry = simfsContext->processControlBlocks;
    SIMFS_OPEN_FILE_GLOBAL_TABLE_TYPE *globalEntry;
    int placeHolder = -1;
    bool foundProcessInList = false;

    while( processBlockEntry != NULL){
        if(processBlockEntry->pid == context->pid){
            foundProcessInList = true;
            break;
        }
    }
    if(!foundProcessInList){
        processBlockEntry = createProcessEntry(context);
    }
    //Search for file table entry
    if((placeHolder = currentProcessHasFile(processBlockEntry,fileName)) >= 0)
        return SIMFS_DUPLICATE_ERROR;
    else{
        globalEntry = getGlobalEntry(fileName);
        if(globalEntry != NULL){ //if there exists an entry
            globalEntry->referenceCount++;
        }
        else{ //if there is not an entry, create one
            globalEntry = createGlobalFileTableEntry(fileName,listElement->nodeReference);
            if(assignGlobalEntry(globalEntry) == SIMFS_ALLOC_ERROR ||
            assignProcessTableEntryIndex(globalEntry,processBlockEntry) == SIMFS_ALLOC_ERROR)
                return SIMFS_ALLOC_ERROR;
        }
        if(placeHolder >= 0) *fileHandle = placeHolder;
    }
    return SIMFS_NO_ERROR;
}

SIMFS_OPEN_FILE_GLOBAL_TABLE_TYPE* getGlobalEntry(char* fileName){
    SIMFS_OPEN_FILE_GLOBAL_TABLE_TYPE* globalEntry = NULL;
    SIMFS_FILE_DESCRIPTOR_TYPE fileDescriptor;
    for(int i = 0; i < SIMFS_MAX_NUMBER_OF_OPEN_FILES; i++){
        fileDescriptor =simfsVolume->block[simfsContext->globalOpenFileTable[i].fileDescriptor].content.fileDescriptor;
        if(namesAreSame(fileName, fileDescriptor.name)){
            globalEntry = (SIMFS_OPEN_FILE_GLOBAL_TABLE_TYPE*)malloc(sizeof(SIMFS_OPEN_FILE_GLOBAL_TABLE_TYPE));
            globalEntry->fileDescriptor = simfsContext->globalOpenFileTable[i].fileDescriptor;
            globalEntry->referenceCount = 0;
            globalEntry->type = fileDescriptor.type;
            globalEntry->accessRights = fileDescriptor.accessRights;
            globalEntry->size = fileDescriptor.size;
            globalEntry->creationTime = fileDescriptor.creationTime;
            globalEntry->lastAccessTime = fileDescriptor.lastAccessTime;
            globalEntry->lastModificationTime = fileDescriptor.lastModificationTime;
            globalEntry->owner = fileDescriptor.owner;
        }
    }

    return globalEntry;
}

int currentProcessHasFile(SIMFS_PROCESS_CONTROL_BLOCK_TYPE *processBlockEntry,char *fileName){
    for(int i = 0; i < SIMFS_MAX_NUMBER_OF_OPEN_FILES_PER_PROCESS; i++){
        if(namesAreSame(simfsVolume->block[processBlockEntry->openFileTable[i].globalEntry->fileDescriptor].content
        .fileDescriptor.name,fileName)){
            return i;
        }
    }

    return -1;
}

SIMFS_OPEN_FILE_GLOBAL_TABLE_TYPE* createGlobalFileTableEntry(char* fileName, SIMFS_INDEX_TYPE index){
    SIMFS_OPEN_FILE_GLOBAL_TABLE_TYPE *globalEntry = (SIMFS_OPEN_FILE_GLOBAL_TABLE_TYPE *)malloc(sizeof(fileName));
    SIMFS_FILE_DESCRIPTOR_TYPE *fileDescriptor = (SIMFS_FILE_DESCRIPTOR_TYPE*)malloc(sizeof(SIMFS_FILE_DESCRIPTOR_TYPE));
    simfsGetFileInfo(fileName,fileDescriptor);
    globalEntry->referenceCount = 0;
    globalEntry->type = fileDescriptor->type;
    globalEntry->accessRights = fileDescriptor->accessRights;
    globalEntry->size = fileDescriptor->size;
    globalEntry->creationTime = fileDescriptor->creationTime;
    globalEntry->lastAccessTime = fileDescriptor->lastAccessTime;
    globalEntry->lastModificationTime = fileDescriptor->lastModificationTime;
    globalEntry->owner = fileDescriptor->owner;
    globalEntry->fileDescriptor = index;

    return globalEntry;
}

SIMFS_PROCESS_CONTROL_BLOCK_TYPE* createProcessEntry(struct fuse_context *context){
    SIMFS_PROCESS_CONTROL_BLOCK_TYPE *processControlBlock = simfsContext->processControlBlocks;
    SIMFS_PROCESS_CONTROL_BLOCK_TYPE * prevBlock = processControlBlock;
    while(processControlBlock != NULL){
        if(processControlBlock->pid == context->pid)
            return processControlBlock;
        else{
            prevBlock = processControlBlock;
            processControlBlock = processControlBlock->next;
        }
    }
    processControlBlock = (SIMFS_PROCESS_CONTROL_BLOCK_TYPE*)malloc(sizeof(SIMFS_PROCESS_CONTROL_BLOCK_TYPE));
    processControlBlock->next = NULL;
    processControlBlock->pid = context->pid;
    processControlBlock->numberOfOpenFiles = 0;
    processControlBlock->currentWorkingDirectory = 0;
    if(simfsContext->processControlBlocks == NULL){
        simfsContext->processControlBlocks = processControlBlock;
    }
    else{
        prevBlock->next = processControlBlock;
    }
    return processControlBlock;
}

SIMFS_ERROR assignProcessTableEntryIndex(SIMFS_OPEN_FILE_GLOBAL_TABLE_TYPE *globalEntry,
        SIMFS_PROCESS_CONTROL_BLOCK_TYPE *processBlockEntry){
    for(int i = 0; i < SIMFS_MAX_NUMBER_OF_OPEN_FILES_PER_PROCESS; i++){
        if(processBlockEntry->openFileTable[i].globalEntry == NULL) {
            processBlockEntry->openFileTable[i].globalEntry = globalEntry;
            return SIMFS_NO_ERROR;
        }
    }

    return SIMFS_ALLOC_ERROR;
}

SIMFS_ERROR assignGlobalEntry(SIMFS_OPEN_FILE_GLOBAL_TABLE_TYPE *globalEntry){

    for(int i = 0; i < SIMFS_MAX_NUMBER_OF_OPEN_FILES; i++){
        if(simfsContext->globalOpenFileTable[i].type ==INVALID_CONTENT_TYPE){
            simfsContext->globalOpenFileTable[i] = *globalEntry;
            return SIMFS_NO_ERROR;
        }
    }

    return SIMFS_ALLOC_ERROR;
}


//////////////////////////////////////////////////////////////////////////

/*
 * The function replaces content of a file with new one pointed to by the parameter writeBuffer.
 *
 * Checks if the file handle points to a valid file descriptor of an open file. If the entry is invalid
 * (e.g., if the reference to the global table is NULL, or if the entry in the global table is INVALID_CONTENT_TYPE),
 * then it returns SIMFS_NOT_FOUND_ERROR.
 *
 * Otherwise, it checks the access rights for writing. If the process owner is not allowed to write to the file,
 * then the function returns SIMFS_ACCESS_ERROR.
 *
 * Then, the functions calculates the space needed for the new content and checks if the write buffer can fit into
 * the remaining free space in the file system. If not, then the SIMFS_ALLOC_ERROR is returned.
 *
 * Otherwise, the function removes all blocks currently held by this file, and then acquires new blocks as needed
 * modifying bits in the in-memory bitvector as needed.
 *
 * It then copies the characters pointed to by the parameter writeBuffer (until '\0' but excluding it) to the
 * new blocks that belong to the file. The function copies any modified block of the in-memory bitvector to
 * the corresponding bitvector block on the disk.
 *
 * Finally, the file descriptor is modified to reflect the new size of the file, and the times of last modification
 * and access.
 *
 * The function returns SIMFS_WRITE_ERROR in response to exception not specified earlier.
 *
 */
SIMFS_ERROR simfsWriteFile(SIMFS_FILE_HANDLE_TYPE fileHandle, char *writeBuffer) {
    SIMFS_OPEN_FILE_GLOBAL_TABLE_TYPE *foundFile = &(simfsContext->globalOpenFileTable[fileHandle]);
    unsigned short writeRightAccessMask = 0200;

    if(foundFile == NULL || foundFile->type == INVALID_CONTENT_TYPE)
        return SIMFS_NOT_FOUND_ERROR;
    else if((foundFile->accessRights & writeRightAccessMask) != writeRightAccessMask){
        return SIMFS_ACCESS_ERROR;
    }
    int numberOfBlocksNeeded = sizeof(writeBuffer)/SIMFS_BLOCK_SIZE;
    if(sizeof(writeBuffer)%SIMFS_BLOCK_SIZE > 0 || numberOfBlocksNeeded == 0)
        numberOfBlocksNeeded++;
    if(!validNumberOfBlocksExist(numberOfBlocksNeeded))
        return SIMFS_ALLOC_ERROR;

    SIMFS_FILE_DESCRIPTOR_TYPE writeToFileDescriptor = simfsVolume->block[foundFile->fileDescriptor].content.fileDescriptor;
   if(simfsVolume->block[writeToFileDescriptor.block_ref].type == INDEX_CONTENT_TYPE){
        for(int i = 0; i < SIMFS_INDEX_SIZE;i++){
            simfsFlipBit(simfsContext->bitvector,simfsVolume->block[writeToFileDescriptor.block_ref].content.index[i]);
        }
    }

   //copy data
    int freeBitIndex;
    SIMFS_BLOCK_TYPE referenceBlock;
    //simfsFlipBit(simfsContext->bitvector,writeToFileDescriptor.block_ref);
    if(numberOfBlocksNeeded == 1){
        freeBitIndex = simfsFindFreeBlock(simfsContext->bitvector);
        referenceBlock = simfsVolume->block[freeBitIndex];
        referenceBlock.type = DATA_CONTENT_TYPE;
        for(int i = 0; i < sizeof(writeBuffer); i++){
            referenceBlock.content.data[i] = writeBuffer[i];
        }
    }
    else{
        int indexOfWriteBuffer = 0;
        freeBitIndex = simfsFindFreeBlock(simfsContext->bitvector);
        referenceBlock = simfsVolume->block[freeBitIndex];
        referenceBlock.type = INDEX_CONTENT_TYPE;
        do {
            for (int i = 0; i < SIMFS_INDEX_SIZE - 1 && indexOfWriteBuffer < sizeof(writeBuffer)-1; i++) {
                referenceBlock.content.index[i] = simfsFindFreeBlock(simfsContext);
                simfsFlipBit(simfsContext->bitvector,referenceBlock.content.index[i]);
                simfsVolume->block[referenceBlock.content.index[i]].type = DATA_CONTENT_TYPE;
                for (int j = 0; j < SIMFS_DATA_SIZE && indexOfWriteBuffer < sizeof(writeBuffer)-1; j++) {
                    simfsVolume->block[referenceBlock.content.index[i]].content.data[j] = writeBuffer[indexOfWriteBuffer++];
                }
            }
            freeBitIndex = simfsFindFreeBlock(simfsContext->bitvector);
            simfsFlipBit(simfsContext->bitvector,freeBitIndex);
            referenceBlock = simfsVolume->block[freeBitIndex];
            referenceBlock.type = INDEX_CONTENT_TYPE;
        }while(indexOfWriteBuffer < sizeof(writeBuffer));
    }
    writeToFileDescriptor.size = sizeof(writeBuffer);
    writeToFileDescriptor.lastAccessTime = time(0);
    writeToFileDescriptor.lastModificationTime = writeToFileDescriptor.lastAccessTime;

    strcpy(simfsVolume->bitvector, simfsContext->bitvector);
    return SIMFS_NO_ERROR;
}

bool validNumberOfBlocksExist(int numberOfBlocksNeeded){
    for(int i = 0; i < SIMFS_NUMBER_OF_BLOCKS/8; i++){

    }
    return false;
}

//////////////////////////////////////////////////////////////////////////

/*
 * The function returns the complete content of the file to the caller through the parameter readBuffer.
 *
 * Checks if the file handle points to a valid file descriptor of an open file. If the entry is invalid
 * (e.g., if the reference to the global table is NULL, or if the entry in the global table is INVALID_CONTENT_TYPE),
 * then it returns SIMFS_NOT_FOUND_ERROR.
 *
 * Otherwise, it checks the user's access right to read the file. If the process owner is not allowed to read the file,
 * then the function returns SIMFS_ACCESS_ERROR.
 *
 * Otherwise, the function allocates memory sufficient to hold the read content with an appended end of string
 * character; the pointer to newly allocated memory is passed back through the readBuffer parameter. All the content
 * of the blocks is concatenated using the allocated space, and an end of string character is appended at the end of
 * the concatenated content.
 *
 * The function returns SIMFS_READ_ERROR in response to exception not specified earlier.
 *
 */
SIMFS_ERROR simfsReadFile(SIMFS_FILE_HANDLE_TYPE fileHandle, char **readBuffer) {
    // TODO: implement

    return SIMFS_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////

/*
 * Removes the entry for the file with the file handle provided as the parameter from the open file table
 * for this process. It decreases the number of open files for in the process control block of this process, and
 * if it becomes zero, then the process control block for this process is removed from the processControlBlocks list.
 *
 * Decreases the reference count in the global open file table, and if that number is 0, it also removes the entry
 * for this file from the global open file table.
 *
 */

SIMFS_ERROR simfsCloseFile(SIMFS_FILE_HANDLE_TYPE fileHandle) {
    struct fuse_context *currentContext = simfs_debug_get_context();
    SIMFS_OPEN_FILE_GLOBAL_TABLE_TYPE *globalFileTableRef;
    SIMFS_PROCESS_CONTROL_BLOCK_TYPE *currentProcessBlock = simfsContext->processControlBlocks;
    SIMFS_PROCESS_CONTROL_BLOCK_TYPE *prevProcessBlock = currentProcessBlock;
    SIMFS_PER_PROCESS_OPEN_FILE_TYPE *globalReference;

    //remove entry for file from the open file table
    while(currentProcessBlock != NULL){
        if(currentProcessBlock->pid == currentContext->pid){
            if(--currentProcessBlock->numberOfOpenFiles == 0){ //deletes from process list if process has no other files
                prevProcessBlock->next = currentProcessBlock->next;
                currentProcessBlock->next = NULL;
            }
            globalReference = &(currentProcessBlock->openFileTable[fileHandle]);
            if(--globalReference->globalEntry->referenceCount == 0){
                globalReference->globalEntry->type = INVALID_CONTENT_TYPE;
            }
            break;
        }
        prevProcessBlock = currentProcessBlock;
        currentProcessBlock = currentProcessBlock->next;
    }


    return SIMFS_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////
//
// The following functions are provided only for testing without FUSE.
//
// When linked to the FUSE library, both user ID and process ID can be obtained by calling fuse_get_context().
// See: http://libfuse.github.io/doxygen/structfuse__context.html
//
//////////////////////////////////////////////////////////////////////////

/*
 * Simulates FUSE context to get values for user ID, process ID, and umask through fuse_context
 */

struct fuse_context *simfs_debug_get_context() {

    // TODO: replace its use with FUSE's fuse_get_context()

    struct fuse_context *context = malloc(sizeof(struct fuse_context));

    context->fuse = NULL;
    context->uid = (uid_t) rand() % 10 + 1;
    context->pid = (pid_t) rand() % 10 + 1;
    context->gid = (gid_t) rand() % 10 + 1;
    context->private_data = NULL;
    context->umask = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; // can be changed as needed

    return context;
} FUSE_CONTEXT;

char *simfsGenerateContent(int size) {
    size = (size <= 0 ? rand() % 1000 : size); // arbitrarily chosen as an example

    char *content = malloc(size);

    int firstPrintable = ' ';
    int len = '~' - firstPrintable;

    for (int i = 0; i < size - 1; i++)
        *(content + i) = firstPrintable + rand() % len;

    content[size - 1] = '\0';
    return content;
}

bool namesAreSame(char *name1, char *name2) {
    if (strcmp(name1, name2) == 0)
        return true;
    return false;
}
