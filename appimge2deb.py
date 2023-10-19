#!/usr/bin/env python3

import re
import os
import sys



def main(args):
    appimg = args.appimage
    rules_f='debian/rules.appimage'
    control_f='debian/control.appimage'
    r_bak = rules_f+'.bak'

    if not os.path.isfile(appimg):
        print("{} not found".format(appimg),file=sys.stderr)
        return 1

    if not os.path.isfile(rules_f):
        print("{} not found".format(rules_f),file=sys.stderr)
        return 2

    if not os.path.isfile(control_f):
        print("{} not found".format(control_f),file=sys.stderr)
        return 3

    try:
        st = os.system("cd debian && ln -sf '{}' rules && ln -sf '{}' control".format(os.path.basename(rules_f),os.path.basename(control_f)))
        if st != 0:
            return 4

        cmd="rm -rf {};sed -i 's|app_imgage:=.*|app_imgage:={}|g' {}".format(r_bak,appimg,rules_f)
        # print(cmd)
        st = os.system(cmd)
        if st != 0:
            return 5

        st = os.system('dpkg-buildpackage  -b -d')
        if st != 0:
            return 6
    finally:
        os.system("cd debian && ln -sf rules.normal rules && ln -sf control.normal control")
        if os.path.isfile(r_bak):
            os.system('mv {} {}'.format(r_bak,rules_f))

    return 0

def parse_arguments():
    import configparser
    import argparse
    parser = argparse.ArgumentParser()

    parser.add_argument('appimage', type=str,
                        help='AppImage file')

    return parser.parse_args()

if __name__ == '__main__':
    exit(main(parse_arguments()))
