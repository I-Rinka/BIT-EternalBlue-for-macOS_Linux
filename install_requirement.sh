#!/bin/bash
git clone https://github.com/I-Rinka/impacket.git # install impacket dependency
cd impacket
sudo -H python3 -m pip install .
cd ..
sudo -H pip3 install pycrypto pyasn1
