import threading
import time
import cv2
import mediapipe as mp
import numpy as np
import pyttsx3
from PIL import Image, ImageDraw, ImageFont
from mindspore import Tensor, dtype, load_checkpoint
# from model import HandModel
from model_m import HandModel_
from tools.draw_bounding_rect import draw_bounding_rect
from tools.draw_landmarks import draw_landmarks
from tools.landmark_handle import landmark_handle

model_path = 'checkpoints/model_test1.ckpt' #模型路径
# model_path = 'checkpoints/model_test1.pth' #模型保存路径
label=["you","two","eight", "no", "hit", "ugly", "praise", "one", "like", "study", "three",
         "moved",
         "",
         "", "", "", "", "", "", "", "", "", "", "", "",
         "",
         ""]
label_china={"you":'你',"two":'二',"eight":'八',"no":'不',"hit":'打', "ugly":'丑陋的',
             "praise":'表扬',"one":'一',"like":'喜欢',"study":'学习',"three":'三',"moved":'感动的','':''
             }

label_num = len(label)
model = HandModel_()
load_checkpoint(model_path, net=model)
# model = HandModel()
# state_dict = t.load(model_path)
# model.load_state_dict(state_dict)
mp_drawing = mp.solutions.drawing_utils
mp_hands = mp.solutions.hands
hands = mp_hands.Hands(
    static_image_mode=False,
    max_num_hands=1,
    min_detection_confidence=0.75,
    min_tracking_confidence=0.75)

FrameCount = 0
time1 = time.time()
fps = 0
background_flag = 0
background_color = 128
engine=pyttsx3.init()
# this_label='start'
start=time.perf_counter()
time_speech_epsode=1.5
def run_speech(label):

    engine.say("{}".format(label))
    # engine.say('{}'.format(label_china[label]))
    engine.runAndWait()

def run_speech_china(label):

    engine.say('{}'.format(label_china[label]))
    engine.runAndWait()
thread_speech = threading.Thread(target=run_speech, args=('欢迎使用智手译平台',), name='speech')
thread_speech.start()

def cv2AddChineseText(img, text, position, textColor=(200, 100, 250), textSize=40):
    if (isinstance(img, np.ndarray)):
        img = Image.fromarray(cv2.cvtColor(img, cv2.COLOR_BGR2RGB))

    draw = ImageDraw.Draw(img)

    fontStyle = ImageFont.truetype(
        "simsun.ttc", textSize, encoding="utf-8")

    draw.text(position, text, textColor, font=fontStyle)

    return cv2.cvtColor(np.asarray(img), cv2.COLOR_RGB2BGR)


def run_thumber(cap,speech):
    global FrameCount
    global time1
    global fps
    global this_label
    global start
    global time_speech_epsode
    ret, frame = cap.read() #读取
    frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    frame = cv2.flip(frame, 1) #水平翻转
    results = hands.process(frame)
    frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)

    hand_local = []
    if results.multi_hand_landmarks:
        for hand_landmarks in results.multi_hand_landmarks:
            # mp_drawing.draw_landmarks(frame, hand_landmarks, mp_hands.HAND_CONNECTIONS)
            for i in range(21):
                x = min(int(hand_landmarks.landmark[i].x * frame.shape[1]), frame.shape[1] - 1)
                y = min(int(hand_landmarks.landmark[i].y * frame.shape[0]), frame.shape[0] - 1)

                hand_local.append([x, y])

            if background_flag:
                frame = np.zeros(frame.shape, np.uint8)
                frame.fill(128)

            draw_landmarks(frame, hand_local)
            brect = draw_bounding_rect(frame, hand_local)

            hand_local = landmark_handle(hand_local)

    if hand_local:
        #print(hand_local)
        # output = model(t.tensor(hand_local))
        output = model(Tensor([hand_local], dtype=dtype.float32)).ravel()
        index, value = output.argmax_with_value()
        # index, value = output.topk(1)[1][0], output.topk(1)[0][0]
        print(index,value)
        # this_label = label[int(index)] # 原代码
        try:
            if int(value)==22:
                this_label = label[7]   #一
            elif int(value)==25:
                this_label = label[1]   #二
            elif int(value)==24:
                this_label = label[10]   #三
            elif int(value)==2:
                this_label = label[2]   #八
            elif int(value)==11:
                this_label = label[11]  #感动的
            elif int(value)==14:
                this_label = label[5]   #丑陋的
            elif int(value)==3:
                this_label = label[3]   #不
            elif int(value)==18:
                this_label = label[4]   #打
            elif int(value)==6:
                this_label = label[6]   #表扬praise
            elif int(value)==7:
                this_label = label[0]   #表扬praise
            elif int(value)==10:
                this_label = label[9]   #学习study
            # else:
            #     this_label = label[int(value)-21]
            print("label的名字",this_label)

        except Exception as e:
            this_label = ''
        if value >-1: #在窗口中显示的字 图片 要显示的字 坐标 字体 大小 颜色 粗度
            if speech == 0:
                cv2.putText(frame,
                            this_label,
                            (300, 450),
                            cv2.FONT_HERSHEY_SIMPLEX,
                            1.5,
                            (200, 100, 250),
                            3)
            else:

                frame=cv2AddChineseText(frame,
                            label_china[this_label],
                            (255, 400)
                            )

            end = time.perf_counter()
            if speech==0:
                if (end - start) > time_speech_epsode:

                    thread_speech = threading.Thread(target=run_speech, args=(this_label,),name='speech')

                    thread_speech.start()
                    start = time.perf_counter()
            else:
                if (end - start) > time_speech_epsode:

                    thread_speech = threading.Thread(target=run_speech, args=(label_china[this_label],), name='speech')

                    thread_speech.start()
                    start = time.perf_counter()



            # speak = win.Dispatch("SAPI.SpVoice")
            # speak.Speak("你好")
    time2 = time.time()
    FrameCount += 1
    if time2 - time1 >= 0.5:
        if FrameCount > 0:
            fps = round(FrameCount / (time2 - time1), 2)
            time1 = time.time()
            FrameCount = 0

    return frame



if __name__ == '__main__':
    # print(threading.active_count())
    # print(threading.enumerate())
    import time

    for i in range(100):
        pass
    end=time.perf_counter()
    print(end-start)
    # thread_speech = threading.Thread(target=run_speech, args=('欢迎使用智手译',), name='speech')
    # thread_speech.start()
    # for i in range(100):
    #     print(thread_speech.is_alive())
    #     time.sleep(0.1)
    #     if thread_speech.is_alive() != True:
    #
    #         thread_speech = threading.Thread(target=run_speech, args=('this_label',), name='speech')
    #         thread_speech.start()
