# ZPI - Semestrální práce - nemocniční postel - DOKUMENTACE

### Potřebný hardware
```
- Raspberry Pi 4
- Gyroskop + akcelerometr (MPUxxx)
- IR senzor
- Senzor vibrací
- Senzor EKG (nepoužit, výstupní data jsou analogová a RPI nemá ADC)
- PIR senzor
- Tenzometr
```

### Instalace

1. Instalace RPI OS na SD kartu

2. Update všech knihoven linuxu
```
sudo apt update && upgrade
``` 

3. Instalace Snapu, poté reboot
```
sudo apt install snapd
sudo snap install core
sudo snap refresh core
sudo reboot
```

4. Instalace MQTT brokeru (Mosquitto)
```
sudo snap install mosquitto
```

5. Instalace Node.js a Node-redu
```
(Node.js)
sudo apt update
sudo apt install nodejs npm
npm install -g n
sudo n stable

(Node-red)
sudo apt install node-red
```

6. Instalace GCC kompilátoru
```
sudo apt install build-essential
```

7. Instalace WiringPi
```
wget https://github.com/WiringPi/WiringPi/releases/download/2.61-1/wiringpi-2.61-1-armhf.deb
sudo dpkg -i wiringpi-2.61-1-armhf.deb
```

8. Nastavení GPIO pinů, I2C a WiringPi
```
sudo raspi-config
-> Interfacing Options -> Advanced Options -> I2C -> Enable
sudo i2cdetect -y 0
sudo i2cdetect -y 1
gpio i2cdetect
gpio load i2c
```

### Kompilace a puštění celého programu programu
```
gcc postel.c -o postel -I/usr/local/include -L/usr/local/lib -lwiringPi
mosquitto -d
node-red
./postel
```

### Připojení skrze SSH k RPI
```
Username: pi
Passwd: 1234
```