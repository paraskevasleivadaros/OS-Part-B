//
// Created by paras on 23-Mar-19.
//

#include "p3150173-p3150090-p3120120-res2.h"

unsigned int seed;
unsigned int customers;
unsigned int profit = 0;
unsigned int servedCounter = 0;
unsigned int transactions = 0;
unsigned int telephonist = N_TEL;
unsigned int remainingSeats = N_SEAT * (N_ZONE_A + N_ZONE_B + N_ZONE_C);
unsigned int remainingSeatsZoneA = N_SEAT * N_ZONE_A;
unsigned int remainingSeatsZoneB = N_SEAT * N_ZONE_B;
unsigned int remainingSeatsZoneC = N_SEAT * N_ZONE_C;

bool *resultPtr;
unsigned int *costPtr;

unsigned int seatsPlan[N_SEAT];
unsigned int *seedPtr = &seed;
unsigned int *profitPtr = &profit;
unsigned int *servedCounterPtr = &servedCounter;
unsigned int *transactionsPtr = &transactions;
unsigned int *remainingSeatsPtr = &remainingSeats;
unsigned int *remainingSeatsZoneAPtr = &remainingSeatsZoneA;
unsigned int *remainingSeatsZoneBPtr = &remainingSeatsZoneB;
unsigned int *remainingSeatsZoneCPtr = &remainingSeatsZoneC;

// Calculate time taken by a request
struct timespec requestStart, requestEnd;
struct tm start;
struct tm end;

unsigned long int totalWaitTime;
unsigned long int totalServTime;

unsigned long int *totalWaitTimePtr = &totalWaitTime;
unsigned long int *totalServTimePtr = &totalServTime;

unsigned int sleepRandom(int, int);

unsigned int zoneRandom();

unsigned int choiceRandom(int, int);

double cardRandom(double, double);

void startTimer();

void stopTimer();

void Clock();

void check_rc(int);

unsigned int Cost(unsigned int, unsigned int);

unsigned int logTransaction();

void bookSeats(unsigned int, unsigned int);

void unbookSeats(unsigned int, unsigned int);

void printSeatsPlan();

void printInfo();

void *customer(void *x);

bool checkAvailableSeats(unsigned int, unsigned int);

bool POS(unsigned int, unsigned int);

bool checkRemainingSeats();

pthread_mutex_t operatorsLock;
pthread_mutex_t paymentLock;
pthread_mutex_t transactionLock;
pthread_mutex_t avgWaitTimeLock;
pthread_mutex_t avgServingTimeLock;
pthread_mutex_t seatsPlanLock;
pthread_mutex_t screenLock;
pthread_mutex_t cashiersLock;
pthread_cond_t availableOperators;

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Παρακαλώ δώστε μόνο το πλήθος των πελατών και τον σπόρο\n");
        exit(-1);
    }

    // Converting string type to integer type
    // using function atoi
    customers = atoi(argv[1]);
    *seedPtr = atoi(argv[2]);

    // Checking if number of customers is positive
    if (customers <= 0) {
        printf("Παρακαλώ δώστε μόνο θετική τιμή για το πλήθος των πελατών\n");
        exit(-2);
    }

    printf("Πελάτες προς εξυπηρέτηση: %03d\n\n", customers);

    pthread_t threads[customers];
    int id[customers];

    check_rc(pthread_mutex_init(&operatorsLock, NULL));
    check_rc(pthread_mutex_init(&paymentLock, NULL));
    check_rc(pthread_mutex_init(&transactionLock, NULL));
    check_rc(pthread_mutex_init(&avgWaitTimeLock, NULL));
    check_rc(pthread_mutex_init(&avgServingTimeLock, NULL));
    check_rc(pthread_mutex_init(&seatsPlanLock, NULL));
    check_rc(pthread_mutex_init(&screenLock, NULL));
    check_rc(pthread_mutex_init(&cashiersLock, NULL));
    check_rc(pthread_cond_init(&availableOperators, NULL));

    startTimer();

    for (int i = 0; i < customers; i++) {
        id[i] = i + 1;
        check_rc(pthread_create(&threads[i], NULL, customer, (void *) id[i]));
    }

    for (int i = 0; i < customers; i++) {
        check_rc(pthread_join(threads[i], NULL));
    }

    stopTimer();

    check_rc(pthread_mutex_destroy(&operatorsLock));
    check_rc(pthread_mutex_destroy(&paymentLock));
    check_rc(pthread_mutex_destroy(&transactionLock));
    check_rc(pthread_mutex_destroy(&avgWaitTimeLock));
    check_rc(pthread_mutex_destroy(&avgServingTimeLock));
    check_rc(pthread_mutex_destroy(&seatsPlanLock));
    check_rc(pthread_mutex_destroy(&screenLock));
    check_rc(pthread_mutex_destroy(&cashiersLock));
    check_rc(pthread_cond_destroy(&availableOperators));

    printInfo();
}

void *customer(void *x) {

    unsigned int id = (int *) x;

    struct timespec waitStart, waitEnd, servStart, servEnd;

    // Customer calling..

    check_rc(pthread_mutex_lock(&operatorsLock));

    clock_gettime(CLOCK_REALTIME, &waitStart);

    while (telephonist == 0) {
        // Customer couldn't find telephonist available. Blocked..
        check_rc(pthread_cond_wait(&availableOperators, &operatorsLock));
    }

    // Customer being served..
    clock_gettime(CLOCK_REALTIME, &waitEnd);

    check_rc(pthread_mutex_lock(&avgWaitTimeLock));
    *totalWaitTimePtr = *totalWaitTimePtr + (waitEnd.tv_sec - waitStart.tv_sec);
    check_rc(pthread_mutex_unlock(&avgWaitTimeLock));

    --telephonist;

    check_rc(pthread_mutex_unlock(&operatorsLock));

    clock_gettime(CLOCK_REALTIME, &servStart);

    if (checkRemainingSeats()) {

        unsigned int zone = zoneRandom();
        unsigned int seats = choiceRandom(N_SEAT_LOW, N_SEAT_HIGH);

        sleep(sleepRandom(T_SEAT_LOW, T_SEAT_HIGH));

        if (checkAvailableSeats(seats, zone)) {

            bookSeats(seats, id);

            if (POS(seats, id)) {

                check_rc(pthread_mutex_lock(&screenLock));

                Clock();
                printf("Η κράτηση ολοκληρώθηκε επιτυχώς. Ο αριθμός συναλλαγής είναι %03d", logTransaction());

                check_rc(pthread_mutex_lock(&seatsPlanLock));
                printf(", οι θέσεις σας είναι οι: ");
                for (int i = 0; i < N_SEAT; i++) {
                    if (seatsPlan[i] == id) {
                        printf("%03d ", i + 1);
                    }
                }
                check_rc(pthread_mutex_unlock(&seatsPlanLock));

                printf("και το κόστος της συναλλαγής είναι %03d\u20AC\n\n", Cost(seats, zone));
                check_rc(pthread_mutex_unlock(&screenLock));
            }
        }
    }

    check_rc(pthread_mutex_lock(&operatorsLock));

    // Customer served successfully
    ++(*servedCounterPtr);

    clock_gettime(CLOCK_REALTIME, &servEnd);

    check_rc(pthread_mutex_lock(&avgServingTimeLock));
    *totalServTimePtr = *totalServTimePtr + (servEnd.tv_sec - servStart.tv_sec);
    check_rc(pthread_mutex_unlock(&avgServingTimeLock));

    ++telephonist;

    check_rc(pthread_cond_broadcast(&availableOperators));

    check_rc(pthread_mutex_unlock(&operatorsLock));

    pthread_exit(NULL); // return
}

unsigned int sleepRandom(int min, int max) {
    return (rand_r(seedPtr) % (max - min + 1)) + min;
}

unsigned int zoneRandom() {
    double f = (double) rand_r(seedPtr) / RAND_MAX;
    if (f <= P_ZONE_A) {
        return 1;
    } else if (f <= (P_ZONE_A + P_ZONE_B)) {
        return 2;
    } else {
        return 3;
    }
}

unsigned int choiceRandom(int min, int max) {
    return (rand_r(seedPtr) % (max - min + 1)) + min;
}

double cardRandom(double min, double max) {
    double f = (double) rand_r(seedPtr) / RAND_MAX;
    return min + f * (max - min);
}

void startTimer() {
    Clock();
    printf("Έναρξη Χρονομέτρησης\n\n");
    time_t t = time(NULL);
    start = *localtime(&t);
    clock_gettime(CLOCK_REALTIME, &requestStart);
}

void stopTimer() {
    time_t t = time(NULL);
    end = *localtime(&t);
    clock_gettime(CLOCK_REALTIME, &requestEnd);
    Clock();
    printf("Λήξη Χρονομέτρησης\n\n");
}

void Clock() {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("[%02d:%02d:%02d] ", tm.tm_hour, tm.tm_min, tm.tm_sec);
}

unsigned int Cost(unsigned int numOfSeats, unsigned int zone) {
    check_rc(pthread_mutex_lock(&paymentLock));
    unsigned int cost;
    costPtr = &cost;
    switch (zone) {
        case 1:
            *costPtr = numOfSeats * C_ZONE_A;
        case 2:
            *costPtr = numOfSeats * C_ZONE_B;
        case 3:
            *costPtr = numOfSeats * C_ZONE_C;

        default:
            *costPtr = 0;
    }
    *profitPtr += *costPtr;
    check_rc(pthread_mutex_unlock(&paymentLock));
    return *costPtr;
}

unsigned int logTransaction() {
    check_rc(pthread_mutex_lock(&transactionLock));
    unsigned int transactionID = ++(*transactionsPtr);
    check_rc(pthread_mutex_unlock(&transactionLock));
    return transactionID;
}

void bookSeats(unsigned int numOfSeats, unsigned int custID) {
    check_rc(pthread_mutex_lock(&seatsPlanLock));
    int temp = numOfSeats;
    for (int i = 0; temp > 0 && i < N_SEAT; i++) {
        if (seatsPlan[i] == 0) {
            seatsPlan[i] = custID;
            temp--;
        }
    }
    (*remainingSeatsPtr) -= numOfSeats;
    check_rc(pthread_mutex_unlock(&seatsPlanLock));
}

void unbookSeats(unsigned int numOfSeats, unsigned int custID) {
    check_rc(pthread_mutex_lock(&seatsPlanLock));
    int temp = numOfSeats;
    for (int i = 0; temp > 0 && i < N_SEAT; i++) {
        if (seatsPlan[i] == custID) {
            seatsPlan[i] = 0;
            temp--;
        }
    }
    (*remainingSeatsPtr) += numOfSeats;
    check_rc(pthread_mutex_unlock(&seatsPlanLock));
}

void check_rc(int rc) {
    if (rc) {
        check_rc(pthread_mutex_lock(&screenLock));
        Clock();
        printf("Σφάλμα: rc\n\n");
        printf("Έξοδος..");
        check_rc(pthread_mutex_unlock(&screenLock));
        exit(-1);
    }
}

bool checkRemainingSeats() {
    check_rc(pthread_mutex_lock(&seatsPlanLock));
    bool result = (*remainingSeatsPtr != 0);
    if (!result) {
        check_rc(pthread_mutex_lock(&screenLock));
        Clock();
        printf("Η κράτηση ματαιώθηκε γιατί το θέατρο είναι γεμάτο\n\n");
        check_rc(pthread_mutex_unlock(&screenLock));
    }
    check_rc(pthread_mutex_unlock(&seatsPlanLock));
    return result;
}

bool checkAvailableSeats(unsigned int choice, unsigned int zone) {
    check_rc(pthread_mutex_lock(&seatsPlanLock));

    // check if there are enough spaces left at the theater
    bool result = (choice <= (*remainingSeatsPtr));
    resultPtr = &result;
    if (!*resultPtr) {
        check_rc(pthread_mutex_lock(&screenLock));
        Clock();
        printf("Η κράτηση ματαιώθηκε γιατί δεν υπάρχουν αρκετές διαθέσιμες θέσεις\n\n");
        check_rc(pthread_mutex_unlock(&screenLock));
    }

    // check if there are enough spaces left at the selected zone
    switch (zone) {
        case 1:
            *resultPtr = (choice <= (*remainingSeatsZoneAPtr));
        case 2:
            *resultPtr = (choice <= (*remainingSeatsZoneAPtr));
        case 3:
            *resultPtr = (choice <= (*remainingSeatsZoneAPtr));
        default:
            *resultPtr = 0;
    }
    check_rc(pthread_mutex_unlock(&seatsPlanLock));
    return *resultPtr;
}

bool POS(unsigned int numOfSeats, unsigned int custID) {
    bool result = (cardRandom(0.0, 1.0) <= P_CARD_SUCCESS);
    if (!result) {
        unbookSeats(numOfSeats, custID);
        check_rc(pthread_mutex_lock(&screenLock));
        Clock();
        printf("Η κράτηση ματαιώθηκε γιατί η συναλλαγή με την πιστωτική κάρτα δεν έγινε αποδεκτή\n\n");
        check_rc(pthread_mutex_unlock(&screenLock));
    }
    return result;
}

void printDuration(unsigned long int minutes, unsigned long int seconds, unsigned long int totalSeconds) {
    if (minutes > 1) {
        printf("Διάρκεια: %ld λεπτά ", minutes);
    } else {
        printf("Διάρκεια: %ld λεπτό ", minutes);
    }
    if (seconds > 1) {
        printf("και %ld δευτερόλεπτα (%lds)\n", seconds, totalSeconds);
    } else {
        printf("και %ld δευτερόλεπτο (%lds)\n", seconds, totalSeconds);
    }
}

void printSeatsPlan() {
    printf("Πλάνο Θέσεων:\n");
    int printCounter = 1;
    printf("| ");
    // every row has N_SEAT seats
    for (int i = 0; i < N_SEAT; i++) {
        if (seatsPlan[i] == 0) {
            printf("Θ %03d: Π     | ", i + 1);
        } else {
            printf("Θ %03d: Π %03d | ", i + 1, seatsPlan[i]);
        }
        if (printCounter == 4) {
            printf("\n| ");
            printCounter = 0;
        }
        printCounter += 1;
    }
}

void printInfo() {
    unsigned long int totalSeconds = requestEnd.tv_sec - requestStart.tv_sec;
    unsigned long int minutes = totalSeconds / 60;
    unsigned long int seconds = totalSeconds % 60;
    printf("Έναρξη: [%02d:%02d:%02d] - ", start.tm_hour, start.tm_min, start.tm_sec);
    printf("Λήξη: [%02d:%02d:%02d]\n", end.tm_hour, end.tm_min, end.tm_sec);
    printDuration(minutes, seconds, totalSeconds);
    printf("\n");
    printSeatsPlan();
    printf("\n\n");
    printf("Μέσος χρόνος αναμονής: %0.2f seconds\n", (double) *totalWaitTimePtr / customers);
    printf("Μέσος χρόνος εξυπηρέτησης: %0.2f seconds\n", (double) *totalServTimePtr / customers);
    printf("Εξυπηρετήθηκαν: %03d πελάτες\n", *servedCounterPtr);
    printf("Δεσμευμένες Θέσεις: %d\n", N_SEAT - (*remainingSeatsPtr));
    printf("Ελεύθερες Θέσεις: %d\n", (*remainingSeatsPtr));
    printf("Συναλλαγές: %d\n", (*transactionsPtr));
    printf("Κέρδη: %d\u20AC\n", (*profitPtr));
    printf("\nΈξοδος..\n");
}