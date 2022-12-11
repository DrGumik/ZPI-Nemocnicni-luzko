/*
ADR: raspberrypi.local
SSH: username: pi
     pass: 1234

Compile: gcc postel.c -o postel -I/usr/local/include -L/usr/local/lib -lwiringPi
Run: ./postel

Libs: WiringPi
Others required apps: gcc
*/

/*
    Knihovny
*/
#include <stdlib.h>
#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

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
float gyroskop(float *Gx, float *Gy, float *Gz) {
    float Gyro_x,Gyro_y,Gyro_z;

    Gyro_x = read_raw_data(GYRO_XOUT_H);
    Gyro_y = read_raw_data(GYRO_YOUT_H);
    Gyro_z = read_raw_data(GYRO_ZOUT_H);

    *Gx = Gyro_x/131;
    *Gy = Gyro_y/131;
    *Gz = Gyro_z/131;

    // úhly získáme tak, že naměřené hodnoty úhlové rychlosti vynásobíme časovým intervalem
    gyroAngleX = gyroAngleX + GyroX * elapsedTime; // stupeň / sekunda * sekunda = stupeň
    gyroAngleY = gyroAngleY + gyro * elapsedTime;
    yaw = yaw + GyroZ * elapsedTime; // namísto yaw jsme mohli dát gyroAngleZ

    // pro získání přesnějších měření kombinujeme, 96% hodnoty bude tvořit hodnota z gyroskopu, 4% z akcelerometru
    roll = 0.96 * gyroAngleX + 0.04 * accAngleX;
    pitch = 0.96 * gyroAngleY + 0.04 * accAngleY;

    return pitch;
}

/////////////////////////////////////////////////////////////////////
/*
    Obsluha senzoru vibrací
*/
bool vibracniSenzor(bool *zaznamenanaVibrace, bool *posledniVibrace) {
  bool vibrace = digitalRead(VIBRATION_PIN);
  
  if (*posledniVibrace != vibrace) {
    !*zaznamenanaVibrace;
  }
  
  *posledniVibrace = vibrace;
}

/////////////////////////////////////////////////////////////////////
/*
    Obsluha PIR senzoru
*/
void pirSenzor(bool *pohybPacienta) {
  if(digitalRead(PIR_PIN) == LOW) {
    *pohybPacienta = true;
  }
  else {
    *pohybPacienta = false;
  }
}

/////////////////////////////////////////////////////////////////////
/*
    Obsluha IR zavory
    (Nahrada za porouchany tenzometr)
*/
void irSenzor(bool *leziNaPosteli) {
  if(digitalRead(IR_PIN) == LOW) {
    *leziNaPosteli = true;
  }
  else {
    *leziNaPosteli = false;
  }
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
      Proměnné pro gyroskop s akcelerometrem
    */
    float Gx = 0, Gy = 0, Gz = 0; Ax = 0, Ay = 0, Az = 0;
    float GyroZ = 0;

    /*
      Proměnné pro ostatní senzory
    */
    bool pohybPacienta = false;
    bool dechPacienta = false;
    bool leziNaPosteli = false;
    bool zaznamenanaVibrace = false;
    bool posledniVibrace = false;


    /*
      Nastavení I2C a gyroskopu
    */
    fd = wiringPiI2CSetup(Device_Address);   /*Initializes I2C with device Address*/
    MPU6050_Init();                          /* Initializes MPU6050 */

    /*
      Hlavní cyklus programu
    */
    while(1)
    {
        /*
          Získání úhlu osy Z z akcelerometru a gyroskopu
          Parametry se předávají jako reference
        */
        GyroZ = gyroskop(&Gx, &Gy, &Gz);

        // Získání informací ze senzoru vibrací
        vibracniSenzor(&zaznamenanaVibrace, &posledniVibrace);

        // Získání informací ze PIR senzoru
        pirSenzor(&pohybPacienta);

        // Získání informací z IR zavory
        irSenzor(&leziNaPosteli);
        

        printf("\n GyroZ = %.2f", GyroZ);
        printf("\n Vibrace: %d", zaznamenanaVibrace);
        printf("\n Pohyb: %d", pohybPacienta);
        printf("\n Lezi na posteli: %d", leziNaPosteli);
        printf("\n");
        
        delay(250);

    }

    return 0;
}