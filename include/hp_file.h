#ifndef HP_FILE_H
#define HP_FILE_H
#include <record.h>



/* Η δομή HP_info κρατάει μεταδεδομένα που σχετίζονται με το αρχείο σωρού */
typedef struct
{

    /* File's descriptor */
    int fileDescriptor;

    /* The index of the block this structure is stored at -- always 0 */
    int blockIndex;

    /* The index of the last block */
    unsigned int lastBlock;

    /* The maximum amount of records a NON-HEADER block can store */
    unsigned int maximumRecords;


} HP_info;



typedef struct
{

    /* The total records the current block stores */
    unsigned int totalRecords;

    /* The next block of the current block. If nextBlock = -1 , then there's no next block */
    int nextBlock;

} HP_block_info;


/*Η συνάρτηση HP_CreateFile χρησιμοποιείται για τη δημιουργία και
κατάλληλη αρχικοποίηση ενός άδειου αρχείου σωρού με όνομα fileName.
Σε περίπτωση που εκτελεστεί επιτυχώς, επιστρέφεται 0, ενώ σε
διαφορετική περίπτωση -1.*/

int HP_CreateFile(char* fileName);



/* Η συνάρτηση HP_OpenFile ανοίγει το αρχείο με όνομα filename και
διαβάζει από το πρώτο μπλοκ την πληροφορία που αφορά το αρχείο σωρού.
Κατόπιν, ενημερώνεται μια δομή που κρατάτε όσες πληροφορίες κρίνονται
αναγκαίες για το αρχείο αυτό προκειμένου να μπορείτε να επεξεργαστείτε
στη συνέχεια τις εγγραφές του.
*/

HP_info* HP_OpenFile(char *fileName);



/* Η συνάρτηση HP_CloseFile κλείνει το αρχείο που προσδιορίζεται
μέσα στη δομή header_info. Σε περίπτωση που εκτελεστεί επιτυχώς,
επιστρέφεται 0, ενώ σε διαφορετική περίπτωση -1. Η συνάρτηση είναι
υπεύθυνη και για την αποδέσμευση της μνήμης που καταλαμβάνει η δομή
που περάστηκε ως παράμετρος, στην περίπτωση που το κλείσιμο
πραγματοποιήθηκε επιτυχώς.
*/

int HP_CloseFile(HP_info* header_info);



/* Η συνάρτηση HP_InsertEntry χρησιμοποιείται για την εισαγωγή μιας
εγγραφής στο αρχείο σωρού. Οι πληροφορίες που αφορούν το αρχείο
βρίσκονται στη δομή header_info, ενώ η εγγραφή προς εισαγωγή
προσδιορίζεται από τη δομή record. Σε περίπτωση που εκτελεστεί
επιτυχώς, επιστρέφετε τον αριθμό του block στο οποίο έγινε η εισαγωγή
(blockId) , ενώ σε διαφορετική περίπτωση -1.
 */

int HP_InsertEntry(HP_info* header_info,Record record);



/*Η συνάρτηση αυτή χρησιμοποιείται για την εκτύπωση όλων των εγγραφών
που υπάρχουν στο αρχείο κατακερματισμού οι οποίες έχουν τιμή στο
πεδίο-κλειδί ίση με value. Η πρώτη δομή δίνει πληροφορία για το αρχείο
κατακερματισμού, όπως αυτή είχε επιστραφεί από την HP_OpenFile.
Για κάθε εγγραφή που υπάρχει στο αρχείο και έχει τιμή στο πεδίο id
ίση με value, εκτυπώνονται τα περιεχόμενά της (συμπεριλαμβανομένου
και του πεδίου-κλειδιού). Να επιστρέφεται επίσης το πλήθος των blocks που
διαβάστηκαν μέχρι να βρεθούν όλες οι εγγραφές. Σε περίπτωση επιτυχίας
επιστρέφει το πλήθος των blocks που διαβάστηκαν, ενώ σε περίπτωση λάθους επιστρέφει -1.
*/

int HP_GetAllEntries(HP_info* header_info, int id);


#endif
