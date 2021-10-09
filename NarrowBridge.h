#ifndef HEADER
# define HEADER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#define T 500
#define lock pthread_mutex_lock
#define unlock pthread_mutex_unlock

int bridgeLen;
int expAverageL,expAverageR;
int vehicleVelocityInfL,vehicleVelocityInfR;
int vehicleVelocitySupL,vehicleVelocitySupR;
int K1,K2;
int K1Temp,K2Temp;
int greenLightTimeL,greenLightTimeR;
int vehiclesL,vehiclesR;
int totalVehicles;
int vehiclesLCounter,vehiclesRCounter; 
int vehiclesLCounterAsc,vehiclesRCounterAsc; 
int ambulanceL, ambulanceR; 
int vehiclesOnBridge;
int vehicleQuantityL, vehicleQuantityR;
int bridgeDir;
int mode;
int bridgeDirReal;
int firstCar;
pthread_t *vehiclesPL;
pthread_t *vehiclesPR;
pthread_mutex_t *bridge;
pthread_mutex_t policeLeftMutex, policeRightMutex;
pthread_mutex_t K1TempMutex, K2TempMutex;
pthread_mutex_t totalVehiclesMutex;
pthread_mutex_t vehiclesLCounterMutex, vehiclesRCounterMutex;
pthread_mutex_t vehiclesLCounterAscMutex, vehiclesRCounterAscMutex;
pthread_mutex_t ambulanceLMutex, ambulanceRMutex;
pthread_mutex_t vehiclesOnBridgeMutex;
pthread_mutex_t vehicleQuantityLMutex, vehicleQuantityRMutex;
pthread_mutex_t bridgeDirMutex;
pthread_mutex_t bridgeDirRealMutex;
pthread_cond_t isSafeLcon, isSafeRcon;
pthread_cond_t policeRightCond, policeLeftCond;

void crossBridge(int dir, int vel, int id);
void leavingBridgeCarnage(int dir);
void leavingBridgeTrafficPolice(int dir);
void leavingBridgeSemaphore(int dir);
int isSafeCarnage(int dir);
int isSafeTrafficPolice(int dir, int ambulance, int position);
int isSafeSemaphore(int dir, int ambulance);
void *vehicle(void *direction);
void *createVehiculeR(void *args);
void *createVehiculeL();
void *carnage(void *args);
void *policeLeft(void *args);
void *policeRight(void *args);
void *trafficPolice(void *args);
void *semaphoreChange(void *args);
void *semaphore(void *args);
void readConfiguration();
int main();

#endif
