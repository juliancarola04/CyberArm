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
5. Instalar la aplicacion DroidCam desde la Play Store
6. Crear una red Mobile Hotspot desde el celular. Se recomienda desactivar los datos.
7. Conectese desde la computadora a la red WiFi generada por el celular.
8. Abra la aplicación DroidCam e identifique la dirección IP del dispositivo y el puerto de DroidCam.
9. Vaya a main.py y asegúrase de reemplazar el valor que se encuentra en la variable ```direccion_camara``` por los correspondientes datos que haya obtenido del punto 8.

### Como ejecutar el proyecto
Abra la aplicación de DroidCam. Una vez que la haya abierto hay dos opciones posibles:
* Si se encuentra en VSCode simplemente ejecute el archivo main.py.
* Si se encuentra en VSCode simplemente ejecute el archivo main.py.
    1. Navegue desde la terminal hasta la carpeta Python
    2. Ejecute ```./.venv/Scripts/Activate.ps1```
    3. Ejecute ```python.exe ./main.py```
