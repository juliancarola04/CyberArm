import socket
import time
import cv2 as cv
import mediapipe as mp
import numpy as np
import math
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

BaseOptions = python.BaseOptions
HandLandmarker = vision.HandLandmarker
HandLandmarkerOptions = vision.HandLandmarkerOptions
HandLandmarkerResult = vision.HandLandmarkerResult
HandLandmarkerConnections = vision.HandLandmarksConnections
VisionRunningMode = vision.RunningMode
mp_drawing = vision.drawing_utils
mp_drawing_styles = vision.drawing_styles

# Configuración de rutas y cámara
model_path = "./hand_landmarker.task"
direccion_camara = "http://192.168.0.102:4747/video" # Acá se pone la dirección IP del DroidCam.
frameADibujar = None

# Acá va las settings del ESP32. Si usamos el router local ponemos la IP que está vinculada a la MAC del router (hay que hacer esto).
ESP32_IP = '192.168.0.103'
PUERTO_UDP = 4210

socket_udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # Queremos usar UDP para hacerlo más rápido.

intervalo_envio = 0.05 # Cada cuanto se va a enviar al ESP32. Se envían 20 paquetes por segundo si se deja en 20. 1s/0.05s = 20.
ultimo_envio = 0.0

cap = cv.VideoCapture(direccion_camara)

# Ahora se van a configurar los distintos ángulos de actuación de cada uno de los servos.
# Esto hay que irlo chequeando, pero el de base casi seguro se mantiene. Los otros 2 tenemos que verlo.

# Ángulos para el servo de la BASE (mover la muñeca de un lado para otro)
base_min = 0
base_centro = 90
base_max = 180

# Parámetros de control. Limita el ángulo hasta que lo toma. Si se sube o bajan los valores luego el ángulo será
# mayor/menor.
# Este parámetro sirve para la base.
palm_angle_min = -50
palm_angle_max = 20

# Ángulos para el servo del CODO (subir y bajar la mano dentro del live).
codo_min = 0
codo_centro = 90
codo_max = 180

# Parámetros de control. Limita el ángulo hasta que lo toma. Si se sube o bajan los valores luego el ángulo será
# mayor/menor.
# Este parámetro sirve para el codo.
wrist_y_min = 0.3
wrist_y_max = 0.9

# Ángulos para el servo del HOMBRO (alejar y acercar la mano)
hombro_min = 0
hombro_centro = 45
hombro_max = 90

# Parámetros de control. Limita el ángulo hasta que lo toma. Si se sube o bajan los valores luego el ángulo será
# mayor/menor.
# Este parámetro sirve para el hombro.
plam_size_min = 0.1
plam_size_max = 0.5

# Ángulo de la pinza. Se puede cambiar desp.
claw_open_angle = 60
claw_close_angle = 0

servo_angle = [base_centro, codo_centro, hombro_centro, claw_open_angle]
prev_servo_angle = servo_angle.copy()

# Valor arbitrario a partir de cuando se considera que está abierto/cerrado el puño
fist_threshold = 7

#####
# Ambas funciones se van a usar en conjunto
# Una se encarga de limitar un valor entrante y luego se mapea ese respectivo valor
# con los máximso y mínimos de ese servo.

def limitar(valor, minimo, maximo):
    return max(min(maximo, valor), minimo)

def mapeo_rango(x, in_min, in_max, out_min, out_max):
    return abs((x - in_min) * (out_max - out_min) // (in_max - in_min) + out_min)

#####

def enviar_angulos_wifi(angulos):
    global ultimo_envio

    ahora = time.monotonic()

    if ahora - ultimo_envio < intervalo_envio:
        return
    
    mensaje = ",".join(
        str(angulo)
        for angulo in angulos
    )

    try:
        socket_udp.sendto(
            mensaje.encode("utf-8"),
            (ESP32_IP, PUERTO_UDP)
        )

        ultimo_envio = ahora

        print(f"Enviado al ESP32: {mensaje}")

    except OSError as error:
        print(f"Error enviando al ESP32: {error}")

def cerro_mano(hand_landmarks, palm_size):
    # Calcula la distancia entre la muñeca y la punta de cada dedo
    distance_sum = 0
    WRIST = hand_landmarks[0]
    for i in [7, 8, 11, 12, 15, 16, 19, 20]:
        distance_sum += (
            (WRIST.x - hand_landmarks[i].x) ** 2
            + (WRIST.y - hand_landmarks[i].y) ** 2
            + (WRIST.z - hand_landmarks[i].z) ** 2
        ) ** 0.5
    return (distance_sum / palm_size) < fist_threshold


def dibujar_angulos(frame, angulos):
    nombres = ["Base", "Codo", "Hombro", "Pinza"]
    for indice, (nombre, angulo) in enumerate(zip(nombres, angulos)):
        texto = f"{nombre}: {angulo} grados"
        cv.putText(
            frame,
            texto,
            (20, 35 + indice * 35),
            cv.FONT_HERSHEY_SIMPLEX,
            0.7,
            (0, 255, 0),
            2,
            cv.LINE_AA,
        )


def landmark_to_servo_angle(hand_landmarks):
    servo_angle = [base_centro, codo_centro, hombro_centro, claw_open_angle]
    WRIST = hand_landmarks[0]
    INDEX_FINGER_MCP = hand_landmarks[5]

    # Se calcula la distancia entre la muñeca y la primera falange del índice así luego podemos saber
    # la altura de la mano a la hora de controlar el hombro.
    palm_size = (
        (WRIST.x - INDEX_FINGER_MCP.x) ** 2
        + (WRIST.y - INDEX_FINGER_MCP.y) ** 2
        + (WRIST.z - INDEX_FINGER_MCP.z) ** 2
    ) ** 0.5

    # A continuación lo que vamos a buscar es la inclinación del vector muñeca -> falange del índice. Con esto vamos a poder saber
    # cuánto es que movió la persona la mano. Para esto vamos a usar el arco tangente de la diferencia_x y la diferencia_y.
    diferencia_x = INDEX_FINGER_MCP.x - WRIST.x
    diferencia_y = INDEX_FINGER_MCP.y - WRIST.y

    # La diferencia_y está en negativo debido a que en los videos las coordenadas Y se miden al revés.
    # ACLARACIÓN IMPORTANTE: El ángulo 0 (es decir, cuando la función de abajo devuelve 0) es cuando el índice x esta justo encima de la muñeca x,
    # por ende cualquier cosa que sea menor a esa significa que está para la izquierda y cualquier cosa que está adelante de ella significa que está a la derecha.
    angle = math.degrees(
        math.atan2(diferencia_x, -diferencia_y)
    )

    #print(f'Ángulo: {angle}')
    

    # Como el ángulo puede ser < -50 y > 20, lo limitamos a esas cotas
    angle = limitar(
        angle,
        palm_angle_min,
        palm_angle_max
    )
    # Una vez limitado mapeamos, teniendo en cuenta esos topes, el ángulo al grado permitido para ese servo en cuestión.
    servo_angle[0] = mapeo_rango(
        angle, palm_angle_min, palm_angle_max, base_max, base_min
    )

    # Ángulo para el Codo (eje Y)
    wrist_y = limitar(WRIST.y, wrist_y_min, wrist_y_max)
    servo_angle[1] = mapeo_rango(
        wrist_y, wrist_y_min, wrist_y_max, codo_max, codo_min
    )

    # Ángulo para el Hombro (eje Z)
    palm_size_lim = limitar(palm_size, plam_size_min, plam_size_max)
    servo_angle[2] = mapeo_rango(
        palm_size_lim, plam_size_min, plam_size_max, hombro_max, hombro_min
    )

    # Abrir o cerrar la pinza
    if cerro_mano(hand_landmarks, palm_size):
        servo_angle[3] = claw_close_angle
    else:
        servo_angle[3] = claw_open_angle

    # Convertir a enteros porque si no se hace desp termina quedando con coma los ángulos.
    servo_angle = [int(i) for i in servo_angle]
    return servo_angle


def mostrar_resultados(
    result: HandLandmarkerResult, output_image: mp.Image, timestamp_ms: int
):
    global frameADibujar
    global servo_angle
    global prev_servo_angle

    cv_image = output_image.numpy_view()
    cv_image = cv.cvtColor(cv_image, cv.COLOR_RGB2BGR)

    if result.hand_landmarks:
        hand_landmarks = result.hand_landmarks[0]
        mp_drawing.draw_landmarks(
            image=cv_image,
            landmark_list=hand_landmarks,
            connections=HandLandmarkerConnections.HAND_CONNECTIONS,
            landmark_drawing_spec=mp_drawing_styles.get_default_hand_landmarks_style(),
            connection_drawing_spec=mp_drawing_styles.get_default_hand_connections_style()            
        )

        servo_angle = landmark_to_servo_angle(hand_landmarks)
        dibujar_angulos(cv_image, servo_angle)

        enviar_angulos_wifi(servo_angle)
    else:
        cv.putText(
            cv_image,
            "No se detecta ninguna mano",
            (20, 35),
            cv.FONT_HERSHEY_SIMPLEX,
            0.7,
            (0, 0, 255),
            2,
            cv.LINE_AA,
        )

    frameADibujar = cv_image


options = HandLandmarkerOptions(
    base_options=BaseOptions(model_asset_path=model_path),
    running_mode=VisionRunningMode.LIVE_STREAM,
    num_hands=1,
    result_callback=mostrar_resultados,
)

with HandLandmarker.create_from_options(options) as landmarker:
    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            continue

        frame = cv.flip(frame, 1)
        rgb_frame = cv.cvtColor(frame, cv.COLOR_BGR2RGB)
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)

        timestamp = int(time.time() * 1000)
        landmarker.detect_async(mp_image, timestamp)

        if frameADibujar is not None:
            cv.imshow("Manos", frameADibujar)

        if cv.waitKey(1) & 0xFF == 27:  # Con el "ESC" podés cerrar el programa
            break

cap.release()
cv.destroyAllWindows()