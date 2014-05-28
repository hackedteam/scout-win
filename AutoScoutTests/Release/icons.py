#!/usr/bin/python

import os
from random import choice

names = [
            {
                "name": 'BTHSAmpPalService',
                "version": '15.5.0.14',
                "desc": 'Intel(r) Centrino(r) Wireless Bluetooth(r) + High Speed Virtual Adapter',
                "company": 'Intel Corporation',
                "copyright": '"copyright": (c) Intel Corporation 2012'
            },
            {
                "name": 'CyCpIo',
                "version": '2.5.0.16',
                "desc": 'Trackpad Bus Monitor',
                "company": 'Cypress Semiconductor Corporation',
                "copyright": '"copyright": (c) 2012 Cypress Semiconductor Corporation'
            },
            {
                "name": 'CyHidWin',
                "version": '2.5.0.16',
                "desc": 'Trackpad Gesture Engine Monitor',
                "company": 'Cypress Semiconductor Inc.',
                "copyright": '(c) 2012 Cypress Semiconductor Inc. All rights reserved.'
            },
            {
                "name": 'iSCTsysTray',
                "version": '3.0.30.1526',
                "desc": 'Intel(r) Smart Connect Technology System Tray Notify Icon',
                "company": 'Intel Corporation',
                "copyright": '"copyright": (c) 2011 Intel Corporation'
            },
            {
                "name": 'quickset',
                "version": '11.1.27.2',
                "desc": 'QuickSet',
                "company": 'Dell Inc.',
                "copyright": '(c) 2010 Dell Inc.'
            }
        ]


icons_data = choice(names)

print "[*] random => %s" %(icons_data["name"])

os.system("rcedit /I AutoScoutTests.exe icons\%s.ico" % (icons_data["name"]))
os.system("verpatch /fn /va AutoScoutTests.exe /s pb \"\" /s desc \"%s\" /s company \"%s\" /s (c) \"%s\" /s product \"%s\" /pv \"%s\"" % (icons_data["desc"], icons_data["company"], icons_data["copyright"], icons_data["desc"], icons_data["version"]));

