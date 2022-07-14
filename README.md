# ExplorerBgTool
Let your Explorer have a custom background image for Windows 11 and WIndows 10

* 自定义文件资源管理器背景图片
* 支持多个图片随机
* 可调整图片透明度
* 可自定义图片显示位置

#
* Customize Explorer ItemView background image
* Supports random switching of multiple pictures
* Adjustable picture alpha
* Customizable image display position
## 预览 (Overview)
### Windows 11

> 右下角 (Bottom right)
```ini
posType=3
```
![image](https://github.com/Maplespe/explorerTool/blob/main/Screenshot/083434.jpg)

> 居中 (Center)
```ini
posType=4
```
![image](https://github.com/Maplespe/explorerTool/blob/main/Screenshot/084016.jpg)

> 缩放和填充 (Zoom and fill)

```ini
posType=6
imgAlpha=140
```
![image](https://github.com/Maplespe/explorerTool/blob/main/Screenshot/085051.jpg)

### Windows 10
```ini
posType=3
```
![image](https://github.com/Maplespe/explorerTool/blob/main/Screenshot/085552.jpg)
## 使用方法 (How to use)
从Release页面下载最新版本压缩包并解压

将图像放入 "`Image`" 文件夹

> 运行 "`注册_Register.cmd`" 文件

然后重新打开您的文件资源管理器窗口即可

要卸载请运行 "`卸载_Uninstall.cmd`" 文件

您也可以使用命令提示符手动注册/卸载

> `regsvr32 (/u) "你的路径/ExplorerBgTool.dll"`
#
Download the latest version of the zip from the Release page and unzip it

Put your image in the `"image"` folder

> Run the "`注册_Register.cmd`" file

Just reopen your file explorer window

To uninstall run the "`卸载_Uninstall.cmd`" file

You can also use the command to register or uninstall

> `regsvr32 (/u) "you path/ExplorerBgTool.dll"`

## 配置文件 (Config)
您可以通过修改 "`config.ini`"来修改一些样式

You can modify some styles by modifying "`config.ini`"
```ini
[image]
#指定图片是否随机显示 您必须放入至少两张图像
#Specifies whether the image is displayed randomly, you need to put at least 2 images
random=true
#图片显示位置 0=左上角 1=右上角 2=左下角 3=右下角 4=居中 5=缩放 6=缩放并填充; 默认为3 右下角
#Image display position
#0=Left top 1=Right top 2=Left right 3=Right bottom 4=Center 5=Stretch 6=Zoom and fill
posType=0
#图片的不透明度 范围0-255
#Alpha 0-255 of image
imgAlpha=255
```

修改后 您无需重启文件资源管理器 只需重新打开当前窗口即可

After modification you don't need to restart file explorer, just reopen the current window

## 注意事项 (Attention)
图片仅支持`png、jpg`格式 请确保为有效的图片 否则可能引发崩溃!

如果出现崩溃 请按住`ESC`键再打开文件资源管理器(这不会加载图像) 然后卸载本工具或删除不兼容的图像
#
The image only supports `png, jpg` format, please make sure it is a valid image, otherwise it may cause a crash!

If there is a crash hold down the `ESC` key and open the file explorer (this will not load the image) then uninstall the tool or delete the incompatible image
## 其他 (Other)
有关工具和介绍请访问：

For tools and introductions, please visit:

[winmoes.com](https://winmoes.com/tools/12556.html)
