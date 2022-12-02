#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/bf.h"
#include "../include/hp_file.h"
#include "../include/record.h"

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}



/*--------------------------------------------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------------------------------------------*/


/* CONSTANTS */
#define HEAP_FILE_ID "P"
#define HP_ERROR -1
#define HP_OK 0
#define NONE -1
#define MAXIMUM_BLOCK_RECORDS ((BF_BLOCK_SIZE-sizeof(HP_block_info))/sizeof(Record))


/* CREATE AND INITIALIZE A HEAP-FILE */
int HP_CreateFile(char* fileName)
{

    /* If no filename was given */
    if(fileName==NULL)
    {
        printf("No filename given!\n");
        return HP_ERROR;
    }

    /* Create a file */
    int code = BF_CreateFile(fileName);

    /* If file's creation failed */
    if(code!=BF_OK)
    {
        BF_PrintError(code);
        return HP_ERROR;
    }

    /* Open the file you just created */
    int fileDescriptor;
    code = BF_OpenFile(fileName,&fileDescriptor);

    /* If file's opening failed */
    if(code!=BF_OK)
    {
        BF_PrintError(code);
        return HP_ERROR;
    }

    /* Create a container block */
    BF_Block *block;
    BF_Block_Init(&block);

    /* Create the header of the file */
    code = BF_AllocateBlock(fileDescriptor, block);

    /* If header's creation failed */
    if(code!=BF_OK)
    {
        BF_PrintError(code);

        /* Destroy the container block */
        BF_Block_Destroy(&block);

        /* Close the file */
        code = BF_CloseFile(fileDescriptor);

        /* If file's close failed */
        if(code!=BF_OK)
            BF_PrintError(code);

        return HP_ERROR;
    }

    /* Go to the header's start and store heap-file's ID */
    char* dataPointer = BF_Block_GetData(block);
    memcpy(dataPointer,HEAP_FILE_ID,sizeof(HEAP_FILE_ID));


    /* Initialize header's file metadata and store them AFTER heap-file's ID */
    dataPointer+=sizeof(HEAP_FILE_ID);

    HP_info fileMetaData;
    fileMetaData.fileDescriptor = -1;
    fileMetaData.blockIndex = 0;
    fileMetaData.lastBlock = 0;
    fileMetaData.maximumRecords = MAXIMUM_BLOCK_RECORDS;

    memcpy(dataPointer,&fileMetaData,sizeof(HP_info));


    /* Set the block as dirty, since you wrote the file's metadata into it */
    BF_Block_SetDirty(block);

    /* Unpin the block */
    code = BF_UnpinBlock(block);

    /* If unpin failed */
    if(code!=BF_OK)
    {

        BF_PrintError(code);

        /* Destroy the container block */
        BF_Block_Destroy(&block);

        /* Close the file */
        code = BF_CloseFile(fileDescriptor);
        if(code!=BF_OK)
            BF_PrintError(code);

        return HP_ERROR;
    }

    /* Destroy the block */
    BF_Block_Destroy(&block);

    /* Close the file */
    code = BF_CloseFile(fileDescriptor);
    if(code!=BF_OK)
    {
        BF_PrintError(code);
        return HP_ERROR;
    }


    return HP_OK;

}

/* OPEN THE HEAP-FILE AND READ ITS METADATA */
HP_info* HP_OpenFile(char *fileName)
{

    /* If no filename was given */
    if(fileName==NULL)
    {
        printf("No filename given!\n");
        return NULL;
    }

    /* Open the file */
    int fileDescriptor;
    int code = BF_OpenFile(fileName, &fileDescriptor);

    /* If file's opening failed */
    if(code!=BF_OK)
    {
        BF_PrintError(code);
        return NULL;
    }

    /* Create a container block */
    BF_Block* block;
    BF_Block_Init(&block);

    /* Get the header */
    code = BF_GetBlock(fileDescriptor, 0, block);

    /* If header's fetch failed */
    if(code!=BF_OK)
    {

        BF_PrintError(code);

        /* Destroy the container block */
        BF_Block_Destroy(&block);

        /* Close the file */
        code = BF_CloseFile(fileDescriptor);
        if(code != BF_OK)
            BF_PrintError(code);

        return NULL;
    }

    /* Go to block's start to check if the file has the heap-file ID */
    char* dataPointer = BF_Block_GetData(block);

    /* If the given file isn't a heap file, fail */
    if(memcmp(dataPointer,HEAP_FILE_ID,sizeof(HEAP_FILE_ID))!=0)
    {
        printf("The given file < %s > is NOT a heap-file!\n",fileName);

        /* Unpin the block since it's NOT a heap-file's header */
        code = BF_UnpinBlock(block);

        /* If unpin failed */
        if(code != BF_OK)
            BF_PrintError(code);

        /* Destroy the container block */
        BF_Block_Destroy(&block);

        /* Close the file */
        code = BF_CloseFile(fileDescriptor);

        /* If file close failed */
        if(code != BF_OK)
            BF_PrintError(code);

        return NULL;
    }

    /* Go to header's metadata, to update the file descriptor since the file was opened, and copy the metadata */
    dataPointer += sizeof(HEAP_FILE_ID);
    memcpy(dataPointer,&fileDescriptor, sizeof(int));

    HP_info* fileMetaData = (HP_info*) malloc(sizeof(HP_info));
    memcpy(fileMetaData,dataPointer,sizeof(HP_info));

    /* Set the header as dirty, since you updated its file descriptor */
    BF_Block_SetDirty(block);

    /* Unpin the header */
    code = BF_UnpinBlock(block);

    /* If header's unpin failed */
    if(code != BF_OK)
    {
        BF_PrintError(code);

        /* Destroy the container block */
        BF_Block_Destroy(&block);

        return fileMetaData;
    }


    return fileMetaData ;

}

/* UPDATE HEADER'S METADATA AND CLOSE THE HEAP-FILE */
int HP_CloseFile(HP_info* metaData)
{

    /* If not file reference was given */
    if(metaData==NULL)
    {
        printf("HP_info structure doesn't exist!\n");
        return HP_ERROR;
    }

    /* Create a container block */
    BF_Block *block;
    BF_Block_Init(&block);

    /* Get heap-file's header */
    int code = BF_GetBlock(metaData->fileDescriptor,0,block);

    /* If header's fetch failed */
    if(code != BF_OK)
    {
        BF_PrintError(code);

        /* Destroy the container block */
        BF_Block_Destroy(&block);

        return HP_ERROR;
    }

    /* Get header's metadata */
    char* dataPointer = BF_Block_GetData(block) + sizeof(HEAP_FILE_ID);
    HP_info fileMetaData;
    memcpy(&fileMetaData, dataPointer, sizeof(HP_info));


    /* If the last block of the header isn't valid anymore */
    if(fileMetaData.lastBlock != metaData->lastBlock)
    {

        /* Update header's metadata */
        fileMetaData.lastBlock = metaData->lastBlock;
        memcpy(dataPointer, &fileMetaData, sizeof(HP_info));

        /* Set the header as dirty, since you just updated its 'last block' value */
        BF_Block_SetDirty(block);


    }

    /* Unpin the header */
    code = BF_UnpinBlock(block);

    /* If header's unpin failed */
    if(code != BF_OK)
    {
        BF_PrintError(code);

        /* Destroy the container block */
        BF_Block_Destroy(&block);

        return HP_ERROR;
    }

    /* Destroy the container block */
    BF_Block_Destroy(&block);

    /* Close the file */
    code = BF_CloseFile(metaData->fileDescriptor);

    /* If file's close failed */
    if(code != BF_OK)
    {

        BF_PrintError(code);

        return HP_ERROR;
    }


    return HP_OK;
}

/* INSERT RECORD IN THE HEAP-FILE */
int HP_InsertEntry(HP_info* metaData, Record record)
{

    /* If no file reference was given */
    if(metaData==NULL)
    {
        printf("HP_info structure doesn't exist!\n");
        return HP_ERROR;
    }

    /* Create a container block */
    BF_Block* block;
    BF_Block_Init(&block);

    /* If the last block isn't the header */
    if(metaData->lastBlock != 0)
    {

        /* Load the last block */
        int code = BF_GetBlock(metaData->fileDescriptor,metaData->lastBlock,block);

        /* If the load of the last block failed */
        if(code != BF_OK)
        {
            BF_PrintError(code);

            /* Destroy the container block */
            BF_Block_Destroy(&block);

            return HP_ERROR;
        }

        /* Get block's metadata */
        char* dataPointer = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);
        HP_block_info blockMetaData;
        memcpy(&blockMetaData,dataPointer, sizeof(HP_block_info));

        /* If the last block has enough room for one more record */
        if(blockMetaData.totalRecords<MAXIMUM_BLOCK_RECORDS)
        {

            /* Increase last block's total records */
            blockMetaData.totalRecords += 1;
            memcpy(dataPointer,&blockMetaData, sizeof(HP_block_info));

            /* Insert the new record at the end of the records */
            dataPointer = BF_Block_GetData(block) + (blockMetaData.totalRecords-1)*sizeof(Record);
            memcpy(dataPointer,&record, sizeof(Record));

            /* Set the last block as dirty, since you changed its block metadata and added a new record into it */
            BF_Block_SetDirty(block);

            /* Unpin the last block */
            code = BF_UnpinBlock(block);

            /* If unpin of the last block failed */
            if(code != BF_OK)
            {

                BF_PrintError(code);

                /* Destroy the container block */
                BF_Block_Destroy(&block);

                return HP_ERROR;

            }

            /* Destroy the container block */
            BF_Block_Destroy(&block);
        }

        /* Otherwise, if the last block is full */
        else
        {

            /* Create a new container block */
            BF_Block* newBlock;
            BF_Block_Init(&newBlock);

            /* Attach the new block at the end of the file */
            code = BF_AllocateBlock(metaData->fileDescriptor, newBlock);

            /* If the attachment of the new, last block failed */
            if(code != BF_OK)
            {
                BF_PrintError(code);

                /* Destroy the new container block */
                BF_Block_Destroy(&newBlock);

                /* Unpin the previously-last block */
                code = BF_UnpinBlock(block);

                /* If unpin of the previously-last block failed */
                if(code != BF_OK)
                    BF_PrintError(code);

                /* Destroy the old container block */
                BF_Block_Destroy(&block);

                return HP_ERROR;
            }

            /* Update the 'last block' of the header */
            metaData->lastBlock += 1;

            /* Update the 'next block' of the previously-last block */
            blockMetaData.nextBlock = metaData->lastBlock;
            dataPointer = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);
            memcpy(dataPointer,&blockMetaData,sizeof(HP_block_info));

            /* Set the previously, last block as dirty, since you updated its 'next block' value */
            BF_Block_SetDirty(block);

            /* Initialize the metadata of the new, last block */
            dataPointer = BF_Block_GetData(newBlock) + BF_BLOCK_SIZE - sizeof(HP_block_info);
            blockMetaData.totalRecords = 1;
            blockMetaData.nextBlock = NONE;
            memcpy(dataPointer,&blockMetaData, sizeof(HP_block_info));

            /* Insert the new record at the start of the new, last block */
            dataPointer = BF_Block_GetData(newBlock);
            memcpy(dataPointer,&record, sizeof(Record));

            /* Set the new, last block as dirty, since you initialized its metadata and added a new record into it */
            BF_Block_SetDirty(newBlock);

            /* Unpin the previously-last block */
            code = BF_UnpinBlock(block);
            if(code != BF_OK)
            {
                BF_PrintError(code);

                /* Destroy the previously-last container block */
                BF_Block_Destroy(&block);

                /* Unpin the new, last block */
                code = BF_UnpinBlock(newBlock);
                if(code != BF_OK)
                    BF_PrintError(code);

                /* Destroy the new, last block */
                BF_Block_Destroy(&newBlock);

                return HP_ERROR;
            }

            /* Destroy the previously-last block */
            BF_Block_Destroy(&block);

            /* Unpin the new, last block */
            code = BF_UnpinBlock(newBlock);
            if(code != BF_OK)
            {
                BF_PrintError(code);

                /* Destroy the new, last block */
                BF_Block_Destroy(&newBlock);

                return HP_ERROR;
            }

            /* Destroy the new, last block */
            BF_Block_Destroy(&newBlock);

        }

    }

    /* Otherwise, if the only block is the header */
    else
    {

        /* Allocate a new block at the end of the file */
        int code = BF_AllocateBlock(metaData->fileDescriptor,block);

        /* If block's allocation failed */
        if(code != BF_OK)
        {
            BF_PrintError(code);

            /* Destroy the container block */
            BF_Block_Destroy(&block);

            return HP_ERROR;
        }

        /* Initialize the block metadata of the new block */
        char* dataPointer = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);
        HP_block_info blockMetaData;
        blockMetaData.totalRecords = 1;
        blockMetaData.nextBlock = NONE;
        memcpy(dataPointer,&blockMetaData,sizeof(HP_block_info));

        /* Insert the new record at the start of the new block */
        dataPointer = BF_Block_GetData(block);
        memcpy(dataPointer, &record, sizeof(Record));

        /* Set the new block as dirty, since you added the new record and the block metadata into it */
        BF_Block_SetDirty(block);

        /* Update the 'last block' of the header */
        metaData->lastBlock = 1;

        /* Unpin the new block */
        code = BF_UnpinBlock(block);

        /* If unpin of the new block failed */
        if(code != BF_OK)
        {
            BF_PrintError(code);

            /* Destroy the container block */
            BF_Block_Destroy(&block);

            return HP_ERROR;
        }

        /* Destroy the container block */
        BF_Block_Destroy(&block);

    }

    return HP_OK;

}

/* GET ALL ENTRIES OF THE HEAP-FILE , WHERE KEY = VALUE */
int HP_GetAllEntries(HP_info* metaData, int value)
{

    /* If no file reference was given */
    if(metaData==NULL)
    {
        printf("HP_info structure doesn't exist!\n");
        return HP_ERROR;
    }

    /* Allocate a container block */
    BF_Block *block;
    BF_Block_Init(&block);


    /* For each block in the heap-file ... */
    for(int i=1;i<=metaData->lastBlock;i++)
    {

        /* Get the block */
        int code = BF_GetBlock(metaData->fileDescriptor, i, block);

        /* If the block couldn't be fetched */
        if(code != BF_OK)
        {
            BF_PrintError(code);

            /* Destroy the container block */
            BF_Block_Destroy(&block);

            return  HP_ERROR;
        }

        /* Get the block's metadata to access its total records */
        char* dataPointer = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);
        HP_block_info blockMetaData;
        memcpy(&blockMetaData, dataPointer, sizeof(HP_block_info));

        /* For each record in the current block ... */
        dataPointer = BF_Block_GetData(block);
        Record record;
        for(int i=0;i<blockMetaData.totalRecords;i++)
        {

            /* Get the current record */
            memcpy(&record,dataPointer, sizeof(Record));

            /* If the record has the desired ID, print the record */
            if(record.id == value)
                printRecord(record);

            /* Move to the next record */
            dataPointer += sizeof(Record);

        }


        /* Unpin the current block before moving to the next block */
        code = BF_UnpinBlock(block);

        /* If unpin of the block failed */
        if(code != BF_OK)
        {

            BF_PrintError(code);

            /* Destroy the container block */
            BF_Block_Destroy(&block);

            return HP_ERROR;
        }

    }


    /* Destroy the block */
    BF_Block_Destroy(&block);

    /* All blocks except header are ALWAYS read , since a heap-file's records are randomly ordered */
    return metaData->lastBlock;
}

