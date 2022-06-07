# ExplorerBgTool
Custom Windows Explorer background image  
* 自定义文件资源管理器背景图片  
* 支持多个图片随机
* 可调整图片透明度
## 预览
### Windows11
![image](https://github.com/Maplespe/explorerTool/blob/main/preview11.jpg)
### Windows10
![image](https://github.com/Maplespe/explorerTool/blob/main/preview10.jpg)
## 使用方法
从Release页面下载压缩包并解压

然后运行 "`注册_Register.cmd`"文件

将图像放入 "`Image`" 文件夹

然后重新打开您的文件资源管理器窗口即可

## 配置文件
您可以通过修改 "`config.ini`"来修改一些样式

```ini
[image]
random=true #指定图片是否随机显示 您必须放入至少两张图像
posType=0 #图片显示位置 0=左上角 1=右上角 2=左下角 3=右下角 4=缩放并填充
imgAlpha=255 #图片的不透明度 范围0-255
```

修改后 您无需重启文件资源管理器 只需重新打开当前窗口即可

要卸载请运行 "`卸载_Uninstall.cmd`" 文件

## 注意事项
图片仅支持`png、jpg`格式 请确保为有效的图片 否则可能引发崩溃!

如果出现崩溃 请按住`ESC`键再打开文件资源管理器(这不会加载图像) 然后卸载本工具或删除不兼容的图像
## 其他
有关工具和介绍请访问：[winmoes.com](https://winmoes.com/tools/12556.html)
