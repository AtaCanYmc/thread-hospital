#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

//----------------------------------------------------------------------------------------------------------------------------------------------------[global variables]

// The number of registration desks that are available.
int REGISTRATION_SIZE = 10;
// The number of restrooms that are available.
int RESTROOM_SIZE = 10;
// The number of cashiers in cafe that are available.
int CAFE_NUMBER = 10;
// The number of General Practitioner that are available.
int GP_NUMBER = 10;
// The number of cashiers in pharmacy that are available.
int PHARMACY_NUMBER = 10;
// The number of assistants in blood lab that are available.
int BLOOD_LAB_NUMBER = 10;
// The number of operating rooms, surgeons and nurses that are available.
int OR_NUMBER = 10;
int SURGEON_NUMBER = 30;
int NURSE_NUMBER = 30;
// The maximum number of surgeons and nurses that can do a surgery. A random value is
// calculated for each operation between 1 and given values to determine the required
// number of surgeons and nurses.
int SURGEON_LIMIT = 5;
int NURSE_LIMIT = 5;
// The number of patients that will be generated over the course of this program.
int PATIENT_NUMBER = 1000;
// The account of hospital where the money acquired from patients are stored.
int HOSPITAL_WALLET = 0;
// The time required for each operation in hospital. They are given in milliseconds. But
// you must use a randomly generated value between 1 and given values below to determine
// the time that will be required for that operation individually. This will increase the
// randomness of your simulation.
// The WAIT_TIME is the limit for randomly selected time between 1 and the given value
// that determines how long a patient will wait before each operation to retry to execute.
// Assuming the given department is full
int WAIT_TIME = 100;
int REGISTRATION_TIME = 100;
int GP_TIME = 200;
int PHARMACY_TIME = 100;
int BLOOD_LAB_TIME = 200;
int SURGERY_TIME = 500;
int CAFE_TIME = 100;
int RESTROOM_TIME = 100;
// The money that will be charged to the patients for given operations. The registration
// and blood lab costs should be static (not randomly decided) but pharmacy and cafe cost
// should be randomly generated between 1 and given value below to account for different
// medicine and food that can be purchased.
//The surgery cost should calculated based on number of doctors and nurses that was
//required to perform it. The formula used for this should be:

// SURGERY_OR_COST + (number of surgeons * SURGERY_SURGEON_COST) +
// (number of nurses * SURGERY_NURSE_COST)
int REGISTRATION_COST = 100;
int PHARMACY_COST = 200; // Calculated randomly between 1 and given value.
int BLOOD_LAB_COST = 200;
int SURGERY_OR_COST = 200;
int SURGERY_SURGEON_COST = 100;
int SURGERY_NURSE_COST = 50;
int CAFE_COST = 200; // Calculated randomly between 1 and given value.

// The global increase rate of hunger and restroom needs of patients. It will increase
//randomly between 1 and given rate below.
int HUNGER_INCREASE_RATE = 10;
int RESTROOM_INCREASE_RATE = 10;

//----------------------------------------------------------------------------------------------------------------------------------------------------[structs & control variables]

//STRUCT
struct _person {
int id;
int hunger;
int WCneed; //restroom
int situation;
int isGiveBlood;
int isGetSurgery;
int surgeonNeeding;
int nurseNeeding;
}; typedef struct _person patient;

patient Patient[1000]; //patient array
sem_t regSize, restSize, cafeSize, GPsize, BLsize, pharSize, ORsize; //semaphores
pthread_mutex_t payMut, preSurgeryMut, postSurgeryMut; //mutex

//----------------------------------------------------------------------------------------------------------------------------------------------------[Functions]

void createPatients() //create patients in array
{    
    for (size_t i = 0; i < PATIENT_NUMBER; i++)
    {
        Patient[i].id = i;
        Patient[i].situation = -1; //-1 nothing //0 needs medicine //1 needs surgery //2 needs blood test
        Patient[i].hunger = rand() % 100 + 1;
        Patient[i].WCneed = rand() % 100 + 1;
        Patient[i].isGetSurgery = 0;
        Patient[i].isGiveBlood = 0;
        Patient[i].surgeonNeeding = 0;
        Patient[i].nurseNeeding = 0;
    }    
}

void payMoney(int amount) //payin money (controlling by a mutex)
{
    pthread_mutex_lock(&payMut);
    HOSPITAL_WALLET += amount;
    pthread_mutex_unlock(&payMut);
}

void increaseNeedings(int personId) //Increase patient's needing while waiting
{
    Patient[personId].WCneed += RESTROOM_INCREASE_RATE;
    Patient[personId].hunger += HUNGER_INCREASE_RATE;
}

void goCafe(int personId) // when hunger >= 100
{
    printf("Patient[%d] is arrived at cafe\n",personId);
    if(sem_trywait(&cafeSize) == 0)
    { 
        printf("Patient[%d] entering cafe\n",personId);
        Patient[personId].hunger = 0;
        payMoney(rand() % CAFE_COST + 1); //paying
        sleep((rand() % CAFE_TIME + 1)/1000);
        sem_post(&cafeSize);
    }
    else //wait and try again
    {
        printf("Patient[%d] waiting for cafe\n",personId);
        sem_wait(&cafeSize);
        sleep((rand() % WAIT_TIME + 1)/1000);
        sem_post(&cafeSize);
        goCafe(personId);
    }
}

void goRestroom(int personId) //when restroom >= 100
{
    printf("Patient[%d] is arrived at restroom\n",personId);
    if(sem_trywait(&restSize) == 0)
    { 
        printf("Patient[%d] entering restroom\n",personId);
        Patient[personId].WCneed = 0;
        sleep((rand() % RESTROOM_TIME + 1)/1000);
        sem_post(&restSize);
    }
    else //wait and try again
    {
        printf("Patient[%d] waiting for restroom\n",personId);
        sem_wait(&restSize);
        sleep((rand() % WAIT_TIME + 1)/1000);
        sem_post(&restSize);
        goRestroom(personId);
    }
}

void checkNeedings(int personId) //Control needings while waiting
{
    if(Patient[personId].WCneed >= 100)
        goRestroom(personId);
    if(Patient[personId].hunger >= 100)
        goCafe(personId);
}

void registerPatient(int personId) //Registration function
{
    printf("Patient[%d] is arrived at registeration\n",personId);
    if(sem_trywait(&regSize) == 0)
    { 
        printf("Patient[%d] registered\n",personId);
        payMoney(REGISTRATION_COST); //paying
        sleep((rand() % REGISTRATION_TIME + 1)/1000);
        sem_post(&regSize);
    }
    else //wait and try again
    {
        printf("Patient[%d] waiting for registeration\n",personId);
        increaseNeedings(personId);
        checkNeedings(personId);
        sem_wait(&regSize);
        sleep((rand() % WAIT_TIME + 1)/1000);
        sem_post(&regSize);
        registerPatient(personId);
    }
}

void getBloodTest(int personId) //Blood lab
{
    printf("Patient[%d] is arrived at Blood Lab\n",personId);
    if(sem_trywait(&BLsize) == 0)
    { 
        printf("Patient[%d] entering Blood Lab\n",personId);
        Patient[personId].isGiveBlood = 1;
        payMoney(BLOOD_LAB_COST); //paying
        usleep(rand() % BLOOD_LAB_TIME + 1);
        sem_post(&BLsize);
    }
    else
    {
        printf("Patient[%d] waiting for Blood Lab\n",personId);
        increaseNeedings(personId);
        checkNeedings(personId);
        sem_wait(&BLsize);
        sleep((rand() % WAIT_TIME + 1)/1000);
        sem_post(&BLsize);
        getBloodTest(personId);
    }
}

void takeMedicine(int personId) //Pharmacy
{
    printf("Patient[%d] is arrived at Pharmacy\n",personId);
    if(sem_trywait(&pharSize) == 0)
    { 
        printf("Patient[%d] entering Pharmacy\n",personId);
        payMoney(rand() % PHARMACY_COST + 1); //paying
        sleep((rand() % PHARMACY_TIME + 1)/1000);
        sem_post(&pharSize);
    }
    else
    {
        printf("Patient[%d] waiting for Pharmacy\n",personId);
        increaseNeedings(personId);
        checkNeedings(personId);
        sem_wait(&pharSize);
        sleep((rand() % WAIT_TIME + 1)/1000);
        sem_post(&pharSize);
        takeMedicine(personId);
    }
    
}

void getReadyforSurgery(int personId) //Pre-surgery controls
{
    Patient[personId].surgeonNeeding = rand() % SURGEON_LIMIT + 1; //Find surgeon and nurse needing for a patient
    Patient[personId].nurseNeeding = rand() % NURSE_LIMIT + 1;
    pthread_mutex_lock(&preSurgeryMut);
    while ((Patient[personId].nurseNeeding > NURSE_NUMBER) || (Patient[personId].surgeonNeeding > SURGEON_NUMBER)) //wait for nurse or surgeon
    {
        sleep((rand() % WAIT_TIME + 1)/1000);
        printf("Patient[%d] is waiting at Operation Room\n",personId);
    }
    SURGEON_NUMBER -= Patient[personId].surgeonNeeding; 
    NURSE_NUMBER -= Patient[personId].nurseNeeding;
    pthread_mutex_unlock(&preSurgeryMut);
}

void endSurgery(int personId) //Post-surgery controls
{
    pthread_mutex_lock(&postSurgeryMut);
    SURGEON_NUMBER += Patient[personId].surgeonNeeding; //Surgeons and nurses can go to new operations
    NURSE_NUMBER += Patient[personId].nurseNeeding;
    Patient[personId].surgeonNeeding = 0;
    Patient[personId].nurseNeeding = 0;
    printf("Patient[%d]'s operation ends\n",personId);
    pthread_mutex_unlock(&postSurgeryMut);
}

void getSurgery(int personId) //Operation room
{
    printf("Patient[%d] is arrived at Surgery\n",personId);
    if(sem_trywait(&ORsize) == 0)
    { 
        printf("Patient[%d] entering Surgery\n",personId);
        getReadyforSurgery(personId); //pre_surgery
        payMoney(SURGERY_OR_COST + (Patient[personId].nurseNeeding * SURGERY_NURSE_COST) + (Patient[personId].surgeonNeeding * SURGERY_SURGEON_COST)); //paying
        sleep((rand() % SURGERY_TIME + 1)/1000);
        endSurgery(personId); //post-surgery
        sem_post(&ORsize);
    }
    else
    {
        printf("Patient[%d] waiting for Surgery\n",personId);
        increaseNeedings(personId);
        checkNeedings(personId);
        sem_wait(&ORsize);
        sleep((rand() % WAIT_TIME + 1)/1000);
        sem_post(&ORsize);
        takeMedicine(personId);
    }
}

void GPexamine(int personId) //examine the patient and send to the >> 0=pharmacy, 1=blood test, 2=surgery
{
    printf("Patient[%d] is arrived at GP office\n",personId);
    if(sem_trywait(&GPsize) == 0)
    { 
        printf("Patient[%d] entering GP office\n",personId);

        if(Patient[personId].isGiveBlood == 0 && Patient[personId].isGetSurgery == 0)
            Patient[personId].situation = rand() % 3; //0 needs medicine //1 needs surgery //2 needs blood test
        else if(Patient[personId].isGetSurgery == 1)
            Patient[personId].situation = rand() % 2 -1;  //-1 nothing //0 needs medicine 
        else if(Patient[personId].isGiveBlood == 1)
            Patient[personId].situation = rand() % 3 -1;  //-1 nothing //0 needs medicine //1 needs surgery 

        if(Patient[personId].situation == 0) // go to the pharmacy
            takeMedicine(personId);
        else if(Patient[personId].situation == 2) // go to the blood test
            getBloodTest(personId);
        else if(Patient[personId].situation == 1) // go to the surgery
            getSurgery(personId);

        sleep((rand() % GP_TIME + 1)/1000);
        sem_post(&GPsize);
    }
    else
    {
        printf("Patient[%d] waiting for GP office\n",personId);
        increaseNeedings(personId);
        checkNeedings(personId);
        sem_wait(&GPsize);
        sleep((rand() % WAIT_TIME + 1)/1000);
        sem_post(&GPsize);
        GPexamine(personId);
    }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------[Threads]

void* patientRoutine(void* args) //Main routine of a patient (a thread) 
{ 
    registerPatient(*(int*)args);

    GPexamine(*(int*)args); //may patient go blood test,surgery,pharmacy after examine

    if(Patient[*(int*)args].situation == 2 && Patient[*(int*)args].isGiveBlood == 1) //did patient get blood test?
        GPexamine(*(int*)args); //examine after blood test

    if(Patient[*(int*)args].situation == 1 && Patient[*(int*)args].isGetSurgery == 1) //did patient get surgery?
        GPexamine(*(int*)args);//examine after surgery
    
    printf("Patient[%d] leave hospital\n",*(int*)args);//leave
    free(args);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------[Main]

int main(int argc, char const *argv[])
{
    srand(time(NULL)); //random
    pthread_t thread[PATIENT_NUMBER]; //patient threads
    //Semaphores
    sem_init(&regSize, 0, REGISTRATION_SIZE);
    sem_init(&restSize, 0, RESTROOM_SIZE);
    sem_init(&cafeSize, 0, CAFE_NUMBER);
    sem_init(&pharSize, 0, PHARMACY_NUMBER);
    sem_init(&BLsize, 0, BLOOD_LAB_NUMBER);
    sem_init(&GPsize, 0, GP_NUMBER);
    sem_init(&ORsize, 0, OR_NUMBER);
    //Mutex
    pthread_mutex_init(&payMut, NULL); //controls paying
    pthread_mutex_init(&preSurgeryMut, NULL); //controls pre-surgery 
    pthread_mutex_init(&postSurgeryMut, NULL); //

    createPatients(); //creating patients

    for (int i = 0; i < PATIENT_NUMBER; i++) { //create threads
        int* size = malloc(sizeof(int));
        *size = i;
        pthread_create(&thread[i], NULL, &patientRoutine, size);     
    }
    for (int j = 0; j < PATIENT_NUMBER; j++) { //join threads
        pthread_join(thread[j], NULL);
    }

    //semaphore destoy
    sem_destroy(&regSize);
    sem_destroy(&restSize);
    sem_destroy(&cafeSize);
    sem_destroy(&pharSize);
    sem_destroy(&BLsize);
    sem_destroy(&GPsize);
    sem_destroy(&ORsize);
    //Mutex destroy
    pthread_mutex_destroy(&payMut);
    pthread_mutex_destroy(&preSurgeryMut);
    pthread_mutex_destroy(&postSurgeryMut);

    printf("\nTotal wallet of hospital = %d\n\n",HOSPITAL_WALLET); //Total money

    return 0;
}


