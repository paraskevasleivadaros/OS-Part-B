//
// Created by paras on 23-Mar-19.
//

#include "p3150173-p3150090-p3120120-res2.h"

unsigned int seed;
unsigned int customers;
unsigned int profit = 0;
unsigned int servedCounter = 0;
unsigned int transactions = 0;
unsigned int transactionsZoneA = 0;
unsigned int transactionsZoneB = 0;
unsigned int transactionsZoneC = 0;
unsigned int telephonist = N_TEL;
unsigned int cashiers = N_CASH;
unsigned int totalSeats = N_SEAT * (N_ZONE_A + N_ZONE_B + N_ZONE_C);
unsigned int remainingSeats = N_SEAT * (N_ZONE_A + N_ZONE_B + N_ZONE_C);
unsigned int remainingSeatsZoneA = N_SEAT * N_ZONE_A;
unsigned int remainingSeatsZoneB = N_SEAT * N_ZONE_B;
unsigned int remainingSeatsZoneC = N_SEAT * N_ZONE_C;

bool *resultPtr;
bool *tempFlagPtr;
unsigned int *tempPtr;
unsigned int *costPtr;

unsigned int seatsPlan[N_SEAT * (N_ZONE_A + N_ZONE_B + N_ZONE_C)];
unsigned int *seedPtr = &seed;
unsigned int *profitPtr = &profit;
unsigned int *servedCounterPtr = &servedCounter;
unsigned int *transactionsPtr = &transactions;
unsigned int *transactionsZoneAPtr = &transactionsZoneA;
unsigned int *transactionsZoneBPtr = &transactionsZoneB;
unsigned int *transactionsZoneCPtr = &transactionsZoneC;
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

char zoneRandom();

unsigned int choiceRandom(int, int);

double cardRandom(double, double);

void startTimer();

void stopTimer();

void Clock();

void check_rc(int);

unsigned int Cost(unsigned int, char);

unsigned int logTransaction(char zone);

bool bookSeats(unsigned int, unsigned int, char);

void unbookSeats(unsigned int, unsigned int, char);

void printSeatsPlan();

void printInfo();

void *customer(void *x);

bool checkAvailableSeats(unsigned int, char);

bool POS(unsigned int, unsigned int, char);

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
pthread_cond_t availableCashiers;

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
    check_rc(pthread_cond_init(&availableCashiers, NULL));

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
    check_rc(pthread_cond_destroy(&availableCashiers));

    printInfo();
}

void *customer(void *x) {

    unsigned int id = (int *) x;

    struct timespec waitStart, waitEnd, servStart, servEnd;
    struct timespec waitStartCash, waitEndCash, servStartCash, servEndCash;

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
    *totalWaitTimePtr += (waitEnd.tv_sec - waitStart.tv_sec);
    check_rc(pthread_mutex_unlock(&avgWaitTimeLock));

    --telephonist;

    check_rc(pthread_mutex_unlock(&operatorsLock));

    clock_gettime(CLOCK_REALTIME, &servStart);

    if (checkRemainingSeats()) {

        char zone = zoneRandom();
        unsigned int seats = choiceRandom(N_SEAT_LOW, N_SEAT_HIGH);

        sleep(sleepRandom(T_SEAT_LOW, T_SEAT_HIGH));

        if (checkAvailableSeats(seats, zone)) {

            if (bookSeats(seats, id, zone)) {

                check_rc(pthread_mutex_lock(&cashiersLock));

                clock_gettime(CLOCK_REALTIME, &waitStartCash);

                while (cashiers == 0) {
                    // Customer couldn't find cashier available. Blocked..
                    check_rc(pthread_cond_wait(&availableCashiers, &cashiersLock));
                }

                // Customer being served..
                clock_gettime(CLOCK_REALTIME, &waitEndCash);

                check_rc(pthread_mutex_lock(&avgWaitTimeLock));
                *totalWaitTimePtr += (waitEndCash.tv_sec - waitStartCash.tv_sec);
                check_rc(pthread_mutex_unlock(&avgWaitTimeLock));

                --cashiers;

                check_rc(pthread_mutex_unlock(&cashiersLock));

                clock_gettime(CLOCK_REALTIME, &servStartCash);

                sleep(sleepRandom(T_CASH_LOW, T_CASH_HIGH));

                if (POS(seats, id, zone)) {

                    check_rc(pthread_mutex_lock(&screenLock));

                    Clock();
                    printf("Η κράτηση ολοκληρώθηκε επιτυχώς. Ο αριθμός συναλλαγής είναι %03d", logTransaction(zone));

                    check_rc(pthread_mutex_lock(&seatsPlanLock));
                    printf(", οι θέσεις σας είναι οι: ");
                    for (int i = 0; i < totalSeats; i++) {
                        if (seatsPlan[i] == id) {
                            printf("%03d ", i + 1);
                        }
                    }
                    check_rc(pthread_mutex_unlock(&seatsPlanLock));

                    printf("και το κόστος της συναλλαγής είναι %03d\u20AC\n\n", Cost(seats, zone));
                    check_rc(pthread_mutex_unlock(&screenLock));
                }

                check_rc(pthread_mutex_lock(&cashiersLock));

                clock_gettime(CLOCK_REALTIME, &servEndCash);

                check_rc(pthread_mutex_lock(&avgServingTimeLock));
                *totalServTimePtr += (servEndCash.tv_sec - servStartCash.tv_sec);
                check_rc(pthread_mutex_unlock(&avgServingTimeLock));

                ++cashiers;

                check_rc(pthread_cond_broadcast(&availableCashiers));

                check_rc(pthread_mutex_unlock(&cashiersLock));

            }
        }
    }

    check_rc(pthread_mutex_lock(&operatorsLock));

    // Customer served successfully
    ++(*servedCounterPtr);

    clock_gettime(CLOCK_REALTIME, &servEnd);

    check_rc(pthread_mutex_lock(&avgServingTimeLock));
    *totalServTimePtr += (servEnd.tv_sec - servStart.tv_sec);
    check_rc(pthread_mutex_unlock(&avgServingTimeLock));

    ++telephonist;

    check_rc(pthread_cond_broadcast(&availableOperators));

    check_rc(pthread_mutex_unlock(&operatorsLock));

    pthread_exit(NULL); // return
}

unsigned int sleepRandom(int min, int max) {
    return (rand_r(seedPtr) % (max - min + 1)) + min;
}

char zoneRandom() {
    double f = (double) rand_r(seedPtr) / RAND_MAX;
    if (f <= P_ZONE_A) {
        return 'A';
    } else if (f <= (P_ZONE_A + P_ZONE_B)) {
        return 'B';
    } else if (f <= (P_ZONE_A + P_ZONE_B + P_ZONE_C)) {
        return 'C';
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

unsigned int Cost(unsigned int numOfSeats, char zone) {
    check_rc(pthread_mutex_lock(&paymentLock));
    unsigned int cost;
    costPtr = &cost;
    switch (zone) {
        case 'A':
            *costPtr = numOfSeats * C_ZONE_A;
            break;
        case 'B':
            *costPtr = numOfSeats * C_ZONE_B;
            break;
        case 'C':
            *costPtr = numOfSeats * C_ZONE_C;
            break;
        default:
            break;
    }
    *profitPtr += *costPtr;
    check_rc(pthread_mutex_unlock(&paymentLock));
    return *costPtr;
}

unsigned int logTransaction(char zone) {
    check_rc(pthread_mutex_lock(&transactionLock));
    switch (zone) {
        case 'A':
            ++(*transactionsZoneAPtr);
            break;
        case 'B':
            ++(*transactionsZoneBPtr);
            break;
        case 'C':
            ++(*transactionsZoneCPtr);
            break;
        default:
            break;
    }
    unsigned int transactionID = ++(*transactionsPtr);
    check_rc(pthread_mutex_unlock(&transactionLock));
    return transactionID;
}

bool bookSeats(unsigned int numOfSeats, unsigned int custID, char zone) {
    check_rc(pthread_mutex_lock(&seatsPlanLock));
    unsigned int temp = 0;
    tempPtr = &temp;
    bool tempFlag = true;
    tempFlagPtr = &tempFlag;
    switch (zone) {
        case 'A':
            for (int j = 0; j < N_ZONE_A && *tempFlagPtr; j++) {
                // check if there are enough consecutive seats
                for (int i = N_SEAT * j; i < (N_SEAT * (j + 1)); i++) {
                    if (seatsPlan[i] == 0) {
                        ++(*tempPtr);
                    }
                }
                if (*tempPtr >= numOfSeats) {
                    *tempPtr = numOfSeats;
                    *tempFlagPtr = false;
                    for (int i = (N_SEAT * j); *tempPtr > 0; i++) {
                        if (seatsPlan[i] == 0) {
                            seatsPlan[i] = custID;
                            --(*tempPtr);
                        }
                    }
                    *remainingSeatsZoneAPtr -= numOfSeats;
                }
            }
            break;
        case 'B':
            for (int j = N_ZONE_A; j < (N_ZONE_A + N_ZONE_B) && *tempFlagPtr; j++) {
                // check if there are enough consecutive seats
                for (int i = N_SEAT * j; i < (N_SEAT * (j + 1)); i++) {
                    if (seatsPlan[i] == 0) {
                        ++(*tempPtr);
                    }
                }
                if (*tempPtr >= numOfSeats) {
                    *tempPtr = numOfSeats;
                    *tempFlagPtr = false;
                    for (int i = (N_SEAT * j); *tempPtr > 0; i++) {
                        if (seatsPlan[i] == 0) {
                            seatsPlan[i] = custID;
                            --(*tempPtr);
                        }
                    }
                    *remainingSeatsZoneBPtr -= numOfSeats;
                }
            }
            break;
        case 'C':
            for (int j = (N_ZONE_A + N_ZONE_B); j < (N_ZONE_A + N_ZONE_B + N_ZONE_C) && *tempFlagPtr; j++) {
                // check if there are enough consecutive seats
                for (int i = N_SEAT * j; i < (N_SEAT * (j + 1)); i++) {
                    if (seatsPlan[i] == 0) {
                        ++(*tempPtr);
                    }
                }
                if (*tempPtr >= numOfSeats) {
                    *tempPtr = numOfSeats;
                    *tempFlagPtr = false;
                    for (int i = (N_SEAT * j); *tempPtr > 0; i++) {
                        if (seatsPlan[i] == 0) {
                            seatsPlan[i] = custID;
                            --(*tempPtr);
                        }
                    }
                    *remainingSeatsZoneCPtr -= numOfSeats;
                }
            }
            break;
        default:
            break;
    }
    bool result = (*tempPtr == 0);
    if (result) {
        (*remainingSeatsPtr) -= numOfSeats;
    } else {
        check_rc(pthread_mutex_lock(&screenLock));
        Clock();
        printf("Η κράτηση ματαιώθηκε γιατί δεν υπάρχουν αρκετές διαθέσιμες συνεχόμενες θέσεις\n\n");
        check_rc(pthread_mutex_unlock(&screenLock));
    }
    check_rc(pthread_mutex_unlock(&seatsPlanLock));
    return result;
}

void unbookSeats(unsigned int numOfSeats, unsigned int custID, char zone) {
    check_rc(pthread_mutex_lock(&seatsPlanLock));
    int temp = numOfSeats;
    for (int i = 0; temp > 0 && i < totalSeats; i++) {
        if (seatsPlan[i] == custID) {
            seatsPlan[i] = 0;
            temp--;
        }
    }
    (*remainingSeatsPtr) += numOfSeats;
    switch (zone) {
        case 'A':
            *remainingSeatsZoneAPtr += numOfSeats;
            break;
        case 'B':
            *remainingSeatsZoneBPtr += numOfSeats;
            break;
        case 'C':
            *remainingSeatsZoneCPtr += numOfSeats;
            break;
        default:
            break;
    }
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

bool checkAvailableSeats(unsigned int choice, char zone) {
    check_rc(pthread_mutex_lock(&seatsPlanLock));

    // check if there are enough spaces left at the theater
    bool result;
    resultPtr = &result;
    *resultPtr = (choice <= (*remainingSeatsPtr));

    if (!*resultPtr) {
        check_rc(pthread_mutex_lock(&screenLock));
        Clock();
        printf("Η κράτηση ματαιώθηκε γιατί δεν υπάρχουν αρκετές διαθέσιμες θέσεις\n\n");
        check_rc(pthread_mutex_unlock(&screenLock));
    } else {
        // check if there are enough spaces left at the selected zone
        switch (zone) {
            case 'A':
                *resultPtr = (choice <= (*remainingSeatsZoneAPtr));
                break;
            case 'B':
                *resultPtr = (choice <= (*remainingSeatsZoneBPtr));
                break;
            case 'C':
                *resultPtr = (choice <= (*remainingSeatsZoneCPtr));
                break;
            default:
                break;
        }
    }
    check_rc(pthread_mutex_unlock(&seatsPlanLock));
    return *resultPtr;
}

bool POS(unsigned int numOfSeats, unsigned int custID, char zone) {
    bool result = (cardRandom(0.0, 1.0) <= P_CARD_SUCCESS);
    if (!result) {
        unbookSeats(numOfSeats, custID, zone);
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
    printf("Πλάνο Θέσεων:\n\n");
    printf("Ζώνη A:\n");
    int changeRow = 1;
    int changeZone = 1;
    printf("| ");

    for (int i = 0; i < totalSeats; i++) {
        if (seatsPlan[i] == 0) {
            printf("Θ %03d: Π     | ", i + 1);
        } else {
            printf("Θ %03d: Π %03d | ", i + 1, seatsPlan[i]);
        }
        // changing line after N_SEAT seats
        if (changeRow == N_SEAT) {
            changeZone += 1;
            printf("\n");
            if (i != (totalSeats - 1)) {
                if (changeZone == N_ZONE_A) {
                    printf("\nΖώνη B:\n");
                }
                if (changeZone == N_ZONE_A + N_ZONE_B) {
                    printf("\nΖώνη C:\n");
                }
                printf("| ");
            }
            changeRow = 0;
        }
        changeRow += 1;
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
    printf("\n");
    printf("Εξυπηρετήθηκαν: %03d πελάτες\n", *servedCounterPtr);
    printf("Δεσμευμένες Θέσεις: %d\n", totalSeats - (*remainingSeatsPtr));
    printf("Ελεύθερες Θέσεις: %d\n", (*remainingSeatsPtr));
    printf("\n");
    printf("Συναλλαγές [Σύνολο]: %d\n", (*transactionsPtr));
    printf("Συναλλαγές [Ζώνη A]: %d\n", (*transactionsZoneAPtr));
    printf("Συναλλαγές [Ζώνη B]: %d\n", (*transactionsZoneBPtr));
    printf("Συναλλαγές [Ζώνη C]: %d\n", (*transactionsZoneCPtr));
    printf("Κέρδη: %d\u20AC\n", (*profitPtr));
    printf("\nΈξοδος..\n");
}