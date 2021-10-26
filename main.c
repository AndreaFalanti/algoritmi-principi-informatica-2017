/*  Progetto API anno 2017/2018: Macchina di Turing non deterministica
 *  Autore: Andrea Falanti (2° scaglione ing. informatica) */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
/*#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <stddef.h>*/

#define REALLOC_LENGTH 3

int AT_LEAST_ONE_MT_LOOPED = 0;
unsigned short int gAcceptationStatesCount = 0;
int AS_SORTED = 1;
unsigned short int gSinkCount = 0;

typedef struct Transition {
    unsigned int state_i;                       //stato iniziale
    char symbol_r;                              //simbolo letto
    char symbol_w;                              //simbolo scritto
    char head_movement;                         //movimento testina
    unsigned int state_f;                       //stato finale della transizione
    struct Transition *next;
} Transition;

typedef struct InputString {
    char *input;                                //puntatore alla stringa di input (allocata in heap)
    unsigned int inputSize;                     //dimensioni stringa di input (# char)
    struct InputString *next;
} InputString;

typedef struct CoWInputString {
    char *input;                                //puntatore alla stringa di input (allocata in heap)
    unsigned int inputSize;                     //dimensioni stringa di input (# char)
    unsigned int MTCount;                       //contatore di MT che condividono la stringa
} CoWInputString;

Transition* CreateTransition (unsigned int s_i, char sy_r, char sy_f, char h_mov, unsigned int s_f) {
    Transition *el;
    el = malloc(sizeof(Transition));

    if (el != NULL) {
        el -> state_i = s_i;
        el -> symbol_r = sy_r;
        el -> symbol_w = sy_f;
        el -> head_movement = h_mov;
        el -> state_f = s_f;
        el -> next = NULL;
        return el;
    }
    else {
        printf("CRITICAL ERROR: allocation failed, out of memory");
        return NULL;
    }
}

InputString* CreateInputString (char *string, unsigned int size) {
    InputString *el;
    el = malloc(sizeof(InputString));

    if (el != NULL) {
        el -> input = string;
        el -> inputSize = size;
        el -> next = NULL;
        return el;
    }
    else {
        printf("CRITICAL ERROR: allocation failed, out of memory");
        return NULL;
    }
}

CoWInputString* CreateCoWInputString (char *string, unsigned int size, unsigned int MTnum) {
    CoWInputString *el;
    el = malloc(sizeof(CoWInputString));

    if (el != NULL) {
        el -> input = string;
        el -> inputSize = size;
        el -> MTCount = MTnum;
        return el;
    }
    else {
        printf("CRITICAL ERROR: allocation failed, out of memory");
        return NULL;
    }
}

void HeadInsertTransitionList (Transition **l, Transition *el ) {
    if (*l != NULL) {
        el -> next = *l;
        *l = el;
    }
    else {
        *l = el;
    }
}

void HeadInsertInputStringList (InputString **l, InputString *el ) {
    if (*l != NULL) {
        el -> next = *l;
        *l = el;
    }
    else {
        *l = el;
    }
}

void TailInsertInputStringList (InputString **l, InputString *el ) {
    InputString *temp;
    temp = *l;

    if (temp == NULL) {
        HeadInsertInputStringList(l, el);       //se la lista è vuota, allora è uguale a fare un inserimento in testa
    }
    else {
        while (temp -> next != NULL) {
            temp = temp -> next;
        }
        temp -> next = el;
    }
}

//inserimento "ordinato" a blocchi in base al carattere letto nella transizione, se un elemento ha stesso simbolo di
//uno già presente si inserisce davanti ad esso. Scandisce la coda e se non si presenta la condizione precedente,
//allora l'elemento è inserito in coda.
void SortedInsertTransitionList (Transition **l, Transition *el ) {
    Transition *temp;
    Transition *prev;
    short int headIns = 1;          //boolean per controllare se l'elemento è inserito in testa

    temp = *l;
    prev = temp;            //non necessario ma almeno l'IDE non da uno warning a caso

    //controlla prima se temp = NULL, nel caso non va in errore perchè non eseguirà la seconda condizione
    while (temp != NULL && temp -> symbol_r != el -> symbol_r) {
        prev = temp;                                                    //tiene il valore di temp precedente
        temp = temp -> next;
        headIns = 0;
    }

    if (headIns == 0) {
        el -> next = temp;
        prev -> next = el;
    }
    else {
        HeadInsertTransitionList(l, el);
    }
}

unsigned int StringToIntConversion (const char *string) {
    int i = 0;
    unsigned int value = 0;
    while (string[i] >= 48 && string[i] <= 57) {      //se il char è un numero (0,1,...,9), attua una conversione
        //value*10 poichè essendo letto da sx a dx, deve tenere conto del reale "peso" della cifra precedente
        value = value*10 + string[i] - 48;
        i++;
    }
    return value;
}

int ReadStream(Transition **t, unsigned int **acceptationStates, unsigned long int *max, InputString **input, int *maxState){
    //printf("Start parsing of stream...\n\n");
    char str[6];

    scanf("%s", str);
    //inserimento funzioni di transizione in lista apposita
    if (strcmp(str, "tr") == 0) {
        char s_f[6], sy_r[2], sy_w[2], h_mov[2];
        Transition *el;
        unsigned int state_i, state_f;

        scanf("%s", str);
        do {
            scanf("%s", sy_r);
            scanf("%s", sy_w);
            scanf("%s", h_mov);
            scanf("%s", s_f);

            state_i = StringToIntConversion(str);
            state_f = StringToIntConversion(s_f);

            if (state_i > *maxState) {      //ritornerà al main il valore max degli stati delle transizioni
                *maxState = state_i;
            }
            else if (state_f > *maxState) {
                *maxState = state_f;
            }

            el = CreateTransition(state_i, sy_r[0], sy_w[0], h_mov[0], state_f);
            HeadInsertTransitionList(t, el);
            scanf("%s", str);
        } while (strcmp(str, "acc") != 0);   //inserisce le transizioni nell'array fino a quando non incontra "acc"
    }

    //inserimento stati di accettazione in lista apposita
    if (strcmp(str, "acc") == 0) {
        /* array dinamico, poi da ordinare con quickSort
         * più rapida ricerca nella simulazione se gli stati di accettazione sono un numero elevato*/

        unsigned int *accStates = NULL;
        unsigned short int reallocCount = 4;
        unsigned int last = 0;

        accStates = malloc (16 * sizeof(unsigned int));
        scanf("%s", str);
        do {
            if (gAcceptationStatesCount == pow(2, reallocCount)) {
                reallocCount++;
                accStates = realloc(accStates, (int) pow(2, reallocCount) * sizeof(unsigned int));
                if (accStates == NULL) {
                    printf("CRITICAL ERROR: reallocation failed (1)");
                    return 1;
                }
            }

            unsigned int value = 0;
            value = StringToIntConversion(str);
            scanf("%s", str);

            accStates[gAcceptationStatesCount] = value;
            gAcceptationStatesCount++;       //var globale che tiene il conto del numero di stati di accettazione

            if (AS_SORTED == 1) {            //controlla solo se l'array è fino ad adesso ordinato
                if (value < last) {         //se un elemento è minore del precedente allora l'array non è ordinato
                    AS_SORTED = 0;
                }
                last = value;
            }

        } while (strcmp(str, "max") != 0);   //inserisce le transizioni nell'array fino a quando non incontra "max"

        accStates = realloc(accStates, gAcceptationStatesCount * sizeof(unsigned int));//realloca alla dimensione esatta
        if (accStates == NULL) {
            printf("CRITICAL ERROR: reallocation failed (2)");
            return 1;
        }
        *acceptationStates = accStates;
    }

    //inserimento massimo numero di transizioni in valore apposito
    if (strcmp(str,"max") == 0) {
        scanf("%li", max);
        scanf("%s", str);               //controlla la prossima linea
    }

    //inserimento stringhe di input in lista apposita
    if (strcmp(str,"run") == 0) {
        char c;
        InputString *el;

        c = getc(stdin);                    //serve a leggere '^M' inserito dalla cat di Linux
        //c = getc(stdin);                    //legge '/n'
        while (c != EOF) {
            unsigned int charNum = 0;
            unsigned short int reallocCount = 7;
            char* iStr;

            iStr = malloc (128 * sizeof(char));      //parte da dimensione 8 char (2^3)
            do {
                c = getc(stdin);

                if (c != '\n' && c != EOF && (c > 37)) {          //(0-37) sono caratteri di controllo da escludere
                    iStr[charNum] = c;
                    charNum++;

                    //realloca iStr per potenze di 2^k (k appartiene a N) così da ridurre operazioni e frammentazioni
                    //della memoria
                    if (charNum >= (pow(2, reallocCount))) {
                        reallocCount++;
                        iStr = realloc(iStr, (int) pow(2, reallocCount) * sizeof(char));
                        if (iStr == NULL) {
                            printf("CRITICAL ERROR: reallocation failed (3)");
                            return 1;
                        }
                    }
                }

                //caso stringa di input terminata ('\n' o EOF)
                else {
                    if (charNum > 0) {
                        iStr = realloc(iStr, charNum * sizeof(char));       //realloca alla dimensione esatta
                        if (iStr == NULL) {
                            printf("CRITICAL ERROR: reallocation failed (4)");
                            return 1;
                        }

                        el = CreateInputString(iStr, charNum);
                        TailInsertInputStringList(input, el);           //preserva l'ordine degli input inserendo in coda
                    }
                    else {                                                  //liberata solo se vuota
                        free(iStr);
                    }
                }
            } while (c != '\n' && c != EOF);
        }
    }
    return 0;

    //printf ("Parsing complete.\n");
}

Transition** CreateTransitionAdjacencyList (int stateNum, Transition *t) {
    Transition **l;
    //alloca un array di puntatori ad elementi della lista di adiacenza del grafo
    l = malloc(stateNum * sizeof(Transition*));

    int k ;
    for (k = 0; k < stateNum; k++) {
        l[k] = NULL;   //inizializza tutti i pointer a NULL, così da non creare problemi con le funzioni di inserimento
    }

    Transition *temp;

    if (l != NULL) {
        //inserisce ogni transizione in una lista di adiacenza del grafo, in modo tale da essere
        //indirizzata più velocemente durante l'esecuzione dell'algoritmo della MT, poichè l'indice dell'array
        //corrisponderà esattamente al numero dello stato da cui la transizione è definita.
        //Inoltre la ordina raggruppando le transizioni per elemento letto.
        while (t != NULL) {
            temp = t -> next;
            t -> next = NULL;
            SortedInsertTransitionList(&l[t -> state_i], t);
            t = temp;
        }
        return l;
    }
    else {
        printf("CRITICAL ERROR: allocation failed, out of memory");
        return NULL;
    }
}

unsigned int* CheckSinkStates (Transition **t, int maxState) {
    unsigned int *sinkStates;
    sinkStates = malloc((maxState + 1) * sizeof(unsigned int));
    int isSink;
    for (unsigned int i = 0; i <= maxState; i++) {
        Transition *temp;
        temp = t[i];
        isSink = 1;

        if (temp == NULL) {           //se non ci sono transizioni, allora non è un pozzo, ma uno stato di accettazione
            isSink = 0;
        }

        while (temp != NULL) {
            if (isSink == 1 && temp -> state_i != temp -> state_f) {
                isSink = 0;                     //se la condizione è soddisfatta, allora non è un pozzo
            }
            temp = temp -> next;
        }
        if (isSink == 1) {
            sinkStates[gSinkCount] = i;          //sono anche già ordinati in ordine crescente
            gSinkCount++;
        }
    }

    sinkStates = realloc(sinkStates, gSinkCount * sizeof(unsigned int));
    return sinkStates;
}

//------------------------    QUICKSORT (versione slide corso API)   ----------------------------------
void Swap (unsigned int *a, unsigned int *b) {
    unsigned int temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

int Partition (unsigned int *array, int p, int r) {
    unsigned int x = array[r];
    int i = p - 1;
    for (int j = p; j <= r - 1; j++) {
        if (array[j] <= x) {
            i++;
            if (i != j) {
                Swap (&array[i], &array[j]);
            }
        }
    }
    Swap (&array[i+1], &array[r]);
    return i+1;
}

void QuickSort (unsigned int *array , int p, int r) {
    if (p < r) {
        int q = Partition(array, p, r);
        QuickSort (array, p, q - 1);
        QuickSort (array, q + 1, r);
    }
}
//-------------------------------------------------------------------------------------------------

//ritorna 1 se state è uno stato di accettazione, 0 altrimenti
int IsAcceptationState (const unsigned int *array, const unsigned int state) {
    //ricerca binaria
    int i = 0;
    int f = gAcceptationStatesCount - 1;
    while (i <= f) {
        int c = (i + f)/2;
        if (array[c] == state) {
            return 1;
        }
        else if (array[c] < state) {
            i = c + 1;
        }
        else {
            f = c - 1;
        }
    }
    return 0;
}

//ritorna 1 se state è uno stato di accettazione, 0 altrimenti
int IsSinkState (const unsigned int *array, const unsigned int state) {
    //ricerca binaria
    int i = 0;
    int f = gSinkCount - 1;
    while (i <= f) {
        int c = (i + f)/2;
        if (array[c] == state) {
            return 1;
        }
        else if (array[c] < state) {
            i = c + 1;
        }
        else {
            f = c - 1;
        }
    }
    return 0;
}

//se la condizione è soddisfatta, le due transizioni verranno eseguite in loop fino a max, la MT può restituire 2
int CheckIfTransitionsLeadToLoop (const Transition *tExe, const Transition *tPrev) {
    if (tExe != NULL && tPrev != NULL) {
        if (tExe -> state_i == tPrev -> state_f && tPrev -> state_i == tExe -> state_f
            && tExe -> symbol_r == tExe -> symbol_w && tPrev -> symbol_r == tPrev -> symbol_w
            && ((tExe -> head_movement == 'L' && tPrev -> head_movement == 'R')||(tExe -> head_movement == 'R'
            && tPrev -> head_movement == 'L')) ) {
                return 1;
        }
    }
    return 0;
}

//controlla se la MT supera i passi massimi consentiti
int CheckIfReachLoopStatus (const long int max, const unsigned long int steps) {
    if (max < steps) {
        AT_LEAST_ONE_MT_LOOPED = 1;
        return 1;
    }
    return 0;
}

void ExecuteTransition (const Transition *t, char *string, unsigned long int *steps, int *headPos,
                        unsigned int *state) {
    string[*headPos] = t -> symbol_w;
    *state = t -> state_f;

    switch(t -> head_movement) {
        case 'R':
            *headPos = *headPos + 1;
            break;
        case 'L':
            *headPos = *headPos - 1;
            break;
        case 'S':
            break;
        default:
            printf("CRITICAL ERROR: wrong head movement in transition executed");
    }

    *steps = *steps + 1;
}

//quando una MT termina, decrementa il conto delle MT condivise e se nullo, libera sia la stringa che la struct
void CoWCountDecrease (CoWInputString *p) {
    p -> MTCount -= 1;
    if (p -> MTCount == 0) {
        free(p -> input);
        free(p);
    }
}

//alla prima write (s_r != s_w) se la stringa è condivisa, la sdoppia (meccanismo della Copy on Write)
CoWInputString* CoWCopy (CoWInputString *p) {
    char *input;
    input = malloc(p -> inputSize * sizeof(char));
    memcpy(input, p -> input, p -> inputSize * sizeof(char));
    CoWInputString *el;
    el = CreateCoWInputString(input, p -> inputSize, 1);
    p -> MTCount -= 1;                                 //decrementa il contatore poichè la MT corrente punterà a el

    return el;
}

//--------------------------- DEBUGGING FUNCTIONS ------------------------------------------------------------

void PrintInput (InputString el) {
    //printf("\n");
    if (el.input != NULL) {
        for(int k=0; k<el.inputSize; k++)
            printf("%c", el.input[k]);
        printf("\ninputSize: %d", el.inputSize);
    }
    else {
        printf("NULL");
    }
}

void PrintAccStates (unsigned int *array) {
    printf("\n--------- Acceptation States: --------------\n");
    for (int i=0; i < gAcceptationStatesCount; i++) {
        printf("%d\n", array[i]);
    }
    printf("Acceptation states count: %d", gAcceptationStatesCount);
    printf("\n---------------------------------------------\n");
}

void PrintSinkStates (unsigned int *array) {
    printf("\n--------- Sink States: --------------\n");
    for (int i=0; i < gSinkCount; i++) {
        printf("%d\n", array[i]);
    }
    printf("Sink states count: %d", gSinkCount);
    printf("\n---------------------------------------------\n");
}

//-------------------------------------------------------------------------------------------------------------


//valori di ritorno: 1 se accetta, 0 se non accetta, 2 se va in loop
int TuringMachineSimulation (Transition **t, const unsigned int *acc, const unsigned int *sinkStates, const long int max,
                             CoWInputString *inputString, unsigned int actualState, int headPos,
                             unsigned long int actualSteps, Transition *startTransition) {
    char actualReadSymbol;
    Transition *stateTransitions = NULL;

    //esegui startTransition se != NULL (solo quella originaria è = NULL)
    if (startTransition != NULL) {
        //controlla se la transizione modifica la stringa e se è condivisa la sdoppia
        if (inputString -> MTCount > 1 && startTransition -> symbol_r != startTransition -> symbol_w) {
            inputString = CoWCopy(inputString);
        }

        ExecuteTransition(startTransition, inputString -> input, &actualSteps, &headPos, &actualState);

        /*//------------------- DEBUG print block -----------------------------------------------
        printf("\ntr: %d %c %c %c %d, state: %d head: %d steps: %d\n", startTransition->state_i, startTransition->symbol_r,
        startTransition->symbol_w, startTransition->head_movement, startTransition->state_f, actualState, headPos,
        actualSteps);
        printf("\ninput: ");
        PrintInput(inputString);
        printf("\n");
        //------------------- DEBUG print block -----------------------------------------------*/

        if (CheckIfReachLoopStatus(max, actualSteps) == 1) {
            //printf("2 ");           //DEBUG
            CoWCountDecrease(inputString);
            return 2;
        }
    }

    while (IsAcceptationState(acc, actualState) == 0) {
        //Ottimizzazione: se si è in uno stato pozzo e una MT è già andata in loop, la si può terminare senza alterare
        //i risultati, con risultato 0
        if (AT_LEAST_ONE_MT_LOOPED == 1 && IsSinkState(sinkStates, actualState) == 1) {
            CoWCountDecrease(inputString);
            return 0;
        }

        //caso head punta array prev (precedenti alla posizione iniziale array input)
        if (headPos < 0) {
            //caso particolare di transizione che manda sicuramente in loop la MT (scorre sempre a sinistra fino a max)
            if (stateTransitions != NULL && stateTransitions -> symbol_r == '_'
                && stateTransitions -> state_f == stateTransitions -> state_i
                && stateTransitions -> head_movement == 'L') {
                //printf("2 ");               //DEBUG
                AT_LEAST_ONE_MT_LOOPED = 1;
                CoWCountDecrease(inputString);
                return 2;
            }

            if (inputString -> MTCount > 1) {
                inputString = CoWCopy(inputString);
            }

            inputString -> input = realloc(inputString -> input, (inputString -> inputSize + REALLOC_LENGTH) * sizeof(char));
            memmove(&inputString -> input[REALLOC_LENGTH], &inputString -> input[0], inputString -> inputSize * sizeof(char));
            for (int i = 0; i < REALLOC_LENGTH; i++) {
                inputString -> input[i] = '_';
            }
            headPos += REALLOC_LENGTH;
            inputString -> inputSize += REALLOC_LENGTH;
            actualReadSymbol = inputString -> input[headPos];
        }

        //caso head punta a array succ (successivi alla posizione iniziale array input)
        else if (headPos >= inputString -> inputSize) {
            //caso particolare di transizione che manda sicuramente in loop la MT (scorre sempre a destra fino a max)
            if (stateTransitions != NULL && stateTransitions -> symbol_r == '_'
                && stateTransitions -> state_f == stateTransitions -> state_i
                && stateTransitions -> head_movement == 'R') {
                //printf("2 ");               //DEBUG
                AT_LEAST_ONE_MT_LOOPED = 1;
                CoWCountDecrease(inputString);
                return 2;
            }

            if (inputString -> MTCount > 1) {
                inputString = CoWCopy(inputString);
            }

            inputString -> input = realloc(inputString -> input, (inputString -> inputSize + REALLOC_LENGTH) * sizeof(char));
            for (int i = 0; i < REALLOC_LENGTH; i++) {
                inputString -> input[inputString -> inputSize + i] = '_';
            }
            inputString -> inputSize += REALLOC_LENGTH;
            actualReadSymbol = inputString -> input[headPos];
        }

        //caso head punta a array originario di input
        else {
            actualReadSymbol = inputString -> input[headPos];
        }

        stateTransitions = t[actualState];

        //cerca una transizione valida
        while(stateTransitions != NULL && stateTransitions -> symbol_r != actualReadSymbol) {
            stateTransitions = stateTransitions -> next;
        }

        if (stateTransitions != NULL) {
            //la lista è semiordinata a blocchi in base alla lettera letta, quindi se ci sono transizioni non
            // deterministiche per quel simbolo saranno quelle successive. Se quella successiva conferma questa ipotesi,
            //allora "sdoppia" la macchina attraverso una ricorsione, aspettando che il "figlio" termini.
            while (stateTransitions -> next != NULL && stateTransitions -> next -> symbol_r == actualReadSymbol) {
               //vedi descrizione funzione per chiarimenti, sa già che delle MT diramate alcune vanno in loop, le altre
               // avranno gli stessi risultati della MT padre solo che spostati di 2*k step, quindi evita di sdoppiarla,
               //setta solamente il flag (evita così di sdoppiare una moltitutide di MT inutili)
                if (CheckIfTransitionsLeadToLoop(stateTransitions, startTransition) == 1) {
                    AT_LEAST_ONE_MT_LOOPED = 1;
                }

                else {
                    int childResult;
                    inputString -> MTCount += 1;            //il figlio avrà la stringa condivisa col padre

                    //printf("----------- New branch: --------------\n"); //DEBUG
                    childResult = TuringMachineSimulation(t, acc, sinkStates, max, inputString, actualState,
                                                          headPos, actualSteps, stateTransitions);
                    //printf("-------------- branch ended ---------------\n");    //DEBUG

                    //se un figlio accetta la stringa, termina tutte quelle superiori con risultato positivo (accetta)
                    if (childResult == 1) {
                        //printf("1 ");       //DEBUG
                        CoWCountDecrease(inputString);
                        return 1;
                    }
                }

                stateTransitions = stateTransitions -> next;            //passa alla successiva e ripete
            }

            //sa che la MT va in loop, evita successive iterazioni e la termina subito
            if (CheckIfTransitionsLeadToLoop(stateTransitions, startTransition) == 1) {
                //printf("2 ");               //DEBUG
                AT_LEAST_ONE_MT_LOOPED = 1;
                CoWCountDecrease(inputString);
                return 2;
            }
            if (inputString -> MTCount > 1 && stateTransitions -> symbol_r != stateTransitions -> symbol_w) {
                inputString = CoWCopy(inputString);
            }

            //caso ultima (o unica) transizione possibile (non c'è scelta, è l'unica eseguibile)
            ExecuteTransition(stateTransitions, inputString -> input, &actualSteps, &headPos, &actualState);
            startTransition = stateTransitions;      //salva l'ultima transizione in startTransition, che adesso è
                                                     //inutilizzata e libera di essere sovrascritta

            /*//------------------- DEBUG print block -----------------------------------------------
            printf("\ntr: %d %c %c %c %d, state: %d head: %d steps: %d\n", stateTransitions->state_i, stateTransitions->symbol_r,
                   stateTransitions->symbol_w, stateTransitions->head_movement, stateTransitions->state_f, actualState, headPos,
                   actualSteps);
            printf("\ninput: ");
            PrintInput(inputString);
            printf("\n");
            //------------------- DEBUG print block -----------------------------------------------*/

            if (CheckIfReachLoopStatus(max, actualSteps) == 1) {
                //printf("2 ");               //DEBUG
                CoWCountDecrease(inputString);
                return 2;
            }
        }

        else {                           //se non ci sono transizioni valide la macchina non accetta
            //printf("0 ");              //DEBUG
            CoWCountDecrease(inputString);
            return 0;
        }
    }
    CoWCountDecrease(inputString);
    return 1;                                   //caso actualState è stato di accettazione
}


int main() {
    unsigned int *acceptationStates = NULL;             //array dinamico di stati di accettazione
    InputString *inputStrings = NULL;                   //lista di stringhe di input
    unsigned long int maxTransitions;                   //numero massimo di transizioni
    int maxStateI = 0;                                  //stato con numero più elevato tra quelli da cui parte una transizione
    Transition **transitionAdjacencyList = NULL;        //lista di adiacenza del grafo (indirizzamento rapido)

    Transition *deltaFunctions = NULL;                  //lista di transizioni

    if (ReadStream(&deltaFunctions, &acceptationStates, &maxTransitions, &inputStrings, &maxStateI) == 1) {
        return 5;               //errore di allocazione nel parsing del file
    }

    //PrintAccStates (acceptationStates);                                 //DEBUG
    if(!AS_SORTED) {
        QuickSort(acceptationStates, 0, gAcceptationStatesCount - 1);
    }
    //PrintAccStates (acceptationStates);


    //maxStateI + 1 perchè ad esempio se maxState = 2, ci sono 3 stati (0, 1, 2)
    transitionAdjacencyList = CreateTransitionAdjacencyList(maxStateI + 1, deltaFunctions);
    deltaFunctions = NULL;

    unsigned int *sinkStates = NULL;             //array dinamico di stati pozzo individuati
    sinkStates = CheckSinkStates(transitionAdjacencyList, maxStateI);
    //PrintSinkStates(sinkStates);               //DEBUG

    while (inputStrings != NULL) {
        int result;
        InputString *temp;
        temp = inputStrings;
        AT_LEAST_ONE_MT_LOOPED = 0;

        CoWInputString *input;
        input = CreateCoWInputString(temp -> input, temp -> inputSize, 1);
        inputStrings = temp -> next;
        free(temp);                                                     //libera la stringa appena utilizzata

       /* printf ("starting input: ");              //DEBUG
        PrintInput(*temp);
        printf ("\n");*/
        result = TuringMachineSimulation (transitionAdjacencyList, acceptationStates, sinkStates, maxTransitions,
                                           input, 0, 0, 0, NULL);
        //printf("\n\n");
        if (result == 6) {
            return 6;
        }

        if (result == 1) {
            printf("1\n");
        }
        else if (AT_LEAST_ONE_MT_LOOPED == 1) {
            printf("U\n");
        }
        else {
            printf("0\n");
        }
    }

    //PrintAccStates(acceptationStates);          //DEBUG
    free(acceptationStates);
    free(sinkStates);
    //printf("\n");                               //DEBUG
    for (int i = 0; i <= maxStateI; i++) {
        Transition *temp;
        while (transitionAdjacencyList[i] != NULL) {
            temp = transitionAdjacencyList[i];
            transitionAdjacencyList[i] = transitionAdjacencyList[i] -> next;
            /*printf("%d %c %c %c %d\n", temp->state_i, temp->symbol_r,
                   temp->symbol_w, temp->head_movement,
                   temp->state_f);                                          //DEBUG*/
            free(temp);
        }
    }
    free(transitionAdjacencyList);

    return 0;
}