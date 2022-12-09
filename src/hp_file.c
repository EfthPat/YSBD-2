#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/hp_file.h"


#define VALUE_CALL_OR_DIE(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}


#define POINTER_CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      return NULL;             \
    }                         \
  }


/*--------------------------------------------------------------------------------------------------------------------*/


int HP_CreateFile(char* fileName)
{

    /* Αν δεν δωθηκε καποιο filename */
    if(fileName==NULL)
    {
        printf("No filename given!\n");
        return HP_ERROR;
    }

    /* Φτιαξε το file */
    VALUE_CALL_OR_DIE(BF_CreateFile(fileName))


    /* Ανοιξε το file */
    int fileDescriptor;
    VALUE_CALL_OR_DIE(BF_OpenFile(fileName,&fileDescriptor))


    /* Φτιαξε ενα container-block */
    BF_Block *block;
    BF_Block_Init(&block);


    /* Φτιαξε το header του file */
    VALUE_CALL_OR_DIE(BF_AllocateBlock(fileDescriptor, block))


    /* Πηγαινε στην αρχη του header για να αποθηκευσεις το ID του heap file */
    char* dataPointer = BF_Block_GetData(block);
    char heapFileID = HEAP_FILE_IDENTIFIER;
    memcpy(dataPointer,&heapFileID,sizeof(char));


    /* Αρχικοποιησε τα metadata του heap file, και τοποθετησε τα ΜΕΤΑ το ID του */
    dataPointer+=sizeof(char);

    HP_info fileMetaData;
    fileMetaData.fileDescriptor = NONE;
    fileMetaData.blockIndex = HEADER_BLOCK;
    fileMetaData.lastBlock = 0;
    fileMetaData.maximumRecords = MAX_RECORDS;

    memcpy(dataPointer,&fileMetaData,sizeof(HP_info));


    /* TODO : UNIT TEST */
    dataPointer = BF_Block_GetData(block);
    char ID;
    memcpy(&ID,dataPointer, sizeof(char));
    HP_info meta;
    memcpy(&meta,dataPointer+sizeof(char),sizeof(HP_info));
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Heap file created\n");
    printf("ID : %c\n",ID);
    printf("File descriptor : %d\n",meta.fileDescriptor);
    printf("Block index : %d\n",meta.blockIndex);
    printf("Last block : %d\n",meta.lastBlock);
    printf("Max records : %d\n",meta.maximumRecords);
    /* TODO : UNTIL HERE */


    /* Θεσε το block ως dirty, αφου εβαλες το ID και τα metadata του */
    BF_Block_SetDirty(block);

    /* Κανε unpin το header και release το container block του */
    VALUE_CALL_OR_DIE(BF_UnpinBlock(block))
    BF_Block_Destroy(&block);

    /* Κλεισε το file */
    VALUE_CALL_OR_DIE(BF_CloseFile(fileDescriptor))


    return HP_OK;

}

HP_info* HP_OpenFile(char *fileName)
{

    /* Αν δεν δωθηκε καποιο filename */
    if(fileName==NULL)
    {
        printf("No filename given!\n");
        return NULL;
    }

    /* Ανοιξε το file */
    int fileDescriptor;
    POINTER_CALL_OR_DIE(BF_OpenFile(fileName, &fileDescriptor))

    /* Create a container block */
    BF_Block* block;
    BF_Block_Init(&block);

    /* Παρε το header */
    POINTER_CALL_OR_DIE(BF_GetBlock(fileDescriptor, HEADER_BLOCK, block))

    /* Πηγαινε στην αρχη του block για τον ελεγχο υπαρξης του heap file ID */
    char* dataPointer = BF_Block_GetData(block);

    /* Αν το file που δωθηκε προς ανοιγμα δεν ειναι heap file, fail */
    char heapFileID = HEAP_FILE_IDENTIFIER;
    if(memcmp(dataPointer,&heapFileID,sizeof(char))!=0)
    {
        printf("The given file < %s > is NOT a heap-file!\n",fileName);

        POINTER_CALL_OR_DIE(BF_UnpinBlock(block))

        BF_Block_Destroy(&block);

        POINTER_CALL_OR_DIE(BF_CloseFile(fileDescriptor))


        return NULL;
    }

    /* Κανε update τον παλιο file descriptor του block */
    dataPointer += sizeof(char);
    memcpy(dataPointer,&fileDescriptor, sizeof(int));

    /* Αντιγραψε τα metadata του header στη δομη του χρηστη */
    HP_info* fileMetaData = (HP_info*) malloc(sizeof(HP_info));
    memcpy(fileMetaData,dataPointer,sizeof(HP_info));


    /* TODO : UNIT TEST */
    dataPointer = BF_Block_GetData(block);
    char ID;
    memcpy(&ID,dataPointer, sizeof(char));
    HP_info meta;
    memcpy(&meta,dataPointer+sizeof(char),sizeof(HP_info));
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Heap file opened\n");
    printf("ID : %c\n",ID);
    printf("File descriptor : %d\n",meta.fileDescriptor);
    printf("Block index : %d\n",meta.blockIndex);
    printf("Last block : %d\n",meta.lastBlock);
    printf("Max records : %d\n",meta.maximumRecords);
    printf("#########################\n");
    printf("File descriptor : %d\n",fileMetaData->fileDescriptor);
    printf("Block index : %d\n",fileMetaData->blockIndex);
    printf("Last block : %d\n",fileMetaData->lastBlock);
    printf("Max records : %d\n",fileMetaData->maximumRecords);
    /* TODO : UNTIL HERE */


    /* Θεσε το header ως dirty, αφου μολις εκανες update τον file descriptor του */
    BF_Block_SetDirty(block);

    /* Κανε unpin το header, και release το container block */
    POINTER_CALL_OR_DIE(BF_UnpinBlock(block))
    BF_Block_Destroy(&block);


    return fileMetaData ;

}

int HP_CloseFile(HP_info* metaData)
{

    if(metaData==NULL)
    {
        printf("HP_info structure doesn't exist!\n");
        return HP_ERROR;
    }


    BF_Block *block;
    BF_Block_Init(&block);


    /* Πηγαινε στο header για να παρεις τα metadata του */
    VALUE_CALL_OR_DIE(BF_GetBlock(metaData->fileDescriptor,HEADER_BLOCK,block))
    char* dataPointer = BF_Block_GetData(block) + sizeof(char);
    HP_info fileMetaData;
    memcpy(&fileMetaData, dataPointer, sizeof(HP_info));


    /* Αν το last-block εχει πλεον αλλαξει, το header πρεπει να γινει update */
    if(fileMetaData.lastBlock != metaData->lastBlock)
    {

        fileMetaData.lastBlock = metaData->lastBlock;
        fileMetaData.fileDescriptor = NONE;
        memcpy(dataPointer, &fileMetaData, sizeof(HP_info));

        /* Θεσε το header ως dirty, αφου το last-block του εγινε update */
        BF_Block_SetDirty(block);

    }

    /* TODO : UNIT TEST */
    dataPointer = BF_Block_GetData(block);
    char ID;
    memcpy(&ID,dataPointer, sizeof(char));
    HP_info meta;
    memcpy(&meta,dataPointer+sizeof(char),sizeof(HP_info));
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Heap file closed\n");
    printf("ID : %c\n",ID);
    printf("File descriptor : %d\n",meta.fileDescriptor);
    printf("Block index : %d\n",meta.blockIndex);
    printf("Last block : %d\n",meta.lastBlock);
    printf("Max records : %d\n\n",meta.maximumRecords);
    /* TODO : UNTIL HERE */


    VALUE_CALL_OR_DIE(BF_UnpinBlock(block))
    BF_Block_Destroy(&block);

    VALUE_CALL_OR_DIE(BF_CloseFile(metaData->fileDescriptor))

    free(metaData);

    return HP_OK;
}

int HP_InsertEntry(HP_info* metaData, Record record)
{

    if(metaData==NULL)
    {
        printf("HP_info structure doesn't exist!\n");
        return HP_ERROR;
    }


    BF_Block* block;
    BF_Block_Init(&block);

    /* Αν το file περιεχει κι αλλα blocks περαν του header */
    if(metaData->lastBlock != 0)
    {

        /* Πηγαινε στο τελευταιο block και παρε τα metadata του */
        VALUE_CALL_OR_DIE(BF_GetBlock(metaData->fileDescriptor,metaData->lastBlock,block))
        char* dataPointer = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);
        HP_block_info blockMetaData;
        memcpy(&blockMetaData,dataPointer, sizeof(HP_block_info));

        /* Αν το τελευταιο block εχει αρκετο χωρο για ακομη ενα record */
        if(blockMetaData.totalRecords<MAX_RECORDS)
        {

            /* TODO : DELETE */
            int previousRecords = blockMetaData.totalRecords;
            /* TODO : END */


            /* Αυξησε τα συνολικα records του τελευταιου block, και βαλε το νεο record στο τελος των records */
            blockMetaData.totalRecords += 1;
            memcpy(dataPointer,&blockMetaData, sizeof(HP_block_info));
            dataPointer = BF_Block_GetData(block) + (blockMetaData.totalRecords-1)*sizeof(Record);
            memcpy(dataPointer,&record, sizeof(Record));

            /* Θεσε το τελευταιο block ως dirty, αφου εβαλες το νεο record σε αυτο, και εκανες update τα metadata του */
            BF_Block_SetDirty(block);

            /* TODO : DELETE */
            /* After record's insertion, new records should be previous records + 1, AND new record should exist at the end of the records */
            dataPointer = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);
            memcpy(&blockMetaData,dataPointer,sizeof(HP_block_info));
            if(previousRecords+1!=blockMetaData.totalRecords)
            {
                printf("FAIL : new records should be previous records + 1\n");
                return -1;
            }
            Record newRecord;
            dataPointer = BF_Block_GetData(block) + (blockMetaData.totalRecords-1)*sizeof(Record);
            memcpy(&newRecord,dataPointer,sizeof(Record));
            if(memcmp(&newRecord,dataPointer,sizeof(Record))!=0)
            {
                printf("FAIL : Records dont match\n");
                return -1;
            }
            /* TODO : END */



            /* Κανε unpin το τελευταιο block και release το container block */
            VALUE_CALL_OR_DIE(BF_UnpinBlock(block))
            BF_Block_Destroy(&block);





        }

        /* Αλλιως, αν το τελευταιο block ειναι γεματο */
        else
        {

            /* Βαλε ενα καινουριο block στο τελος του αρχειου, οπου και θα αποθηκευτει το νεο record */
            BF_Block* newBlock;
            BF_Block_Init(&newBlock);
            VALUE_CALL_OR_DIE(BF_AllocateBlock(metaData->fileDescriptor, newBlock))

            /* TODO : DELETE */
            int previousLastBlock = metaData->lastBlock;
            /* TODO : HERE */

            /* Κανε update το last block του header */
            metaData->lastBlock += 1;

            /* Κανε update το last block του πρωην last block */
            blockMetaData.nextBlock = metaData->lastBlock;
            dataPointer = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);
            memcpy(dataPointer,&blockMetaData,sizeof(HP_block_info));

            /* Θεσε το πρωην last block ως dirty, αφου εκανες update την 'last block' τιμη του */
            BF_Block_SetDirty(block);


            /* Αρχικοποιησε και τοποθετησε τα metadata του νεου last block στο τελος του */
            dataPointer = BF_Block_GetData(newBlock) + BF_BLOCK_SIZE - sizeof(HP_block_info);
            blockMetaData.totalRecords = 1;
            blockMetaData.nextBlock = NONE;
            memcpy(dataPointer,&blockMetaData, sizeof(HP_block_info));

            /* Βαλε το νεο record στην αρχη του */
            dataPointer = BF_Block_GetData(newBlock);
            memcpy(dataPointer,&record, sizeof(Record));



            /* Θεσε το last block ως dirty, αφου εβαλες τα metadata και το νεο record σε αυτο  */
            BF_Block_SetDirty(newBlock);

            /* TODO : DELETE */
            /* after record's insertion, last header's block should be previous last block + 1 and the new block should have the new record */
            if(previousLastBlock+1!=metaData->lastBlock)
            {
                printf("FAIL : header's last block should be previous block + 1\n");
                return -1;
            }
            dataPointer = BF_Block_GetData(newBlock);
            Record newRecord;
            memcpy(&newRecord,dataPointer, sizeof(Record));
            if(memcmp(&newRecord,&record, sizeof(Record))!=0)
            {
                printf("FAIL : records dont match\n");
                return -1;
            }
            dataPointer += BF_BLOCK_SIZE - sizeof(HP_block_info);
            memcpy(&blockMetaData,dataPointer, sizeof(HP_block_info));
            if(blockMetaData.totalRecords!=1)
            {
                printf("FAIL:last block's total records should be 1\n");
                return -1;
            }
            if(blockMetaData.nextBlock!=NONE)
            {
                printf("FAIL: last block's next block should be NONE\n");
                return -1;
            }

            /* TODO : HERE */

            /* Κανε unpin το πρωην last block, και destroy το container block του */
            VALUE_CALL_OR_DIE(BF_UnpinBlock(block))
            BF_Block_Destroy(&block);

            /* Κανε unpin το νεο last block, και destroy το container block του */
            VALUE_CALL_OR_DIE(BF_UnpinBlock(newBlock))
            BF_Block_Destroy(&newBlock);



        }

    }


    /* Αν ομως το header ειναι το μοναδικο block στο αρχειο */
    else
    {

        /* Φτιαξε ενα νεο block στο τελος του αρχειου */
        VALUE_CALL_OR_DIE(BF_AllocateBlock(metaData->fileDescriptor,block))


        /* Αρχικοποιησε τα block-metadata του νεου block, και βαλ´τα στο τελος του block */
        char* dataPointer = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);
        HP_block_info blockMetaData;
        blockMetaData.totalRecords = 1;
        blockMetaData.nextBlock = NONE;
        memcpy(dataPointer,&blockMetaData,sizeof(HP_block_info));

        /* Βαλε το νεο record στην αρχη του block */
        dataPointer = BF_Block_GetData(block);
        memcpy(dataPointer, &record, sizeof(Record));

        /* Θεσε το νεο block ως dirty, αφου του προσθεσες το νεο record και τα metadata του */
        BF_Block_SetDirty(block);

        /* Κανε update το last-block του header */
        metaData->lastBlock = 1;

        /* TODO : DELETE */

        dataPointer = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);
        memcpy(dataPointer,&blockMetaData,sizeof(HP_block_info));
        if(blockMetaData.totalRecords!=1)
        {
            printf("FAIL : records of last block should be 1\n");
            return -1;
        }
        if(blockMetaData.nextBlock!=NONE)
        {
            printf("FAIL : next block of last block should be NONE\n");
            return -1;
        }
        if(metaData->lastBlock!=1)
        {
            printf("FAIL : last block of header should be 1\n");
            return -1;
        }

        /* TODO : HERE */

        /* Κανε unpin το τελευταιο block, και release το container block */
        VALUE_CALL_OR_DIE(BF_UnpinBlock(block))
        BF_Block_Destroy(&block);

    }

    return HP_OK;

}

int HP_GetAllEntries(HP_info* metaData, int value)
{

    if(metaData==NULL)
    {
        printf("HP_info structure doesn't exist!\n");
        return HP_ERROR;
    }

    BF_Block *block;
    BF_Block_Init(&block);


    /* Για καθε block στο heap-file ... */
    for(int i=1;i<=metaData->lastBlock;i++)
    {

        /* Παρε το τρεχον block */
        VALUE_CALL_OR_DIE(BF_GetBlock(metaData->fileDescriptor, i, block))

        /* Βρες τα συνολικα records του block */
        char* dataPointer = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);
        HP_block_info blockMetaData;
        memcpy(&blockMetaData, dataPointer, sizeof(HP_block_info));

        /* Για καθε record στο τρεχον block */
        dataPointer = BF_Block_GetData(block);
        Record record;
        for(int i=0;i<blockMetaData.totalRecords;i++)
        {

            /* Παρε το τρεχον record */
            memcpy(&record,dataPointer, sizeof(Record));

            /* Αν το record εχει το ID που ψαχνουμε, τυπωσε το */
            if(record.id == value)
                printRecord(record);

            /* Προχωρα στο επομενο record */
            dataPointer += sizeof(Record);
        }


        /* TODO : DELETE */
        if(i<metaData->lastBlock && blockMetaData.totalRecords!=MAX_RECORDS)
        {
            printf("FAIL : block isn't full\n");
            return -1;
        }
        /* TODO : HERE */

        /* Κανε unpin το τρεχον block πριν πας στο επομενο */
        VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

    }


    BF_Block_Destroy(&block);

    /* Στο heap-file διαβαζονται ολα τα blocks εγγραφων (εκτος του header), αφου οι εγγραφες του δεν εχουν διαταξη */
    return metaData->lastBlock;
}

/* CUSTOM FUNCTION */
void printStatistics(char* filename)
{


    HP_info* metaData = HP_OpenFile(filename);
    if(metaData==NULL)
    {
        printf("FAIL!\n");
        return;
    }


    BF_Block *block;
    BF_Block_Init(&block);


    for(int i=1;i<=metaData->lastBlock;i++)
    {



        BF_GetBlock(metaData->fileDescriptor, i, block);


        char* dataPointer = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);
        HP_block_info blockMetaData;
        memcpy(&blockMetaData, dataPointer, sizeof(HP_block_info));


        printf("---------------------------------------------------------\n");
        printf("Block : %d --- Total Records : %d \n",i,blockMetaData.totalRecords);

        dataPointer = BF_Block_GetData(block);
        Record record;
        for(int i=0;i<blockMetaData.totalRecords;i++)
        {


            memcpy(&record,dataPointer, sizeof(Record));
            printf("Record : %d --- ",i+1);
            printRecord(record);

            dataPointer += sizeof(Record);
        }




        BF_UnpinBlock(block);

    }


    BF_Block_Destroy(&block);


    HP_CloseFile(metaData);

    return;
}







