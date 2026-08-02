# CyberArm




### Como preparar el proyecto
1. Asegurate de tener instalado Python. En este caso se utilizó la versión 3.11.7: https://www.python.org/downloads/release/python-3117/ 
2. Bajarse el modelo que será usado para detectar los puntos de la mano: https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/latest/hand_landmarker.task
3. Mover el archivo a la carpeta de Python
4. Desde la consola accede a la ruta del proyecto y ejectuta los siguientes comandos:
    ```
    cd /ruta/de/la/carpeta/python
    ```
    ```
    python.exe -m venv .venv
    ```
    ```
    Set-ExecutionPolicy Unrestricted -Scope Process
    ```
    ```
    ./.venv/Scripts/Activate.ps1
    ```
    ```
    pip install -r requirements.txt
    ```
6. Conseguir credenciales de una red local la cual se pueda administrar
7. Conectar el dispositivo que ejecutará el programa de Python, el celular que se utilizará como cámara y el ESP32. Para conectar el ESP32 dirígete al archivo con el nombre de "credencial.h" situado en la carpeta del ESP32 y coloca las credenciales de la red.
8. Asegúrate de tener instalada la librería de ESP32Servo en tu IDE de confianza (se usó PlatformIO en este proyecto) y carga el código.
9. Cuando hayas conectado los 3 dispositivos a la red busca la MAC que tiene cada uno de ellos. La mayoría de routers permiten ver la dirección MAC de los dispositivos conectados a la red. Una vez tengas la MAC de cada uno de los dispositivos resérvales a cada uno una IP en el DHCP asociándola con la MAC.
![Bind MAC e IP](assets/img/MAC-IP.png "Bind MAC con IP")
10. Según la IP que le hayas asignado al ESP32 y al celular, pon su dirección en 2 partes del código de Python: ```direccion_camara``` y ```ESP32_IP```.
11. Instalar la aplicacion DroidCam desde la Play Store y ábrala.
12. Asegúrese que la IP que le sale en la aplicación de DroidCam sea la misma que puso en el router para el celular.

### Como ejecutar el proyecto
Abra la aplicación de DroidCam. Una vez que la haya abierto hay dos opciones posibles:
* Si se encuentra en VSCode simplemente ejecute el archivo main.py.
* Si NO se encuentra en VSCode simplemente ejecute el archivo main.py.
    1. Navegue desde la terminal hasta la carpeta Python
    2. Ejecute ```./.venv/Scripts/Activate.ps1```
    3. Ejecute ```python.exe ./main.py```
