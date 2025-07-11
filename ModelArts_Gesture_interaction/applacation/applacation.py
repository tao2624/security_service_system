import tkinter as tk
from tkinter import ttk
import cv2
from PIL import Image, ImageTk
import hand_app
import speech_app
import volume_app



speech=1 #0是英文1是中文

class Applacation(ttk.Frame):
    def __init__(self, parent):
        global speech
        ttk.Frame.__init__(self)
        self.root=parent
        self.english_theme = {'设置': 'Set up', '白昼模式': 'Daytime', '黑夜模式': 'Night',
                              '英文模式': 'English mode', '功能列表': 'Feature List',
                              '手语翻译': 'Sign language', '音量调节': 'Modulation', '绘图写字': 'Drawing',
                              '结束命令': 'End'
            , '视频': 'Video frequency', '智手译': 'Intelligent hand translation'}
        self.var_0 = tk.BooleanVar()
        self.var_1 = tk.IntVar(value=1)
        self.var_2 = tk.IntVar(value=0)
        self.open = 0
        self.theme = ['light', 'dark']
        self.root.tk.call("set_theme", "light")
        self.width = 1000
        self.height = 700
        self.speech=speech #0是英文1是中文
        self.root.title('智手译' if self.speech==1 else self.english_theme['智手译'])
        self.set_up()


    def set_up(self):

        #主题
        self.radio_frame = ttk.LabelFrame(self, text="设置" if self.speech==1 else self.english_theme['设置'], padding=(20, 10)
                                          )
        self.radio_frame.grid(row=0, column=0, padx=30, pady=20, sticky="nsew")

        self.radio_1 = ttk.Radiobutton(
            self.radio_frame, text="白昼模式" if self.speech==1 else self.english_theme['白昼模式']
            , variable=self.var_1, value=1,command=lambda: self.Theme_point(0),style="Switch.TCheckbutton"
        )
        self.radio_1.grid(row=0, column=0, padx=30, pady=20, sticky="nsew")
        self.radio_2 = ttk.Radiobutton(
            self.radio_frame, text="黑夜模式" if self.speech==1 else self.english_theme['黑夜模式']
            , variable=self.var_1, value=2,command=lambda: self.Theme_point(1),style="Switch.TCheckbutton"
        )
        self.radio_2.grid(row=1, column=0, padx=30, pady=20, sticky="nsew")

        self.radio_3 = ttk.Checkbutton(
            self.radio_frame, text="英文模式" if self.speech==1 else self.english_theme['英文模式']
            , variable=self.var_0,command=lambda : self.china_english_sf()
        )
        self.radio_3.grid(row=2, column=0, padx=30, pady=20, sticky="nsew")

        # 分割线
        self.separator = ttk.Separator(self)
        self.separator.grid(
            row=1, column=0, padx=(20, 10), pady=10, sticky="ew"
        )

        # 选项
        self.check_frame = ttk.LabelFrame(self, text="功能列表" if self.speech==1 else self.english_theme["功能列表"]
                                          ,padding=(20, 10))
        self.check_frame.grid(
            row=2, column=0, padx=10, pady=15, sticky="nsew"
        )


        self.radio_1 = ttk.Radiobutton(
            self.check_frame, text="手语翻译" if self.speech==1 else self.english_theme["手语翻译"]
            , variable=self.var_2, value=1, command=lambda: self.hand_apply(),
         style = "Toggle.TButton"

        )
        self.radio_1.grid(row=0, column=0, padx=40, pady=20, sticky="nsew")
        self.radio_2 = ttk.Radiobutton(
            self.check_frame, text="音量调节" if self.speech==1 else self.english_theme["音量调节"]
            , variable=self.var_2, value=2, command=lambda: self.volume_apply()
            , style="Toggle.TButton"

        )
        self.radio_2.grid(row=1, column=0, padx=40, pady=20, sticky="nsew")

        self.radio_3 = ttk.Radiobutton(
            self.check_frame, text="绘图写字" if self.speech==1 else self.english_theme["绘图写字"]
            , variable=self.var_2, value=3, command=lambda:  self.speech_apply()
            , style="Toggle.TButton"

        )
        self.radio_3.grid(row=2, column=0, padx=40, pady=20, sticky="nsew")

        self.radio_4 = ttk.Radiobutton(
            self.check_frame, text="结束命令" if self.speech==1 else self.english_theme["结束命令"]
            , variable=self.var_2, value=4, command=lambda: self.stop_run()
            , style="Toggle.TButton"

        )
        self.radio_4.grid(row=3, column=0, padx=40, pady=20, sticky="nsew")


        #视频
        self.vedio = ttk.LabelFrame(self, text="视频" if self.speech==1 else self.english_theme["视频"]
                                    ,padding=(10, 10))
        self.vedio.grid(
            row=0, column=1, padx=10, pady=10, sticky="nsew",rowspan=3
        )
        self.vedio.columnconfigure(index=0, weight=1)


        self.canvas = tk.Canvas(
            self.vedio, bg='white', width=self.width, height=self.height
                                )
        self.canvas.grid(row=0, column=0, padx=5, pady=5, sticky="nsew")


    def Theme_point(self,point):
        self.root.tk.call("set_theme", "{}".format(self.theme[point]))


    def applacation_run(self):

        cap = cv2.VideoCapture(0)
        while self.open!=0:
            try:
                if self.open==1:
                    cvimage = hand_app.run_thumber(cap,self.speech)
                elif self.open==2:
                    cvimage = volume_app.volume_run(cap,self.speech)
                elif self.open==3:
                    cvimage = speech_app.speech_run(cap,self.speech)
                else:
                    break
                pic = self.handle(cvimage, self.width, self.height)
                self.canvas.create_image(0, 0, anchor='nw', image=pic)
                self.root.update()
            except Exception as e:
                raise e
                break
        self.open=0
        cap.release()

    def hand_apply(self):
        self.open=1
        self.applacation_run()

    def volume_apply(self):
        self.open=2
        self.applacation_run()
    def speech_apply(self):
        self.open=3
        self.applacation_run()

    def handle(self,cvimage, image_width, image_height):
        cvimage = cv2.cvtColor(cvimage, cv2.COLOR_BGR2RGB)
        pilImage = Image.fromarray(cvimage)
        pilImage = pilImage.resize((image_width, image_height), Image.LANCZOS)
        tkImage = ImageTk.PhotoImage(image=pilImage)
        return tkImage

    def stop_run(self):
        self.open=0

    def china_english_sf(self):
        global speech
        speech=1-speech
        self.theme_china_english_sf()

    def theme_china_english_sf(self):
        if self.speech==0: #英文
            self.root.destroy()
            tk_theme()

        else:  #中文
            self.root.destroy()
            tk_theme()

def tk_theme():
    root = tk.Tk()
    #root.geometry('750x500')
    root.geometry('1920x1080')
    root.tk.call("source", "azure.tcl")
    applacation = Applacation(root)
    applacation.pack(fill="both", expand=True)
    root.mainloop()

if __name__ == '__main__':
    tk_theme()