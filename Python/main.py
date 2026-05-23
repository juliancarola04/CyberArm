# Referencia de la API https://ai.google.dev/edge/api/mediapipe/python/mp/tasks/vision. Hay algunas cosas que no están como
# las drawing_utils y drawing_styles.
# Para saber la documentación de estas usen este código en otro programa Python:

# from mediapipe.tasks.python import vision
#
# for x in dir(vision): # Esto nos va a decir todos los submodulos de vision
#     if not x.startswith("_"):
#         print(x)
#
# help(vision.moduloEspecifico) # Con esto podemos saber documentación de un módulo en específico 

import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

import numpy as np
import cv2 as cv
import time

BaseOptions = python.BaseOptions
HandLandmarker = vision.HandLandmarker
HandLandmarkerOptions = vision.HandLandmarkerOptions
HandLandmarkerResult = vision.HandLandmarkerResult
HandLandmarkerConnections = vision.HandLandmarksConnections
VisionRunningMode = vision.RunningMode

mp_drawing = vision.drawing_utils
mp_drawing_styles = vision.drawing_styles

model_path = './hand_landmarker.task' 
direccion_camara = 'http://10.216.110.241:4747/video' # Acá va la dirección IP de la camara. Si fuera la de la webcam sería 0 o 1, pero en formato numérico.

frameADibujar = None

cap = cv.VideoCapture(direccion_camara)


# HandLandmarkerResult va a contener los puntos cardinales de cada uno de los puntos de la mano.
# https://ai.google.dev/edge/mediapipe/solutions/vision/hand_landmarker/python#handle_and_display_results 
def mostrar_resultados(result: HandLandmarkerResult, output_image: mp.Image, timestamp_ms: int):
    
    cv_image = output_image.numpy_view() # Como CV funciona con arreglos numpy, lo transformamos de nuevo
    cv_image = cv.cvtColor(cv_image, cv.COLOR_RGB2BGR) # y lo volvemos a cambiar a los colores correctos.

    global frameADibujar

    for hand_landmarks in result.hand_landmarks:

        # Por cada uno de los puntos de la mano vamos a dibujar en el frame CV los puntos y las conexiones entre estos.
        mp_drawing.draw_landmarks(
            image=cv_image,
            landmark_list=hand_landmarks,
            connections=HandLandmarkerConnections.HAND_CONNECTIONS, # Con esto mostramos las conexiones. Se puede quitar si se quiere.
            landmark_drawing_spec=mp_drawing_styles.get_default_hand_landmarks_style(),
            connection_drawing_spec=mp_drawing_styles.get_default_hand_connections_style()
        )

    frameADibujar = cv_image # Guardamos el frame ya todo procesado que será el que utilizaremos cuando lo mostremos.

options = HandLandmarkerOptions(
    base_options=BaseOptions(model_asset_path=model_path),
    running_mode=VisionRunningMode.LIVE_STREAM,
    num_hands = 1, # El control va a ser con 1 sola mano, así que lo especificamos. Si quieren probar de detectar más manos tienen que cambiar esto.
    result_callback=mostrar_resultados)

with HandLandmarker.create_from_options(options) as landmarker:

  while cap.isOpened():
     ret, frame = cap.read() # ret nos indicará si captó algo y frame el frame que captó la cámara.
     if not ret:
        break
     
     frame = cv.flip(frame, 1) # Lo flipeamos así tiene más sentido a la hora de controlar el brazo.
     rgb_frame = cv.cvtColor(frame, cv.COLOR_BGR2RGB) # Image de Mediapipe trabaja con RGB y CV con BGR. Así que lo transformamos. 
     mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame) # Las tasks de Mediapipe trabajan con image, mientras que CV usa arreglos numpy, así que lo formateamos.

     timestamp = int(time.time() * 1000) # Tenemos que tener un timestamp para que a la hora de procesar los frames, al ser asincrónicos, se hagan de manera organizada.
     landmarker.detect_async(mp_image, timestamp) # Cuando termina de procesar la imagen de Mediapipe llama a la función definida en result_callback (mostrar_resultados en este caso.)

     if frameADibujar is not None:
        cv.imshow("Manos", frameADibujar)

     if cv.waitKey(1) & 0xFF == 27: # Con el ESC se aborta el programa.
        break
    

