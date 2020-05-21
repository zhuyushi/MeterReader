# 项目简介

传统机械式指针表具无法将表具读数外传，需要人工进行读取，本项目使用深度学习方法实现传统机械式指针表具读数的自动读取。

本项目的深度学习算法基于百度PaddlePaddle框架实现；模型的训练使用百度PaddleModels工具完成；推理程序部署在ARM64平台上，基于PaddleLite实现。

针对模型训练，本项目提供了模型训练所需的数据集、模型训练工具的配置文件，以及训练好的模型，相关资源在“train”文件夹中。

针对推理部署，本项目提供了使用C++开发的，可以在ARM64平台、linux系统上运行程序源码，以及程序运行所需要的针对ARM平台优化过的模型文件，相关资源在“inference”文件夹中。

# 系统方案

第一步使用目标检测算法检测待识别表具区域，第二步使用语义分割算法分割出表具的刻度和指针，第三步计算表具读数。

![image](https://github.com/zhuyushi/MeterReader/blob/master/image/system.png) 

* 目标检测<br>
  平衡考虑算法的推理速度和检测效果，目标检测算法采用YOLOv3模型实现。目标检测部分只做检测不对表具进行分类。 

* 语义分割<br>
  根据目标检测的结果，从原图中裁剪出表具区域图像，做为语意分割模型的输入，考虑到刻度和指针均为细小区域，采用分割效果较好的DeepLapv3+模型实现。 

* 读数计算<br>
  预先配置表具相关数据（单位、量程等），结合语意分割结果计算出表具最终读数。

# 目标检测

## 目标检测方案

1、目标检测算法采用YOLOv3模型实现，被检测表具基本没有很小目标的情况，所以根据经验，人工重新设计了锚框的尺寸，以便更加准确的预测目标区域。

2、目标检测部分只对表具进行检测，不进行分类识别。表具的类别、单位和量程等信息，通过配置的形式输入给推理进程，结合分割结果综合计算表具读数。 

## 模型训练与评估 

* 数据集 <br>
  标注工具    ：Labelme <br>
  数据集格式 ：coco <br>

* 模型训练 <br>
  训练工具    ：使用百度models提供的PaddleDetection工具 <br> 
  模型选择    ：yolov3_darknet53 <br>
  预训练模型 ：百度提供的DarkNet53_pretrained.tar <br>

* 模型评估 <br>
  使用百度PaddleDetection配套的评估程序进行评估。
  目标检测算法的评估结果（coco评估标准）: mAP[.5:.9]为94.7%。 <br>
  
![image](https://github.com/zhuyushi/MeterReader/blob/master/image/yolov3-estimate.png) 

## 检测结果

![image](https://github.com/zhuyushi/MeterReader/blob/master/image/detection-1.png) 

![image](https://github.com/zhuyushi/MeterReader/blob/master/image/detection-2.png) 

![image](https://github.com/zhuyushi/MeterReader/blob/master/image/detection-3.png) 

# 语义分割

## 语义分割方案 

1、表具刻度与指针都较为细小，采用效果较好的DeepLabv3+分割模型。 

2、目标检测表具图像区域作为语义分割模型的输入。

![image](https://github.com/zhuyushi/MeterReader/blob/master/image/seg.png) 

## 模型训练与评估

* 数据集 <br>
  标注工具    ：Labelme（标注类型：line） <br>
  数据集格式 ：PaddleSeg <br>

* 模型训练 <br>
  训练工具    ：使用百度models提供的PaddleSeg工具 <br>
  模型选择    ： deeplabv3p_xception65 <br>
  预训练模型 ：百度提供的deeplabv3p_xception65_bn_coco <br>

* 模型评估 <br>
  使用百度PaddleSeg配套的评估程序进行评估。 <br>
  DeepLabv3+语义分割算法的评估结果，刻度和指针的IOU达到了70%以上。 <br>

![image](https://github.com/zhuyushi/MeterReader/blob/master/image/deeplabv3-estimate.png) 

## 分割结果

![image](https://github.com/zhuyushi/MeterReader/blob/master/image/seg-reasult.png) 

# 读数计算

从语义分割的结果中只能获取到表具指针和刻度的像素分割区域，表具的具体读数还需要结合表具的配置信息（表具种类、单位、量程）进行专门的计算获得，本项目使用的计算方法如下。

![image](https://github.com/zhuyushi/MeterReader/blob/master/image/read.png) 

# 模型部署

本项目目前实现了基于Paddle-Lite在ARM平台上的推理程序，代码位于“inference/arm64-RK3399”中，代码目录结构如下。

![image](https://github.com/zhuyushi/MeterReader/blob/master/image/infer-code.png) 

* 目标平台 <br>
  RK3399开发环境 ： CPU（ARMv8） <br>

* 软件环境 <br>
  操作系统：Ubuntu16.04 <br>
  基础软件：Paddle-Lite、OpenCV等 <br>
  开发语言：C++ <br>


