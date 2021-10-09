#include "NarrowBridge.h"

//---------------------------------------------------------------------------------------------------------------------------------------
//Funcion que hace el paso de los vehiculos sobre el puente de semaforos
void crossBridge(int dir, int vel, int id){
    int start = dir == 1 ? 0 : bridgeLen - 1; // Se define el inicio del puente en base a la direccion
    int end = dir == -1 ? -1 : bridgeLen;     // Se define el final del puente en base a la direccion

    lock(&bridge[start]);                     // Bloquea la primera locacion a avanzar en el puente
    lock(&vehiclesOnBridgeMutex);
    ++vehiclesOnBridge;                       // Aumenta el contador sobre los vehiculos sobre el puente
    unlock(&vehiclesOnBridgeMutex);
    printf("V %d en pos %d en la direccion: %d\n", id, start, dir);
    for(int i = start + dir; i != end; i += dir){
        printf("V %d en pos %d en la direccion: %d\n", id, i, dir);
        sleep(vel);
        lock(&bridge[i]);
        unlock(&bridge[i - dir]);
    }
    unlock(&bridge[end - dir]);               // Desbloquea la ultima locacion a avanzar en el puente
    lock(&vehiclesOnBridgeMutex);
    --vehiclesOnBridge;                       // Decrementa el contador sobre los vehiculos sobre el puente
    unlock(&vehiclesOnBridgeMutex);

    if(dir == 1 && (mode == 3 || mode == 2)){                // Si el modo es de los oficiales decrementa el contador de carros que pasaron el puente
        lock(&vehiclesLCounterMutex);
        --vehiclesLCounter;
        unlock(&vehiclesLCounterMutex);
    } else if (dir == -1 && (mode == 3 || mode == 2)){
        lock(&vehiclesRCounterMutex);
        --vehiclesRCounter;
        unlock(&vehiclesRCounterMutex);
    }
    if (mode == 2) {                  // Si el modo es el de los semaforos se decrementa el contador de carros pendientes por crear 
        lock(&totalVehiclesMutex);
        --totalVehicles;
        unlock(&totalVehiclesMutex);
    }

    printf("Vehiculo %d salio.... %d\n --------------- Vehiculos en el puente %d\n", id, dir, vehiclesOnBridge);
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Funcion para el ultimo vehiculo que deja el puente para el modo de carnage
void leavingBridgeCarnage(int dir){
    if(((dir == -1) || vehicleQuantityR == 0) && vehiclesOnBridge == 0){  // Si es el ultimo en el puente con direccion de derecha a izquierda
        if (ambulanceR){ // Si habia una ambulancia apaga la senal
            lock(&ambulanceRMutex);
            ambulanceR = 0;
            unlock(&ambulanceRMutex);
        }
        pthread_cond_broadcast(&isSafeLcon); // Despierta a los carros del otro lado
    }else if(((dir == 1) || vehicleQuantityL == 0) && vehiclesOnBridge == 0){  // Si es el ultimo en el puente con direccion de izquierda a derecha
        if (ambulanceL){ // Si habia una ambulancia apaga la senal
            lock(&ambulanceLMutex);
            ambulanceL = 0;
            unlock(&ambulanceLMutex);
        }
        pthread_cond_broadcast(&isSafeRcon); // Despierta a los carros del otro lado
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Funcion para el ultimo vehiculo que deja el puente para el modo de semaforos
void leavingBridgeTrafficPolice(int dir){
    if((dir == -1) && vehiclesOnBridge == 0){ // Si es el ultimo en el puente con direccion de derecha a izquierda
        printf("\nSaLiendo del puente R\n\n");
        lock(&bridgeDirRealMutex);
        bridgeDirReal = 1;  // Sede al otro lado la direccion real del puente
        unlock(&bridgeDirRealMutex);
        pthread_cond_signal(&policeLeftCond); // Despierta al oficial de la izquierda para que deje pasar carros
    }else if((dir == 1) && vehiclesOnBridge == 0){ // Si es el ultimo en el puente con direccion de izquierda a derecha
        printf("\nSaLiendo del puente L\n\n");
        lock(&bridgeDirRealMutex);
        bridgeDirReal = -1;  // Sede al otro lado la direccion real del puente
        unlock(&bridgeDirRealMutex);
        pthread_cond_signal(&policeRightCond); // Despierta al oficial de la derecha para que deje pasar carros
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Funcion para el ultimo vehiculo que deja el puente para el modo de semaforos
void leavingBridgeSemaphore(int dir) {
    if(dir == -1 && vehiclesOnBridge == 0){ // Si soy el ultimo vehiculo hacia la izquierda
        if (ambulanceR){                    // y soy una ambulancia
            lock(&ambulanceRMutex);
            ambulanceR = 0;                 // Apago bandera de ambulancia
            unlock(&ambulanceRMutex);
            if(bridgeDir == dir){           // Doy paso al lado que este en verde
                pthread_cond_broadcast(&isSafeRcon);
            } else if (vehiclesLCounter > 0){
                lock(&bridgeDirRealMutex);
                bridgeDirReal *= -1;
                unlock(&bridgeDirRealMutex);
                pthread_cond_broadcast(&isSafeLcon);
            }
            return;
        }
        if (vehiclesLCounter > 0){
            lock(&bridgeDirRealMutex);          // Si no soy una ambulancia
            bridgeDirReal *= -1;                // Da paso al otro lado del puente
            unlock(&bridgeDirRealMutex);
            pthread_cond_broadcast(&isSafeLcon);
        }
    }else if(dir == 1 && vehiclesOnBridge == 0){ // Si soy el ultimo vehiculo hacia la derecha
        if (ambulanceL){                         // y soy una ambulancia
            lock(&ambulanceLMutex);
            ambulanceL = 0;                      // Apago bandera de ambulancia
            unlock(&ambulanceLMutex);
            if(bridgeDir == dir){                // Doy paso al lado que este en verde
                pthread_cond_broadcast(&isSafeLcon);
            } else if (vehiclesRCounter > 0){
                lock(&bridgeDirRealMutex);
                bridgeDirReal *= -1;
                unlock(&bridgeDirRealMutex);
                pthread_cond_broadcast(&isSafeRcon);
            }
            return;
        }
        if(vehiclesRCounter > 0){
            lock(&bridgeDirRealMutex);            // Si no soy una ambulancia
            bridgeDirReal *= -1;                  // doy paso al otro lado del puente
            unlock(&bridgeDirRealMutex);
            pthread_cond_broadcast(&isSafeRcon);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------
//Funcion que le dice al vehiculo si es seguro pasar para el modo de carnage
int isSafeCarnage(int dir){
    if(vehiclesOnBridge == 0){ // Si no hay vehiculos puedo tomar la direccion del puente
        lock(&bridgeDirMutex);
        bridgeDir = dir;
        unlock(&bridgeDirMutex);
    }

    if(dir == bridgeDir){  // Si puedo pasar
        if((dir == 1) && !ambulanceR){ // Si no hay ambulancias al inicio del otro lado
            return 1;
        }else if((dir == -1) && !ambulanceL){ // Si no hay ambulancias al inicio del otro lado
            return 1;
        }

        return 0;
    }else{
        return 0;
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------
//Funcion que le dice al vehiculo si es seguro pasar para el modo de oficiales
int isSafeTrafficPolice(int dir, int ambulance, int position){
    if(dir == bridgeDirReal){   // Si no vienen carros en la direccion contraria
        if((dir == 1 && K1Temp > 0)){ // Si todavia quedan campos para pasar de la izquierda
            lock(&K1TempMutex);
            --K1Temp;
            unlock(&K1TempMutex);
            return 1;
        }else if (dir == -1 && K2Temp > 0){   // Si todavia quedan campos para pasar de la derecha
            lock(&K2TempMutex);
            --K2Temp;
            unlock(&K2TempMutex);
            return 1;
        }else if(ambulance && position == 1){ // Si es una ambulancia en la primera posicion ignora que ya no hayan campos para pasar
            if((dir == 1 && K1Temp == 0) || (dir == -1 && K2Temp == 0)){
                return 1;
            }
        }
        return 0;
    }else{
        return 0;
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------
//Funcion que le dice al vehiculo si es seguro pasar para el modo de semaforos
int isSafeSemaphore(int dir, int ambulance){
    if(dir == bridgeDirReal){   // Si no vienen carros en la direccion contraria
        if(dir == bridgeDir){   // Si el semaforo esta en verde
            if((dir == 1) && !ambulanceR){ // Si no hay ambulancias del otro lado
                return 1;   // Puedo pasar
            }else if((dir == -1) && !ambulanceL){ // Si no hay ambulancias del otro lado
                return 1;   // Puedo pasar
            }
        } else if(ambulance){   // Si soy una ambulancia
            return 1;   // Puedo pasar
        }

        return 0;
    }else{
        return 0;
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------
//Funcion que representa un vehiculo, existe la posibilidad de que sea ambulancia o un simple carro
void *vehicle(void *direction){
    int dir = *(int *)direction; // Define la direccion del carro
    int id = 0;

    if(!firstCar && mode == 3){             // Si es el primero carro del modo 3 agarra las direcciones del puente
        firstCar = 1;
        lock(&bridgeDirRealMutex);
        bridgeDirReal = dir;
        unlock(&bridgeDirRealMutex);
        lock(&bridgeDirMutex);
        bridgeDir = dir;
        unlock(&bridgeDirMutex);
    }

    if (dir == 1){                          //Si es de izquierda a derecha aumento el contador de la cola izquierda
        lock(&vehicleQuantityLMutex);
        ++vehicleQuantityL;
        unlock(&vehicleQuantityLMutex);
        lock(&vehiclesLCounterAscMutex);
        ++vehiclesLCounterAsc;
        unlock(&vehiclesLCounterAscMutex);
        id = vehiclesLCounterAsc;
    }else{                                  //Si es de derecha a izquierda aumento el contador de la cola derecha
        lock(&vehicleQuantityRMutex);
        ++vehicleQuantityR;
        unlock(&vehicleQuantityRMutex);
        lock(&vehiclesRCounterAscMutex);
        ++vehiclesRCounterAsc;
        unlock(&vehiclesRCounterAscMutex);
        id = vehiclesRCounterAsc;
    }

    printf("Vehiculo %d creado.... %d\n", id, dir);

    int position = dir == 1 ? vehicleQuantityL : vehicleQuantityR;      // Define la posicion en la cola del carro
    int isAmb = rand() % 100;
    int velocity = 0;
    if (dir == -1){
        velocity = rand() % ((vehicleVelocitySupR - vehicleVelocityInfR) + 1) + vehicleVelocityInfR;
    }else{
        velocity = rand() % ((vehicleVelocitySupL - vehicleVelocityInfL) + 1) + vehicleVelocityInfL;
    }
    int ambulance = 0;
    velocity = T / velocity;                                            // Calcula la velocidad del carro

    if (isAmb <= 5){                                                    // 5% de probabilidad de que el vehiculo sea ambulancia
        ambulance = 1;
        printf("Vehiculo %d es una ambulancia de lado: %d\n", id, dir);
        if (position == 1){                                             // Si la ambulancia esta al principio de alguna cola se levanta la bandera
            lock(&ambulanceLMutex);
            ambulanceL = dir == 1 ? 1 : ambulanceL;
            unlock(&ambulanceLMutex);
            lock(&ambulanceRMutex);
            ambulanceR = dir == -1 ? 1 : ambulanceR;
            unlock(&ambulanceRMutex);
        }
    }

    int isSafe = 0;                                                     //Pregunta si es seguro pasar dependiendo del modo del puente
    if (mode == 1){
        isSafe = isSafeCarnage(dir);
    }else if (mode == 2){
        isSafe = isSafeSemaphore(dir, ambulance);
    }else if (mode == 3){
        isSafe = 0;
        if(position == 1){
            dir == 1 ? pthread_cond_signal(&policeLeftCond) : pthread_cond_signal(&policeRightCond);
        }
    }

    while (!isSafe){                                                // Si es seguro entra al puente y si no lo es se duerme el carro
        int posLock = dir == 1 ? 0 : bridgeLen - 1;

        lock(&bridge[posLock]);
        if (dir == 1){
            pthread_cond_wait(&isSafeLcon, &bridge[posLock]);
            position = vehicleQuantityL;
        }else{
            pthread_cond_wait(&isSafeRcon, &bridge[posLock]);
            position = vehicleQuantityR;
        }
        unlock(&bridge[posLock]);

        if (mode == 1){
            isSafe = isSafeCarnage(dir);
        }else if (mode == 2){
            isSafe = isSafeSemaphore(dir, ambulance);
        }else if (mode == 3){
            isSafe = isSafeTrafficPolice(dir, ambulance, position);
        }
    }

    if (dir == 1){                                                 // Saca el carro de la cola de carros de la izquierda
        lock(&vehicleQuantityLMutex);
        --vehicleQuantityL;
        unlock(&vehicleQuantityLMutex);
    }else{                                                         // Saca el carro de la cola de carros de la derecha
        lock(&vehicleQuantityRMutex);
        --vehicleQuantityR;
        unlock(&vehicleQuantityRMutex);
    }

    crossBridge(dir, velocity, id);                     // Comienza a pasar el puente

    if (mode == 1){                                                 // Toma acciones una vez que un carro sale del puente
        leavingBridgeCarnage(dir);
    }else if (mode == 2){
        leavingBridgeSemaphore(dir);
    }else if (mode == 3){
        leavingBridgeTrafficPolice(dir);
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------
//Funcion para crear a los vehiculos del lado derecha
void *createVehiculeR(void *args){
    int dir = -1;
    for(int i=0;i<vehiclesR;i++){
        int time =  (-1 * expAverageR) * log(1 - (double)rand() / (double)RAND_MAX);    //Calculo para generar la media de la distribucion exponencial
        sleep(time);
        pthread_create(&vehiclesPR[i],NULL,vehicle,&dir); // Crea el carro
    }

    for(int i=0;i<vehiclesR;i++){
      pthread_join(vehiclesPR[i],NULL);
    }

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
//Funcion para crear a los vehiculos del lado izquierdo
void *createVehiculeL(){
    for(int i=0;i<vehiclesL;i++){
        int dir = 1;
        int time =  (-1 * expAverageL) * log(1 - (double)rand() / (double)RAND_MAX);    //Calculo para generar la media de la distribucion exponencial
        sleep(time);
        pthread_create(&vehiclesPL[i],NULL,vehicle,&dir); // Crea el carro
    }

    for(int i=0;i<vehiclesL;i++){
        pthread_join(vehiclesPL[i],NULL);
    }

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Hilo principal del modo carnage
void *carnage(void *args){

    pthread_t createVehiculeRThread;
    pthread_t createVehiculeLThread;

    pthread_create(&createVehiculeRThread,NULL,createVehiculeR,NULL);   // Crea hilo de creacion de carros por la derecha
    pthread_create(&createVehiculeLThread,NULL,createVehiculeL,NULL);   // Crea hilo de creacion de carros por la izquierda

    pthread_join(createVehiculeRThread,NULL);
    pthread_join(createVehiculeLThread,NULL);

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Funcion del oficial de la izquierda: controla el paso de los carros (hilos) de la izquierda
void *policeLeft(void *args){
    while (vehiclesLCounter) {
        printf("Se durmio policia IZQUIERDO\n");
        pthread_cond_wait(&policeLeftCond, &policeLeftMutex); // No ocupo estar siempre despierto, me despiertan los carros o el otro oficial
        lock(&K1TempMutex);
        K1Temp = K1;                                          // Reinicia el contador para los carros que pueden pasar hacia un lado
        unlock(&K1TempMutex);
        printf("Se desperto policia IZQUIERDO\n");
        if(vehiclesOnBridge == 0 && bridgeDir == 1 && bridgeDirReal == 1){ // Verifica si es posible que los carros pasen
            if(vehicleQuantityL > 0){                                      // Si hay un carrro en la cola hace K + 1 senales
                for(int i = 0; i < K1 + 1; i++){
                    pthread_cond_signal(&isSafeLcon);                       // Despierta a  K + 1 carros
                }
                lock(&bridgeDirMutex);
                bridgeDir = -1;
                unlock(&bridgeDirMutex);
            }else{                                                          // Si no hay carros del lado que toca le pasa el turno al otro oficial
                lock(&bridgeDirMutex);
                bridgeDir = -1;
                unlock(&bridgeDirMutex);
                lock(&bridgeDirRealMutex);
                bridgeDirReal = -1;
                unlock(&bridgeDirRealMutex);
                pthread_cond_signal(&policeRightCond);                       //Despierta al otro policia
            }
        }
    }

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Funcion del oficial de la derecha: controla el paso de los carros (hilos) de la derecha
void *policeRight(void *args){
    while (vehiclesRCounter) {
        printf("Se durmio policia DERECHO\n");
        pthread_cond_wait(&policeRightCond, &policeRightMutex); // No ocupo estar siempre despierto, me despiertan los carros o el otro oficial
        lock(&K2TempMutex);
        K2Temp = K2;                                            // Reinicia el contador para los carros que pueden pasar hacia un lado
        unlock(&K2TempMutex);
        printf("Se desperto policia DERECHO\n");
        if(vehiclesOnBridge == 0 && bridgeDir == -1 && bridgeDirReal == -1){ // Verifica si es posible que los carros pasen
            if(vehicleQuantityR > 0){                                        // Si hay un carrro en la cola hace K + 1 senales
                for(int i = 0; i < K2 + 1; i++){
                    pthread_cond_signal(&isSafeRcon);                       // Despierta a  K + 1 carros
                }
                lock(&bridgeDirMutex);
                bridgeDir = 1;
                unlock(&bridgeDirMutex);
            }else{                                                          // Si no hay carros del lado que toca le pasa el turno al otro oficial
                lock(&bridgeDirMutex);
                bridgeDir = 1;
                unlock(&bridgeDirMutex);
                lock(&bridgeDirRealMutex);
                bridgeDirReal = 1;
                unlock(&bridgeDirRealMutex);
                pthread_cond_signal(&policeLeftCond);                       //Despierta al otro policia
            }
        }
    }

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Hilo principal del modo semaforo
void *trafficPolice(void *args){

    pthread_t createVehiculeRThread;
    pthread_t createVehiculeLThread;

    pthread_t policeRightThread;
    pthread_t policeLeftThread;

    K1Temp = K1;
    K2Temp = K2;

    vehiclesLCounter = vehiclesL;
    vehiclesRCounter = vehiclesR;

    pthread_create(&policeLeftThread,NULL,policeLeft,NULL);     // Crea hilo de para el oficial de los carros de la izquierda
    pthread_create(&policeRightThread,NULL,policeRight,NULL);   // Crea hilo de para el oficial de los carros de la derecha

    pthread_create(&createVehiculeLThread,NULL,createVehiculeL,NULL); // Crea hilo de creacion de carros por la izquierda
    pthread_create(&createVehiculeRThread,NULL,createVehiculeR,NULL); // Crea hilo de creacion de carros por la derecha

    pthread_join(createVehiculeLThread,NULL);
    pthread_join(createVehiculeRThread,NULL);

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Hilo para la funcionalidad de cambio de semaforo
void *semaphoreChange(void *args) {
    lock(&bridgeDirRealMutex);  // Comienza la direccion del puente hacia la derecha
    bridgeDirReal = 1;
    unlock(&bridgeDirRealMutex);

    while (totalVehicles) { // Mientras queden carros sin pasar
        printf("--------------Vehiculos pendientes por salir: %d-----------------\nBridgeDir = %d --------------- BridgeDirReal = %d\n", totalVehicles, bridgeDir, bridgeDirReal);
        lock(&bridgeDirMutex);  // Pone el semaforo izquierdo en verde
        bridgeDir = 1;
        unlock(&bridgeDirMutex);

        printf("\nSemaforo izquierdo en verde\n\n");
        if (vehiclesOnBridge == 0) {
            pthread_cond_broadcast(&isSafeLcon);    // Da paso si hay carros de este lado dormidos
        }
        sleep(greenLightTimeL);     // Espera el tiempo de luz verde izquierdo


        lock(&bridgeDirMutex);  // Pone el semaforo derecho en verde
        bridgeDir = -1;
        unlock(&bridgeDirMutex);

        printf("\nSemaforo derecho en verde\n\n");
        if (vehiclesOnBridge == 0) {
            pthread_cond_broadcast(&isSafeRcon);    // Da paso si hay carros de este lado dormidos
        }
        sleep(greenLightTimeR);     // Espera el tiempo de luz verde derecho
    }

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Hilo principal del modo semaforo
void *semaphore(void *args) {

    pthread_t createVehiculeRThread;
    pthread_t createVehiculeLThread;
    pthread_t createSemaphoreChangeThread;

    totalVehicles = vehiclesL + vehiclesR;
    vehiclesLCounter = vehiclesL;
    vehiclesRCounter = vehiclesR;

    pthread_create(&createSemaphoreChangeThread,NULL,semaphoreChange,NULL);    // Crea hilo de cambio de semaforos
    pthread_create(&createVehiculeRThread,NULL,createVehiculeR,NULL);   // Crea hilo de creacion de carros por la derecha
    pthread_create(&createVehiculeLThread,NULL,createVehiculeL,NULL);   // Crea hilo de creacion de carros por la izquierda

    pthread_join(createVehiculeRThread,NULL);
    pthread_join(createVehiculeLThread,NULL);

    pthread_exit(0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Funcion que lee las variables del archivo de configuracion
// y las inicializa
void readConfiguration(){
    char const* const fileName = "config.txt";
    FILE* file = fopen(fileName, "r");  // Abre archivo de configuracion
    char line[256];
    const char s[4] = " = ";
    char *token;

    fgets(line, sizeof(line), file);    // Tamaño del puente
    strtok(line, s);
    token = strtok(NULL, s);
    bridgeLen = atoi(token);

    fgets(line, sizeof(line), file);    // Media exponencial izquierda
    strtok(line, s);
    token = strtok(NULL, s);
    expAverageL = atoi(token);

    fgets(line, sizeof(line), file);    // Media exponencial derecha
    strtok(line, s);
    token = strtok(NULL, s);
    expAverageR = atoi(token);

    fgets(line, sizeof(line), file);    // Velocidad inferior izquierda
    strtok(line, s);
    token = strtok(NULL, s);
    vehicleVelocityInfL = atoi(token);

    fgets(line, sizeof(line), file);    // Velocidad inferior derecha
    strtok(line, s);
    token = strtok(NULL, s);
    vehicleVelocityInfR = atoi(token);

    fgets(line, sizeof(line), file);    // Velocidad superior izquierda
    strtok(line, s);
    token = strtok(NULL, s);
    vehicleVelocitySupL = atoi(token);

    fgets(line, sizeof(line), file);    // Velocidad superior derecha
    strtok(line, s);
    token = strtok(NULL, s);
    vehicleVelocitySupR = atoi(token);

    fgets(line, sizeof(line), file);    // Cantidad K1 oficiales de transito
    strtok(line, s);
    token = strtok(NULL, s);
    K1 = atoi(token);

    fgets(line, sizeof(line), file);    // Cantidad K2 oficiales de transito
    strtok(line, s);
    token = strtok(NULL, s);
    K2 = atoi(token);

    fgets(line, sizeof(line), file);    // Tiempo de luz verde izquierda
    strtok(line, s);
    token = strtok(NULL, s);
    greenLightTimeL = atoi(token);

    fgets(line, sizeof(line), file);    // Tiempo de luz verde derecha
    strtok(line, s);
    token = strtok(NULL, s);
    greenLightTimeR = atoi(token);

    fgets(line, sizeof(line), file);    // Cantidad de vehiculos por la izquierda
    strtok(line, s);
    token = strtok(NULL, s);
    vehiclesL = atoi(token);

    fgets(line, sizeof(line), file);    // Cantidad de vehiculos por la derecha
    strtok(line, s);
    token = strtok(NULL, s);
    vehiclesR = atoi(token);

    fclose(file);
}

//---------------------------------------------------------------------------------------------------------------------------------------
int main(){
    readConfiguration(); // Lee el archivo de configuracion

    srand(time(NULL));   // Seed para el random

    bridge = (pthread_mutex_t*) malloc(bridgeLen*sizeof(pthread_mutex_t));

    pthread_cond_init(&isSafeLcon,NULL);    // Inicializacion de mutex y condiciones para semaforos de SO
    pthread_cond_init(&isSafeRcon,NULL);
    pthread_mutex_init(&ambulanceLMutex, NULL);
    pthread_mutex_init(&ambulanceRMutex, NULL);
    pthread_mutex_init(&vehiclesOnBridgeMutex, NULL);
    pthread_mutex_init(&vehicleQuantityLMutex, NULL);
    pthread_mutex_init(&vehicleQuantityRMutex, NULL);
    pthread_mutex_init(&vehiclesLCounterAscMutex, NULL);
    pthread_mutex_init(&vehiclesRCounterAscMutex, NULL);
    pthread_mutex_init(&bridgeDirMutex, NULL);
    pthread_mutex_init(&bridgeDirRealMutex, NULL);

    for(int i=0;i<bridgeLen;i++){   // Inicializacion del puente
        pthread_mutex_init(&bridge[i],NULL);
    }

    vehiclesPL = (pthread_t*) malloc(vehiclesL*sizeof(pthread_t)); // Hilos a crear para los carros de la izquierda
    vehiclesPR = (pthread_t*) malloc(vehiclesR*sizeof(pthread_t)); // Hilos a crear para los carros de la derecha

    // Variables que se usan como boolean en el programa de manera general
    ambulanceR = 0;
    ambulanceL = 0;
    vehicleQuantityL = 0;
    vehicleQuantityR = 0;
    vehiclesOnBridge = 0;
    bridgeDir = 0;
    firstCar = 0;
    vehiclesLCounterAsc = 0;
    vehiclesRCounterAsc = 0;

    // Menu para escoger el modo de administras el puente, 1- Carnage, 2- Semaforos, 3- Oficiales
    printf("\n                   NarrowBridge\n");
    printf("\n");
    printf("Escoja el modo a ejecutar:\n");
    printf("\n");
    printf("1) Carnage.\n");
    printf("2) Semaforos.\n");
    printf("3) Oficiales de transito.\n");
    printf("\n");
    printf("-> ");
    scanf("%d", &mode);

    if (mode == 1){ // Inicializa el modo carnage
        printf("\n");
        pthread_t carnageThread;

        pthread_create(&carnageThread,NULL,carnage,NULL);

        pthread_join(carnageThread,NULL);
    } else if (mode == 2){ // Inicializa el modo semaforo
        printf("\n");
        pthread_t semaphoreThread;

        pthread_mutex_init(&totalVehiclesMutex,NULL);
        pthread_mutex_init(&vehiclesLCounterMutex, NULL);
        pthread_mutex_init(&vehiclesRCounterMutex, NULL);

        pthread_create(&semaphoreThread,NULL,semaphore,NULL);

        pthread_join(semaphoreThread,NULL);
    } else if (mode == 3){ // Inicializa el modo oficiales de transito
        printf("\n");
        pthread_t trafficPoliceThread;

        pthread_mutex_init(&policeLeftMutex, NULL);
        pthread_mutex_init(&policeRightMutex, NULL);

        pthread_cond_init(&policeLeftCond,NULL);
        pthread_cond_init(&policeRightCond,NULL);

        pthread_mutex_init(&K1TempMutex, NULL);
        pthread_mutex_init(&K2TempMutex, NULL);

        pthread_mutex_init(&vehiclesLCounterMutex, NULL);
        pthread_mutex_init(&vehiclesRCounterMutex, NULL);

        pthread_create(&trafficPoliceThread,NULL,trafficPolice,NULL);

        pthread_join(trafficPoliceThread,NULL);
    } else{
        printf("\nOpcion invalida\n");
    }
    //pthread_exit(0);

    return 0;
}
