#include <stdlib.h>
#include "./../include/bf.h"
#include "./../include/hp_file.h"

#define RECORDS_NUM 20
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


  //printf("Inserting %d entries ...\n",RECORDS_NUM);

  for(int i =0 ; i<RECORDS_NUM;i++)
  {
      /* Φτιαξε το record */
      Record record = randomRecord();
      /* Βαλε το record */
      HP_InsertEntry(info, record );

  }

  //printf("Entry insertion completed ...\n");

  /* Φτιαξε ενα τυχαιο ID και ψαξε γι' αυτο */
  int id = rand() % RECORDS_NUM;
  //printf("Searching for ID : %d\n",id);
  HP_GetAllEntries(info, id);

  /* Κλεισε το heap file */
  //printf("Closing heap file ...\n");
  HP_CloseFile(info);
  printStatistics(FILE_NAME);
  //printf("Heap file closed ...\n");
  BF_Close();
}
