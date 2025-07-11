from mindspore.nn import Cell,Dense
import mindspore
from mindspore import Tensor,load_checkpoint
from mindspore import dtype
import numpy as np
class HandModel_(Cell):
    def __init__(self):
        super(HandModel_, self).__init__()
        self.linear1 = Dense(in_channels=48,out_channels=40)
        self.linear2 = Dense(in_channels=40,out_channels=32)
        self.linear3 = Dense(in_channels=32,out_channels=27)

    def construct(self, input):
        out = self.linear1(input)
        out = self.linear2(out)
        out = self.linear3(out)


        return out



if __name__ == '__main__':
    model_path = './checkpoints/model_test1.ckpt'
    model=HandModel_()
    load_checkpoint(model_path, net=model)
    hand_local= [-0.39361702127659576, -0.21808510638297873, -0.25, -0.26063829787234044, -0.4308510638297872, -0.425531914893617, -0.574468085106383, -0.601063829787234, -0.3882978723404255, -0.9840425531914894, -0.0851063829787234, -0.4148936170212766, -0.18617021276595744, -0.6436170212765957, -0.26595744680851063, -0.851063829787234, -0.2765957446808511, -0.9574468085106383, -0.40425531914893614, -0.26063829787234044, -0.601063829787234, -0.05851063829787234, -0.6170212765957447, 0.12234042553191489, -0.2127659574468085, -0.8776595744680851, -0.4148936170212766, -0.2127659574468085, -0.5585106382978723, 0.09042553191489362, -0.5053191489361702, 0.324468085106383, -0.16489361702127658, -0.7659574468085106, -0.3617021276595745, -0.18617021276595744, -0.48936170212765956, -0.005319148936170213, -0.44680851063829785, 0.17553191489361702, -0.07446808510638298, 0.015957446808510637, 0.2393617021276596, -1.0, -0.10638297872340426, 0.03723404255319149, 0.28191489361702127, 0.24468085106382978]
    output = model(Tensor([hand_local],dtype=dtype.float32)).ravel()
    index, value = output.argmax_with_value()

    # index, value = output.top_k
    print(output,index,value)
    # print(output)