#include "./../include/sht_file.h"



#define VALUE_CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);          \
      printf("Code is : %d\n", code);\
      return HT_ERROR;        \
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


/* Αυτη η συναρτηση δεχεται ως ορισμα ενα string και παραγει με βαση αυτο εναν ακεραιο αριθμο */
int SHT_hashfunction(char* key)
{
    int hashValue=0;
    for(int i=0;i<KEY_LENGTH;i++)
    hashValue += key[i];

    return (3*hashValue+15)/8;

}


int SHT_CreateSecondaryIndex(char *sfileName, Record_Attribute keyAttribute, int totalBuckets, char* fileName)
{

    /* Check if the total buckets are valid  */
    if (totalBuckets <= 0 || buckets > MAX_BUCKETS)
    {
        printf("Buckets should be in the following range : (0, %ld]\n", MAX_BUCKETS);
        return SHT_ERROR;
    }


    /* Φτιαξε το file */
    VALUE_CALL_OR_DIE(BF_CreateFile(sfileName))


    /* Ανοιξε το File */
    int fileDescriptor;
    VALUE_CALL_OR_DIE(BF_OpenFile(sfileName, &fileDescriptor))

    /* Φτιαξε το header block */
    BF_Block* block;
    BF_Block_Init(&block);
    VALUE_CALL_OR_DIE(BF_AllocateBlock(fileDescriptor, block))

    /* Αποθηκευσε το secondary hash file ID στο header */
    char* dataPointer = BF_Block_GetData();
    char shFileID = SECONDARY_HASH_FILE_IDENTIFIER;
    memcpy(dataPointer,&shFileID, sizeof(char));

    /* Αρχικοποιησε  και αντιγραψε τα metadata στο header */
    SHT_info fileMetaData;
    fileMetaData.fileDescriptor = -1;
    fileMetaData.totalBuckets = totalBuckets;
    for (int i = 0; i < MAX_BUCKETS; i++)
        fileMetaData.bucketToBlock[i] = NONE;
    fileMetaData.totalBlocks = 1;
    fileMetaData.totalRecords = 0;

    dataPointer += sizeof(char);
    memcpy(dataPointer,&fileMetaData, sizeof(SHT_info));

    /* Θεσε το header ως dirty, αφου το αρχικοποιησες με το ID και τα metadata του */
    BF_Block_SetDirty(block);

    /* Κανε unpin το header, και release το container block */
    VALUE_CALL_OR_DIE(BF_UnpinBlock(block))
    BF_Block_Destroy(&block);

    /* Κλεισε το αρχειο */
    VALUE_CALL_OR_DIE(BF_CloseFile(fileDescriptor))


    return SHT_OK;
}


SHT_info* SHT_OpenSecondaryIndex(char* sfileName)
{

    /* Ανοιξε το file */
    int fileDescriptor;
    POINTER_CALL_OR_DIE(BF_OpenFile(fileName, &fileDescriptor))

    /* Παρε το header */
    BF_Block *block;
    BF_Block_Init(&block);
    POINTER_CALL_OR_DIE(BF_GetBlock(fileDescriptor, HEADER_BLOCK, block))

    /* Πηγαινε στην αρχη του header, και παρε το ID του file */
    char* dataPointer = BF_Block_GetData(block);
    char shFileID;
    memcpy(&shFileID,dataPointer, sizeof(char));

    /* Αν το αρχειο που ανοιξαμε δεν ειναι secondary hash file */
    if(shFileID != SECONDARY_HASH_FILE_IDENTIFIER)
    {
        printf("The given file < %s > is NOT a secondary hash file!\n",sfileName);

        /* Κανε unpin και release το container block */
        POINTER_CALL_OR_DIE(BF_UnpinBlock(block))
        BF_Block_Destroy(&block);

        /* Κλεισε το αρχειο */
        POINTER_CALL_OR_DIE(BF_CloseFile(fileDescriptor))

        return NULL;
    }


    /* Αντιγραψε τα metadata του header, για να τα επιστρεψεις στο χρηστη */
    SHT_info* fileMetaData = (SHT_info*) malloc(sizeof(SHT_info));
    dataPointer += sizeof(char);
    memcpy(fileMetaData,dataPointer, sizeof(SHT_info));

    /* Κανε unpin το header, και release το container block του */
    POINTER_CALL_OR_DIE(BF_UnpinBlock(block))
    BF_Block_Destroy(&block);

    return fileMetaData;

}


int SHT_SecondaryInsertEntry(SHT_info* fileMetaData, Record record, int blockID)
{

    /* Βρες σε ποιο bucket πεφτει το συγκεκριμενο record, και το last block του bucket */
    int bucket = SHT_hashfunction(record.name) % fileMetaData->totalBuckets;
    int lastBlock = fileMetaData->bucketToBlock[bucket];

    /* Δομες για το record και τα metadata του block */
    SHT_block_info blockMetaData;
    SHT_Record secondaryRecord;

    BF_Block* block;
    BF_Block_Init(&block);

    /* Αν δεν εχει δημιουργηθει ακομα καποιο block για το συγκεκριμενο bucket */
    if(lastBlock == NONE)
    {

        /* Φτιαξε το πρωτο block του bucket, και πηγαινε στην αρχη του για να γραψεις τα metadata του */
        VALUE_CALL_OR_DIE(BF_AllocateBlock(fileDescriptor, block))
        char* dataPointer = BF_Block_GetData(block);

        /* Αρχικοποιησε τα metadata του block, και γραφ' τα στο block */
        blockMetaData.totalRecords = 1;
        blockMetaData.previousBlock = NONE;
        memcpy(dataPointer,blockMetaData, sizeof(SHT_block_info));

        /* Φτιαξε το secondary record, και περνα το στο block */
        memcpy(secondaryRecord.key,record.name, sizeof(KEY_LENGTH));
        secondaryRecord.blockID = blockID;

        dataPointer += sizeof(SHT_block_info);
        memcpy(dataPointer,secondaryRecord,sizeof(SHT_Record));


    }






}