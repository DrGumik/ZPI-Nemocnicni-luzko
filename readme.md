# ZPI - Semestrální práce - nemocniční postel - DOKUMENTACE

### Potřebný hardware
```
- Raspberry Pi 4
- Gyroskop + akcelerometr (MPU6050)
- IR senzor
- Senzor vibrací
- Senzor EKG (nepoužit, výstupní data jsou analogová a RPI nemá ADC)
- PIR senzor
- Tenzometr (nepoužit, při testování funkčnosti prasknul)
```

### Instalace

1. Instalace RPI OS na SD kartu

2. Update všech knihoven linuxu
```
sudo apt update && upgrade
``` 

3. Instalace MQTT brokeru (Mosquitto)
```
sudo apt install mosquitto mosquitto-clients libmosquitto-dev
sudo systemctl status mosquitto
sudo systemctl enable mosquitto
```

4. Instalace Node.js a Node-redu
```
(Node.js)
sudo apt update
sudo apt install nodejs npm
npm install -g n
sudo n stable

(Node-red)
bash <(curl -sL https://raw.githubusercontent.com/node-red/linux-installers/master/deb/update-nodejs-and-nodered)
sudo systemctl enable nodered.service

sudo reboot
```

5. Instalace GCC kompilátoru
```
sudo apt install build-essential
```

6. Instalace WiringPi
```
wget https://github.com/WiringPi/WiringPi/releases/download/2.61-1/wiringpi-2.61-1-armhf.deb
sudo dpkg -i wiringpi-2.61-1-armhf.deb
```

7. Nastavení GPIO pinů, I2C a WiringPi
```
sudo raspi-config
-> Interfacing Options -> I2C -> Enable
sudo i2cdetect -y 0
sudo i2cdetect -y 1
gpio i2cdetect
gpio load i2c
```

### Kompilace a puštění celého programu programu
```
cd ZPI
gcc postel.c -o postel -I/usr/local/include -L/usr/local/lib -lwiringPi -lmosquitto -lm
./postel
```

### Připojení skrze SSH k RPI
```
Username: pi
Passwd: 1234
```
