import cv2
from cvzone.HandTrackingModule import HandDetector
import numpy as np
import os
from hand_app import cv2AddChineseText
imgNumeber = 0
alpha = 0.8  # 缩放比例
ws, hs = int(480 * alpha), int(270 * alpha)  # 16:9的图像显示比例
threshold = 360  # 线条的高度阈值

# 处理换页过快的问题
buttonPressed = False  # 只有当False才能换一次页
buttonCounter = 0  # 记录距离上一次换页已经过去多少帧了
buttonDelay = 30  # 每30帧才能执行一次换页

# 保存板书的每一个坐标点
annotations = [[]]  # 二维列表，每个列表保存连续绘制一次后的坐标点
annotationNumber = -1  # 当前使用的是第几个列表中的一次绘图后的关键点
annotationStart = False  # 开始绘图后，需要知道一次绘图的终点和起点
detector = HandDetector(maxHands=1, detectionCon=0.8)  # 最多检测1只手，检测置信度0.8

def speech_run(cap,speech):
    folderpath = r'./ppt'
    # 列表，存放每个文件的名称
    pathImage = sorted(os.listdir(folderpath), key=len)
    global imgNumeber
    global alpha
    global ws, hs
    global threshold
    global buttonPressed
    global buttonCounter
    global buttonDelay
    global annotations
    global annotationNumber
    global annotationStart
    global detector
    # 图像是否读取成功success，读取的帧图像img
    success, img = cap.read()  # 每次执行读取一帧
    pathFullImage = os.path.join(folderpath, pathImage[imgNumeber])
    # 获取指定路径的图片文件
    imgCurrent = cv2.imread(pathFullImage)

    # 翻转图像，使电脑图像和我们自己呈镜像关系
    img = cv2.flip(img, flipCode=1)  # flipCode=0上下翻转，1左右翻转

    # 返回手部信息hands，绘制关键点后的图像img
    hands, img = detector.findHands(img)

    # 在相机上画一条线，线以上做手势才能触发
    cv2.line(img, (0, threshold), (1280, threshold), (153, 50, 204), 3)
    # 在相机上画一个手指移动映射区域
    cv2.rectangle(img, (0, 0), (1280, 360), (0, 180, 0), 3)
    if speech==0:
        cv2.putText(img, "Please write here!", (10, 50), cv2.FONT_HERSHEY_COMPLEX, 1.5, (300, 255, 255), 4)
    else:
        img = cv2AddChineseText(img,
                                  '请写在这里',
                                  (10, 50), (255, 0, 0)
                                  )
    # 关键点处理
    if hands and buttonPressed is False:

        hand = hands[0]
        # 统计多少根手指翘起，返回一个列表，1表示手指是翘起的，0表示弯曲
        fingers = detector.fingersUp(hand)  # 最好手掌正对摄像机
        #print(fingers)  # [0,1,1,1,1]
        # 将手指移动的边界限制在一个框中，框的大小映射到屏幕大小
        lmList = hand['lmList']  # 获取21个关键点的xyz坐标
        # indexFinger = lmList[8][0], lmList[8][1]  # 获取食指指尖的xy坐标
        # 设置映射区域
        xval = int(np.interp(lmList[8][0], [300, 1280], [0, 1280]))  # x的映射区域
        yval = int(np.interp(lmList[8][1], [0, 360], [0, 720]))  # y的映射区域

        # 手部中心点坐标，hand是一个字典
        cx, cy = hand['center']  # 返回中心点坐标

        # 如果手的中心点坐标在线条以上就能继续操作
        if cy < threshold:  # 在图像上，坐标y向下为正，x向右为正

            # ① 手掌正对摄像机，只有大拇指翘起，执行向左换页操作
            if fingers == [1, 0, 0, 0, 0]:
                #print('to left')

                # 如果当前的ppt不是第一张才能再向前移动一张
                if imgNumeber > 0:
                    # 完成一次操作后，下次就不能再换页操作了
                    buttonPressed = True

                    # 当前展示的ppt编号向前移动一个，编号减一
                    imgNumeber -= 1  # pathImage[imgNumeber]指向的图片文件名改变

            # ② 手掌正对摄像机，只有小拇指翘起，执行向右换页操作
            if fingers == [0, 0, 0, 0, 1]:
                #print('to right')

                # 如果当前的ppt不是最后一张才能再向后移动一张
                if imgNumeber < len(pathImage) - 1:  # pathImage列表，存放所有的图片文件名

                    # 完成一次操作后，下次就不能再换页操作了
                    buttonPressed = True

                    # 当前展示的ppt编号向后移动一个，编号加一
                    imgNumeber += 1  # pathImage[imgNumeber]指向的图片文件名改变

        # ③ 指针设置，如果食指和中指同时竖起就绘制一个圈，不需要在线条以上
        if fingers == [0, 1, 1, 0, 0]:
            #print('circle')

            # 在ppt图片上的食指指尖绘制圆圈
            cv2.circle(imgCurrent, (xval, yval), 11, (180, 0, 0), 3)

        # ④ 板书设置，如果只有食指竖起就按食指轨迹移动绘制线条
        if fingers == [0, 1, 0, 0, 0]:
            # 如果之前没绘制过图那么annotationStart=False
            if annotationStart == False:
                annotationStart = True  # 那么当前帧开始绘图
                annotationNumber += 1  # 将当前绘图结果到保存在该索引指向的列表中
                annotations.append([])  # 在二维列表中添加一个子列表用来保存坐标

            cv2.circle(imgCurrent, (xval, yval), 5, (0, 255, 255), 1)
            # 将食指的每一帧坐标都保存在指定的索引列表中
            annotations[annotationNumber].append((xval, yval))
        # 如果不绘制板书了，当前一次绘图的坐标都保存好
        else:
            annotationStart = False  # 不绘制了

        # ⑤ 删除前一次的板书，不全删
        if fingers == [1,1,1,0,0]:
            if annotations:
                annotations.pop(-1)  # 删除最后一次绘图的坐标
                annotationNumber -= 1  # 绘图索引向前移动一个
                buttonPressed = True

        # ⑥ 擦除全部板书，如果没有手指竖起就删除所有板书
        if fingers == [0,0,0,0,0]:
            annotations = [[]]  # 二维列表重置
            annotationNumber = -1  # 重置当前使用的列表索引
            annotationStart = False # 重置绘图过程

    # 设置延时器
    if buttonPressed is True:  # 此时已经换过页了
        buttonCounter += 1  # 延时器计数加一
        if buttonCounter > buttonDelay:  # 如果延时器超过规定的帧数
            buttonPressed = False  # 下一帧可以换页
            buttonCounter = 0  # 延时计时器重置

    # 绘制板书，将第④步保存的坐标点都绘制出来
    for i in range(len(annotations)):  # 取出绘图列表
        # 遍历每个列表，绘制一次绘图的点
        for j in range(len(annotations[i])):
            # 绘制每两个点之间的线条
            if j != 0:
                cv2.line(imgCurrent, annotations[i][j - 1], annotations[i][j], (60, 0, 255), 5)

    # 显示图像
    imgSmall = cv2.resize(img, (ws, hs))  # 将摄像机帧图片缩小
    h, w, _ = imgCurrent.shape  # 获取ppt图像的高，宽，不要通道数
    # # ppt图片的右下角位置上显示视频图像，[h,w]
    imgCurrent[h-hs:h, w - ws:w] = imgSmall
    return imgCurrent

if __name__ == '__main__':
    cap = cv2.VideoCapture(0)
    while True:
        speech_run(cap)
    cap.release()
    cv2.destroyAllWindows()