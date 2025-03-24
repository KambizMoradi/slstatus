#!/bin/bash
pkill slstatus
sudo rm config.h
sudo make clean install
slstatus &
