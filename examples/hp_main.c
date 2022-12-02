#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "./../include/bf.h"
#include "./../include/hp_file.h"

#define RECORDS_NUM 10
#define FILE_NAME "data.txt"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }


int main()
{

  BF_Init(LRU);

  HP_CreateFile(FILE_NAME);
  HP_info* info = HP_OpenFile(FILE_NAME);

  Record record = {"a",1,"tim","pat","athens"};
  printf("Inserting entries ...\n");

  HP_InsertEntry(info, record );

  printf("Entry insertion completed ...\n");

  int id = rand() % RECORDS_NUM;
  printf("Searching for ID : %d\n",id);
  HP_GetAllEntries(info, id);

  HP_CloseFile(info);
  BF_Close();
}
