import cv2
import math
import mediapipe as mp
from datetime import datetime
import datetime
from ctypes import cast, POINTER
from comtypes import CLSCTX_ALL
from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume
from hand_app import cv2AddChineseText
def Normalize_landmarks(image, hand_landmarks):
    new_landmarks = []
    for i in range(0, len(hand_landmarks.landmark)):
        float_x = hand_landmarks.landmark[i].x
        float_y = hand_landmarks.landmark[i].y
        width = image.shape[1]
        height = image.shape[0]
        pt = mp_drawing._normalized_to_pixel_coordinates(float_x, float_y, width, height)
        new_landmarks.append(pt)
    return new_landmarks


def Draw_hand_points(image, normalized_hand_landmarks):
    cv2.circle(image, normalized_hand_landmarks[4], 12, (0,180 , 0), -1, cv2.LINE_AA)
    cv2.circle(image, normalized_hand_landmarks[8], 12, (0,180 , 0), -1, cv2.LINE_AA)
    cv2.line(image, normalized_hand_landmarks[4], normalized_hand_landmarks[8], (0,180 , 0), 3)
    x1, y1 = normalized_hand_landmarks[4][0], normalized_hand_landmarks[4][1]
    x2, y2 = normalized_hand_landmarks[8][0], normalized_hand_landmarks[8][1]
    mid_x, mid_y = (x1 + x2) // 2, (y1 + y2) // 2
    length = math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2)  # 得到大拇指到食指的距离
    if length < 100:
        cv2.circle(image, (mid_x, mid_y), 12, (0,180 , 0), cv2.FILLED)
    else:
        cv2.circle(image, (mid_x, mid_y), 12, (0,180 , 0), cv2.FILLED)
    return image, length

def vol_tansfer(x):
    dict = {0: -65.25, 1: -56.99, 2: -51.67, 3: -47.74, 4: -44.62, 5: -42.03, 6: -39.82, 7: -37.89, 8: -36.17,
            9: -34.63, 10: -33.24,
            11: -31.96, 12: -30.78, 13: -29.68, 14: -28.66, 15: -27.7, 16: -26.8, 17: -25.95, 18: -25.15, 19: -24.38,
            20: -23.65,
            21: -22.96, 22: -22.3, 23: -21.66, 24: -21.05, 25: -20.46, 26: -19.9, 27: -19.35, 28: -18.82, 29: -18.32,
            30: -17.82,
            31: -17.35, 32: -16.88, 33: -16.44, 34: -16.0, 35: -15.58, 36: -15.16, 37: -14.76, 38: -14.37, 39: -13.99,
            40: -13.62,
            41: -13.26, 42: -12.9, 43: -12.56, 44: -12.22, 45: -11.89, 46: -11.56, 47: -11.24, 48: -10.93, 49: -10.63,
            50: -10.33,
            51: -10.04, 52: -9.75, 53: -9.47, 54: -9.19, 55: -8.92, 56: -8.65, 57: -8.39, 58: -8.13, 59: -7.88,
            60: -7.63,
            61: -7.38, 62: -7.14, 63: -6.9, 64: -6.67, 65: -6.44, 66: -6.21, 67: -5.99, 68: -5.76, 69: -5.55, 70: -5.33,
            71: -5.12, 72: -4.91, 73: -4.71, 74: -4.5, 75: -4.3, 76: -4.11, 77: -3.91, 78: -3.72, 79: -3.53, 80: -3.34,
            81: -3.15, 82: -2.97, 83: -2.79, 84: -2.61, 85: -2.43, 86: -2.26, 87: -2.09, 88: -1.91, 89: -1.75,
            90: -1.58,
            91: -1.41, 92: -1.25, 93: -1.09, 94: -0.93, 95: -0.77, 96: -0.61, 97: -0.46, 98: -0.3, 99: -0.15, 100: 0.0}
    return dict[x]

def vol_tansfer_reverse(x):
    error = []
    dict = {0: -65.25, 1: -56.99, 2: -51.67, 3: -47.74, 4: -44.62, 5: -42.03, 6: -39.82, 7: -37.89, 8: -36.17,
            9: -34.63, 10: -33.24,
            11: -31.96, 12: -30.78, 13: -29.68, 14: -28.66, 15: -27.7, 16: -26.8, 17: -25.95, 18: -25.15, 19: -24.38,
            20: -23.65,
            21: -22.96, 22: -22.3, 23: -21.66, 24: -21.05, 25: -20.46, 26: -19.9, 27: -19.35, 28: -18.82, 29: -18.32,
            30: -17.82,
            31: -17.35, 32: -16.88, 33: -16.44, 34: -16.0, 35: -15.58, 36: -15.16, 37: -14.76, 38: -14.37, 39: -13.99,
            40: -13.62,
            41: -13.26, 42: -12.9, 43: -12.56, 44: -12.22, 45: -11.89, 46: -11.56, 47: -11.24, 48: -10.93, 49: -10.63,
            50: -10.33,
            51: -10.04, 52: -9.75, 53: -9.47, 54: -9.19, 55: -8.92, 56: -8.65, 57: -8.39, 58: -8.13, 59: -7.88,
            60: -7.63,
            61: -7.38, 62: -7.14, 63: -6.9, 64: -6.67, 65: -6.44, 66: -6.21, 67: -5.99, 68: -5.76, 69: -5.55, 70: -5.33,
            71: -5.12, 72: -4.91, 73: -4.71, 74: -4.5, 75: -4.3, 76: -4.11, 77: -3.91, 78: -3.72, 79: -3.53, 80: -3.34,
            81: -3.15, 82: -2.97, 83: -2.79, 84: -2.61, 85: -2.43, 86: -2.26, 87: -2.09, 88: -1.91, 89: -1.75,
            90: -1.58,
            91: -1.41, 92: -1.25, 93: -1.09, 94: -0.93, 95: -0.77, 96: -0.61, 97: -0.46, 98: -0.3, 99: -0.15, 100: 0.0}
    for i in range(100):
        error.append(abs(dict[i] - x))
    return error.index(min(error))

mp_drawing = mp.solutions.drawing_utils
mp_hands = mp.solutions.hands
devices = AudioUtilities.GetSpeakers()
interface = devices.Activate(
    IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
volume = cast(interface, POINTER(IAudioEndpointVolume))
volRange = volume.GetVolumeRange()
minVol = volRange[0]
maxVol = volRange[1]
print(minVol, maxVol)
#cap = cv2.VideoCapture(0)
hands = mp_hands.Hands(
    min_detection_confidence=0.5, min_tracking_confidence=0.5)
len_max = 0
len_min = 0
num = 0

def volume_run(cap,speech):
    stop = datetime.datetime.now() + datetime.timedelta(seconds=5)
    global len_max
    global len_min
    global num
    if num == 0:
        while datetime.datetime.now() < stop:
            success, image = cap.read()
            if not success:
                print("camera frame is empty!" if speech==0 else '相机框架为空')
                continue
            image = cv2.cvtColor(cv2.flip(image, 1), cv2.COLOR_BGR2RGB)
            image.flags.writeable = False
            results = hands.process(image)

            image.flags.writeable = True
            image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
            if results.multi_hand_landmarks:
                for hand_landmarks in results.multi_hand_landmarks:
                    mp_drawing.draw_landmarks(
                        image, hand_landmarks, mp_hands.HAND_CONNECTIONS)

                    normalized_landmarks = Normalize_landmarks(image, hand_landmarks)
                    image, length = Draw_hand_points(image, normalized_landmarks)
                    if length > len_max:
                        len_max = length
                    strRate = ''
                    cv2.putText(image, strRate, (10, 410), cv2.FONT_HERSHEY_COMPLEX, 1.2, (300, 255, 255), 2)
                    strRate1 = 'Maximum range = %d' % len_max
                    if speech==0:
                        cv2.putText(image, strRate1, (10, 110), cv2.FONT_HERSHEY_COMPLEX, 1.2, (300, 255, 255), 2)
                    else:
                        image = cv2AddChineseText(image,
                                                  '请定义食指和拇指的最大范围',
                                                  (10, 410),(255, 0, 0)
                                                  )
            cv2.imshow('result', image)
            if cv2.waitKey(5) & 0xFF == 27:
                break
            num = 1

    success, image = cap.read()
    if not success:
        print("camera frame is empty!" if speech==0 else '相机框架为空')


    image = cv2.cvtColor(cv2.flip(image, 1), cv2.COLOR_BGR2RGB)
    image.flags.writeable = False
    results = hands.process(image)

    image.flags.writeable = True
    image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
    if results.multi_hand_landmarks:
        for hand_landmarks in results.multi_hand_landmarks:
            mp_drawing.draw_landmarks(
                image, hand_landmarks, mp_hands.HAND_CONNECTIONS)

            normalized_landmarks = Normalize_landmarks(image, hand_landmarks)
            try:
                image, length = Draw_hand_points(image, normalized_landmarks)
                # print(length) #20~300
                cv2.rectangle(image, (50, 150), (85, 350), (300, 255, 255), 1)
                if length > len_max:
                    length = len_max

                vol = int((length) / len_max * 100)
                volume.SetMasterVolumeLevel(vol_tansfer(vol), None)

                cv2.rectangle(image, (50, 150 + 200 - 2 * vol), (85, 350), (300, 255, 255), cv2.FILLED)
                percent = int(length / len_max * 100)
                # print(percent)

                strRate = str(percent) + '%'
                cv2.putText(image, strRate, (40, 410), cv2.FONT_HERSHEY_COMPLEX, 1.2, (300, 255, 255), 2)

                vol_now = vol_tansfer_reverse(volume.GetMasterVolumeLevel())
                strvol = 'The current volume is ' + str(vol_now)
                if speech==0:
                    cv2.putText(image, strvol, (10, 470), cv2.FONT_HERSHEY_COMPLEX, 1.2, (300, 255, 255), 2)
                else:
                    image = cv2AddChineseText(image,
                                              '当前音量是'+ str(vol_now),
                                              (10, 430), (255, 0, 0)
                                              )

            except:
                pass

    return image