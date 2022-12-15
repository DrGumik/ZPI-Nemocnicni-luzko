/*
ADR: raspberrypi.local
SSH: username: pi
     pass: 1234

Compile: gcc postel.c -o postel -I/usr/local/include -L/usr/local/lib -lwiringPi -lmosquitto -lm
Run: ./postel

Libs: 
    WiringPi
    Math
    Mosquitto
    StdLib, StdIO
*/

/*
    Knihovny
*/
#include <stdlib.h>
#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <mosquitto.h>
#include <math.h>

/*
  Konstanty GPIO pinů pro ostatní senzory
*/
#define VIBRATION_PIN 0 // GPIO 17
#define PIR_PIN 2 // GPIO 27
#define IR_PIN 3 // GPIO 22

/*
    Konstanty a proměnné pro gyroskop
*/
#define Device_Address 0x68
#define PWR_MGMT_1   0x6B
#define SMPLRT_DIV   0x19
#define CONFIG       0x1A
#define GYRO_CONFIG  0x1B
#define INT_ENABLE   0x38
#define ACCEL_XOUT_H 0x3B
#define ACCEL_YOUT_H 0x3D
#define ACCEL_ZOUT_H 0x3F
#define GYRO_XOUT_H  0x43
#define GYRO_YOUT_H  0x45
#define GYRO_ZOUT_H  0x47

int fd;
float accAngleX, accAngleY;

/////////////////////////////////////////////////////////////////////
/*
    Obsluha MPU6050 gyroskopu, inicializace a získání dat
*/
// Inicializace
void MPU6050_Init(){
  wiringPiI2CWriteReg8 (fd, SMPLRT_DIV, 0x07);    /* Write to sample rate register */
  wiringPiI2CWriteReg8 (fd, PWR_MGMT_1, 0x01);    /* Write to power management register */
  wiringPiI2CWriteReg8 (fd, CONFIG, 0);           /* Write to Configuration register */
  wiringPiI2CWriteReg8 (fd, GYRO_CONFIG, 24);     /* Write to Gyro Configuration register */
  wiringPiI2CWriteReg8 (fd, INT_ENABLE, 0x01);    /*Write to interrupt enable register */
}
// Získání surových dat
short read_raw_data(int addr){
  short high_byte,low_byte,value;
  high_byte = wiringPiI2CReadReg8(fd, addr);
  low_byte = wiringPiI2CReadReg8(fd, addr+1);
  value = (high_byte << 8) | low_byte;
  return value;
}
// Získání reálných dat ze surových (přepočet apod...)
void gyroskop(float elapsedTime) {
  /* Čtení surových dat MPU6050, přepočet a škálování dat */
  float AccX = read_raw_data(ACCEL_XOUT_H) / 16384.0;
  float AccY = read_raw_data(ACCEL_YOUT_H) / 16384.0;
  float AccZ = read_raw_data(ACCEL_ZOUT_H) / 16384.0;

  // určení úhlu x a y trigonometrickou metodou a ošetření hodnot odečtením odchylky
  accAngleX = (atan(AccY / sqrt(pow(AccX, 2) + pow(AccZ, 2))) * 180 / 3.14159) - 0; // AccErrorX ~ (+76.653)
  accAngleY = (atan(-1 * AccX / sqrt(pow(AccY, 2) + pow(AccZ, 2))) * 180 / 3.14159) + 0; // AccErrorY ~ (-4.878)

  if (accAngleX < 0) {
    accAngleX = accAngleX * -1;
  }

  if (accAngleY < 0) {
    accAngleY = accAngleY * -1;
  }
}

/////////////////////////////////////////////////////////////////////
/*
    Obsluha senzoru vibrací
*/
int vibracniSenzor(int *zaznamenanaVibrace, int *posledniVibrace) {
  int vibrace = digitalRead(VIBRATION_PIN);
  
  if (*posledniVibrace != vibrace) {
    *zaznamenanaVibrace = vibrace;
  }
  
  *posledniVibrace = vibrace;
}

/////////////////////////////////////////////////////////////////////
/*
    Obsluha PIR senzoru
*/
void pirSenzor(int *pohybPacienta) {
  *pohybPacienta = digitalRead(PIR_PIN);
}

/////////////////////////////////////////////////////////////////////
/*
    Obsluha IR zavory
    (Nahrada za porouchany tenzometr)
*/
void irSenzor(int *leziNaPosteli) {
  *leziNaPosteli = digitalRead(IR_PIN);
}

/////////////////////////////////////////////////////////////////////
/*
    Programový milisekundový delay
*/
void ms_delay(int val){
  int i,j;

  for(i=0;i<=val;i++)
    for(j=0;j<1200;j++);
}

/////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////
/*
    Hlavní funkce programu
*/
int main(){
  /*
    Proměnné času
  */
  float elapsedTime, currentTime, previousTime;
  /*
    Proměnné pro ostatní senzory a mqtt
  */
  int pohybPacienta = 0;
  int dechPacienta = 0;
  int leziNaPosteli = 0;
  int zaznamenanaVibrace = 0;
  int posledniVibrace = 0;
  int recconect;
  struct mosquitto * mqtt;


  /*
    Inicializace GPIO
  */
  wiringPiSetup();
  pinMode(VIBRATION_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(IR_PIN, INPUT);


  /*
    Inicializace I2C a gyroskopu
  */
  printf("Nastavuji I2C a gyroskop!\n");
  fd = wiringPiI2CSetup(Device_Address);   /*Initializes I2C with device Address*/
  MPU6050_Init();                          /* Initializes MPU6050 */

  /*
    Inicializace a připojení MQTT
  */
  printf("Nastavuji MQTT!\n");
  mosquitto_lib_init();

  mqtt = mosquitto_new("publisher-test", true, NULL);
  recconect = mosquitto_connect(mqtt, "localhost", 1883, 60);

  // Pokud se nepodaří připojit, ukončí se program
  if(recconect != 0){
    printf("Client could not connect to broker! Error Code: %d\n", recconect);
    mosquitto_destroy(mqtt);
    return -1;
  }
  else {
    printf("RPI uspesne pripojeno k MQTT serveru!\n");
  }

  /*
    Hlavní cyklus programu
  */
  printf("Spoustim mereni!\n");
  while(1)
  {
      /*
        Získání úhlu osy Z z akcelerometru a gyroskopu
        Parametry se předávají jako reference
      */
      previousTime = currentTime; // získání času z předchozí smyčky
      currentTime = millis(); // aktuální čas
      elapsedTime = (currentTime - previousTime) / 1000;  // vypočítá čas, za který se senzor pootočil o určitý úhel

      gyroskop(elapsedTime);

      // Získání informací ze senzoru vibrací
      vibracniSenzor(&zaznamenanaVibrace, &posledniVibrace);

      // Získání informací ze PIR senzoru
      pirSenzor(&pohybPacienta);

      // Získání informací z IR zavory
      irSenzor(&leziNaPosteli);
      
      /*
        Přetypování dat do stringů a odeslání na MQTT server
      */
      char angleXStringBfr[10];
      sprintf(angleXStringBfr, "%6.2f", accAngleX);
      char angleYStringBfr[10];
      sprintf(angleYStringBfr, "%6.2f", accAngleY);
      char vibraceStringBfr[1];
      sprintf(vibraceStringBfr, "%d", zaznamenanaVibrace);
      char pohybStringBfr[1];
      sprintf(pohybStringBfr, "%d", pohybPacienta);
      char leziStringBfr[1];
      sprintf(leziStringBfr, "%d", leziNaPosteli);

      mosquitto_publish(mqtt, NULL, "postel/gyroskopX", 6, angleXStringBfr, 0, false);
      mosquitto_publish(mqtt, NULL, "postel/gyroskopY", 6, angleYStringBfr, 0, false);
      mosquitto_publish(mqtt, NULL, "postel/vibrace", 1, vibraceStringBfr, 0, false);
      mosquitto_publish(mqtt, NULL, "postel/pir", 1, pohybStringBfr, 0, false);
      mosquitto_publish(mqtt, NULL, "postel/IR", 1, leziStringBfr, 0, false);

      /*
        Výpis dat do terminálu
      */
      system("clear");
      printf("\n Uhel X = %s", angleXStringBfr);
      printf("\n Uhel Y = %s", angleYStringBfr);
      printf("\n Vibrace: %s", vibraceStringBfr);
      printf("\n PIR: %s", pohybStringBfr);
      printf("\n Lezi na posteli: %s", leziStringBfr);
      printf("\n");
      
      delay(500);
  }

  /*
    Ukončení MQTT
  */
  mosquitto_disconnect(mqtt);
  mosquitto_destroy(mqtt);
  mosquitto_lib_cleanup();

  return 0;
}
