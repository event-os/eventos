# EventOS Nano开发环境搭建
-------
#### 概述
**EventOS**与**EventOS Nano**的开发环境，主要就是：MinGW、Python3、SCons构建工具三个部分。当然了，如果不安装这些开发环境，**EventOS Nano**也是可以通过MDK和IAR例程来使用的。

但我个人历来主张开发单片机程序，不要直接在单片机的IDE直接进行开发。我一贯的理念，是**使用PC技术武装单片机的开发**，要进行**跨平台的开发**，能大大加快开发效率。在长久的跨平台开发中，也能使单片机工程师涉猎到多个编程领域，从而使技能更加丰富，对软件的理解慢慢变深，就业面也随之变宽，竞争力变强，避免陷入内卷。也就是说，单片机工程师们，要写好单片机的程序，必须跳出来，看看其他软件领域是怎么写程序的，才能写好单片机程序。

什么是跨平台的开发呢？比如说，我们需要开发STM32上运行程序，就一定要对程序进行分层，让与硬件有关的程序，严格的限制在最小范围内（驱动层）。底层的程序，在单片机的IDE（比如MDK）中编程、调试，其他所有的与硬件无关的程序，都在PC上开发完成。PC的开发环境和调试环境非常友好，也不必反复烧写，而且可以利用单元测试等手段来保证软件质量。我自己用过的开发环境，大概有几种：Linux GCC，Qt和MinGW。

应用层所产生的对底层的依赖，使用虚拟底层去实现。这样至少可以保证应用层程序能够编译通过。编译通过后，再加入到MDK等IDE中去，进行集成调试。也可以让应用层模块单独运行在PC上，与单片机的底层程序之间使用一条总线进行通信，这样不仅开发效率非常高，而且让单片机工程师建立起软件分层的思想。**EventOS**的一个重要特性，就是要为单片机工程师的跨平台开发，提供完整的技术支持。

我个人是所谓的Linux派的程序员。我直接或者间接的，受Linux影响较大。MinGW提供了POSIX接口（Windows本身的POSIX接口，据说实现的不好，尚未验证），SCons提供了强大的构建能力，Python脚本可以用来写例程自动生成、代码格式检查和整理等自动化的工具。**MinGW**、**Python3**和**SCons**的组合，非常理想。

#### **MinGW**

安装MinGW，我建议安装32位。因为**EventOS Nano**目前所面对的单片机，主要还是32位（16位和8位尚未验证）。有一个简便的安装MinGW的方法，那就是直接安装**QP-bundle for Windows**，里面自带32位MinGW。安装完毕后，将QP中MinGW的安装路径，添加到环境变量中去。然后注销Windows账户，重新登录，在Windows Terminal或者PowerShell中，敲以下指令：
```
gcc --version
```
如果显示了GCC的版本，如下所示，就说明安装成功。
```
PS C:\Users\EventOS> gcc --version
gcc.exe (i686-posix-dwarf-rev0, Built by MinGW project) 5.3.0
Copyright (C) 2015 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

#### **Python3和SCons**
Python3的安装，如果是Win10环境，可以在MicroSoft Store中安装。当然也可以在Python官网上下载进行安装（安装时，要勾选添加环境变量；如果忘记，可以手动添加。手动添加后要注销Windows账户，重新登录）。安装成功的标志，是在Windows Terminal或者PowerShell中跳入以下指令：
```
python
```
如果进入了Python，会有类似的显示：
```
Python 3.10.2 (tags/v3.10.2:a58ebcc, Jan 17 2022, 14:12:15) [MSC v.1929 64 bit (AMD64)] on win32
Type "help", "copyright", "credits" or "license" for more information.
>>>
```

然后把Python安装目录下的路径..\Python310\Scripts**，添加进入环境变量（使其生效，同样要注销Windows账户，重新登录）。如果在MicroSoft Store中安装的Python，其路径会复杂一些。比如像下面的路径：
```
C:\Users\EventOS\AppData\Local\Packages\PythonSoftwareFoundation.Python.3.10_qbz5n2kfra8p0\LocalCache\local-packages\Python310\Scripts
```

然后在Windows Terminal或者PowerShell中，输入以下指令，等待安装成功。
```
pip install scons
```
查看是否安装成功的方法，是在Windows Terminal或者PowerShell中，输入以下指令。
```
scons --version
```
正确安装的话，会显示类似的信息：
```
SCons by Steven Knight et al.:
        SCons: v4.3.0.559790274f66fa55251f5754de34820a29c7327a, Tue, 16 Nov 2021 19:09:21 +0000, by bdeegan on octodog
        SCons path: ['C:\\Users\\EventOS\\AppData\\Local\\Packages\\PythonSoftwareFoundation.Python.3.10_qbz5n2kfra8p0\\LocalCache\\local-packages\\Python310\\site-packages\\SCons']
Copyright (c) 2001 - 2021 The SCons Foundation
```

#### 开始构建
上述工具都安装成功后，在**EventOS Nano**的目录下，输入
```
scons
```
就开启了整个工程的构建。愉快的开始开发吧！