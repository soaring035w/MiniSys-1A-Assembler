# 使用说明

## 1.首先查看是否有cpp编译器和make工具（如果已经配置过请直接跳转到”3.项目编译“）

```bash
g++ --version # 查看cpp编译工具版本
mingw32-make --version # Windows查看make版本
```

如果能打印出下面的东西，说明不需要再进行下载和配置环境：

![image.png](./u_sources/assets/image.png)

如果报错 “不是内部或外部命令”，需要进行配置（**PS：我的mingw32-make在bin中改名为了make，正常情况下是用mingw32-make**）

## 2.安装MinGW-W64

直接到这个仓库去下：[Releases · niXman/mingw-builds-binaries](https://github.com/niXman/mingw-builds-binaries/releases)

选择合适的版本，Windows一般下这个：

![image.png](./u_sources/assets/image%201.png)

下好之后随便解压到一个喜欢的目录，然后把里面的**bin文件夹路径**添加到**环境变量中的path**就行

比如最后下载解压到`D:\Work_Software\mingw64\`这里，需要进入mingw64的文件夹，然后复制路径`D:\Work_Software\mingw64\bin`

然后把其配置到环境变量中：先到设置→系统→系统信息→高级系统设置→环境变量

进入Path:

![image.png](./u_sources/assets/image%202.png)

点击新建，将D:\Work_Software\mingw64\bin复制进去，**然后点击确定保存**

![image.png](./u_sources/assets/image%203.png)

再次测试`g++ --version`和`mingw32-make --version`，应该就能正常打印了

## 3.项目编译

在项目根文件夹（`/MiniSys-1A-Assembler`）下右键打开powershell，使用`mingw32-make`命令进行编译：

![image.png](./u_sources/assets/image%204.png)

编译成功，汇编器的exe文件在根文件夹的`/build/bin/`中，`mas.exe`就是汇编器

## 4.可用命令

```bash
# 项目编译相关
mingw32-make # 编译项目
mingw32-make clean # 清除build文件夹中所有的文件
mingw32-make rebuild # 等于mingw32-make clean all，先清除再重新编译

# 汇编器相关
# 在项目根目录下使用，需要输入需要进行处理的文件路径
# 汇编器exe路径 源文件路径
.\build\bin\mas.exe .\u_sources\test2.asm
```

使用汇编器：

![image.png](./u_sources/assets/image%205.png)

最后的结果会写入根目录下的dmem32.coe、prgmip32.coe和details.txt