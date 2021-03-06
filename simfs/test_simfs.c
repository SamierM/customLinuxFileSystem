#include "simfs.h"

#include <stdio.h>

#define SIMFS_FILE_NAME "simfsFile.dta"

int main()
{
//    srand(time(NULL)); // uncomment to get true random values in get_context()

    if (simfsCreateFileSystem(SIMFS_FILE_NAME) != SIMFS_NO_ERROR)
        exit(EXIT_FAILURE);

    if (simfsMountFileSystem(SIMFS_FILE_NAME) != SIMFS_NO_ERROR)
        exit(EXIT_FAILURE);

    // TODO: implement thorough testing of all the functionality

    // the following is just some sample code for simulating user and process identifiers that are
    // needed in the simfs functions
    // int count = 10;
    // char *content;
    // struct fuse_context *context;
    // for (int i = 0; i < count; i++)
    // {
    //     context = simfs_debug_get_context();
    //     printf("user ID = %02i, process ID = %02i, group ID = %02i, umask = %04o\n",
    //            context->uid, context->pid, context->gid, context->umask);
    //     content = simfsGenerateContent(i * 10);
    //     printf("content = \"%s\"\nhash(content) = %ld\n", content, hash((unsigned char *) content));
    // }

    //testing create file FILE_CONTENT_TYPE
    if(simfsCreateFile("testFileForCreate", FILE_CONTENT_TYPE) == SIMFS_NO_ERROR)
        printf("testFileForCreate created successfully!\n" );
    if(simfsCreateFile("testFileForCreate", FILE_CONTENT_TYPE) != SIMFS_DUPLICATE_ERROR)
        printf("simfsCreateFile detected duplicate successfully\n");


    ///////////////////////////////////////////////////////////
    //testing delete file
    if(simfsDeleteFile("fileDoesNotExist") == SIMFS_NOT_FOUND_ERROR)
        printf("We correctly did not find the file!\n");
    else
        printf("Should have produced a not found error!");
    if(simfsDeleteFile("/testFileForCreate") == SIMFS_NO_ERROR)
        printf("Correctly deleted the test file!");
    else
        printf("We did not correctly delete the test file!");
    //@TODO Access error testing and implementation
    // Also to test deleting a non empty folder


    ///////////////////////////////////////////////////////////

    if (simfsUmountFileSystem(SIMFS_FILE_NAME) != SIMFS_NO_ERROR)
        exit(EXIT_FAILURE);

    if (simfsMountFileSystem(SIMFS_FILE_NAME) != SIMFS_NO_ERROR)
        exit(EXIT_FAILURE);

    if (simfsUmountFileSystem(SIMFS_FILE_NAME) != SIMFS_NO_ERROR)
        exit(EXIT_FAILURE);

    // unsigned char testBitVector[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    // simfsFlipBit(testBitVector, 44);
    // printf("Found free block at %d\n", simfsFindFreeBlock(testBitVector));
    // simfsClearBit(testBitVector, 33);
    // printf("Found free block at %d\n", simfsFindFreeBlock(testBitVector));
    // simfsSetBit(testBitVector, 33);
    // printf("Found free block at %d\n", simfsFindFreeBlock(testBitVector));

    return EXIT_SUCCESS;
}
