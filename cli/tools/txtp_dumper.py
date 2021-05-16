#!/usr/bin/env python3
import os, glob, argparse

#******************************************************************************
# TXTP DUMPER
#
# Creates .txtp from a text list, mainly .m3u with virtual txtp to actual .txtp
# example:
# 
# # %TITLE Stage 1
# bgm.awb#1 .txtp
# >> creates bgm.awb#1 .txtp
#
#******************************************************************************

class Cli(object):
    def _parse(self):
        description = (
            "Makes TXTP from list"
        )
        epilog = (
            "examples:\n"
            "  %(prog)s !tags.m3u\n"
            "  - make .txtp per line in !tags.m3u\n\n"
            "  %(prog)s !tags.m3u -m \n"
            "  - make full txtp rather than mini-txtp\n    (may overwrite files when using subsongs)\n\n"
            "  %(prog)s lines.txt -s sound \n"
            "  - make .txtp per line, setting a subdir\n\n"
        )

        p = argparse.ArgumentParser(description=description, epilog=epilog, formatter_class=argparse.RawTextHelpFormatter)
        p.add_argument('files', help="Files to get (wildcards work)", nargs='+')
        p.add_argument('-s',  dest='subdir', help="Create .txtp pointing to subdir")
        p.add_argument('-m',  dest='maxitxtp', help="Create regular txtp rather than mini-txtp", action='store_true')
        p.add_argument('-f',  dest='force', help="Make .txtp even if line isn't a .txtp", action='store_true')
        p.add_argument('-o',  dest='output', help="Output dir (default: current)")
        #p.add_argument('-n',  dest='normalize', help="Normalize .txtp formatting")
        return p.parse_args()

    def start(self):
        args = self._parse()
        if not args.files:
            return
        App(args).start()

#******************************************************************************

class App(object):
    def __init__(self, args):
        self.args = args

    def start(self):
        filenames = []
        for filename in self.args.files:
            filenames += glob.glob(filename)

        for filename in filenames:
            path = self.args.output or '.'
            if self.args.output:
                try:
                    os.makedirs(self.args.output)
                except OSError:
                    pass
            
            with open(filename) as fi:
                for line in fi:
                    line = line.strip()
                    line = line.rstrip()
                    if line.startswith('#'):
                        continue

                    if not line.endswith('.txtp'):
                        if self.args.force:
                            line += '.txtp'
                        else:
                            continue

                    line.replace('\\', '/')

                    subdir = self.args.subdir
                    if self.args.maxitxtp or subdir:
                        index = line.find('.') #first extension

                        if line[index:].startswith('.txtp'): #???
                            continue

                        name = line[0:index] + '.txtp'

                        text = line.replace('.txtp', '').strip()
                        if subdir:
                            subdir.replace('\\', '/')
                            if not subdir.endswith('/'):
                                subdir = subdir + '/'
                            text = subdir + text
                    else:
                        name = line
                        text = ''

                    outpath = os.path.join(path, name)

                    with open(outpath, 'w') as fo:
                        if text:
                            fo.write(text)
                        pass

if __name__ == "__main__":
    Cli().start()
