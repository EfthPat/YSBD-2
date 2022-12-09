#ifndef HEAP_SHT_FILE_H
#define HEAP_SHT_FILE_H

#include "./record.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <bf.h>

#define SHT_OK 0
#define SHT_ERROR (-1)

#define NONE (-1)

#define HEADER_BLOCK 0

#define KEY_LENGTH 15

#define SECONDARY_HASH_FILE_IDENTIFIER 'S'

#define MAX_BUCKETS ((BF_BLOCK_SIZE-4*sizeof(int)-sizeof(char))/(2*sizeof(int)))

/* Η δομη αυτη αποθηκευεται στο header, και περιεχει πληροφοριες σχετικα με το secondary hash file */
typedef struct
{

    int fileDescriptor;
    int totalBuckets;
    int totalBlocks;
    int totalRecords;
    int bucketToBlock[MAX_BUCKETS];

} SHT_info;


/* Η δομη αυτη αποθηκευεται τοπικα σε καθε block */
typedef struct
{

    int totalRecords;
    int previousBlock;

} SHT_block_info;

typedef struct
{
    /* To key με βαση το οποιο γινεται hash η εγγραφη */
    char key[KEY_LENGTH];
    /* Σε ποιο block του PRIMARY hash file βρισκεται το record */
    int blockID;

}SHT_Record;

int SHT_CreateSecondaryIndex(char *sfileName, Record_Attribute keyAttribute, int totalBuckets, char* fileName);


SHT_info* SHT_OpenSecondaryIndex(char* sfileName);



int SHT_SecondaryInsertEntry(SHT_info* header_info, Record record, int block_id);











#endif
