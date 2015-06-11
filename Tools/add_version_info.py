#!/usr/bin/env python2
#
# Add CRC checksum and version information to an ELF file
#
# Copyright (C)2015 Thomas Kindler <mail_git@t-kindler.de>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
import os, struct
import argparse, subprocess
import getpass, platform

from StringIO import StringIO
from datetime import datetime

from crc32_forge import CRC32
from elf import ELFObject


class VersionInfo:
    format = "<II32s16s16s16s16s"
    git_cmd = "git describe --always --dirty"

    def __init__(self, elf):
        ss = elf.getSections()
        now = datetime.now()

        dprint("Running \"%s\"..." % self.git_cmd)

        git_version = subprocess.check_output(
            self.git_cmd.split(" ")
        ).strip()

        dprint("  %s" % git_version)

        self.image_crc    = 0x00000000  # must be calculated later
        self.image_size   = ss[-1].lma + ss[-1].sh_size - ss[0].lma      
        self.git_version  = git_version
        self.build_user   = getpass.getuser()
        self.build_host   = platform.node()
        self.build_date   = now.strftime("%Y-%m-%d")
        self.build_time   = now.strftime("%H:%M:%S")

    def pack_into(self, buffer, offset):
        return struct.pack_into( 
            self.format, buffer, offset,
            self.image_crc, self.image_size,
            self.git_version, 
            self.build_user, self.build_host, 
            self.build_date, self.build_time
        )


def parse_args():
    global args

    # Parse arguments
    #
    parser = argparse.ArgumentParser(
        description="Add CRC checksum and version information to an ELF file"
    )

    parser.add_argument(
        "--version", action="version",
        version="%(prog)s 1.1.0"
    )

    parser.add_argument(
        "-q", "--quiet", dest="verbose",
        default=True, action="store_false",
        help="do not print status messages"
    )

    parser.add_argument(
        "-s", "--section", dest="section",
        help = "name of the version_info section",
        default=".version_info"
    )

    parser.add_argument(
        "source", help = "source file"
    )

    parser.add_argument(
        "target", nargs = "?",
        help="target file"
    )

    args = parser.parse_args()
    if args.target == None:
        args.target = args.source


def dprint(*str):
    if args.verbose:
        for s in str:
            print s,
        print


def elf_to_bin(elf, elf_data):
    bin_data = bytearray()
    ss = elf.getSections()

    for s in ss:
        gap = (s.lma - ss[0].lma) - len(bin_data)
        bin_data += b'\xFF' * gap
        bin_data += elf_data[s.sh_offset : s.sh_offset + s.sh_size]

    return str(bin_data)


def patch_elf():
    dprint("Loading \"%s\"..." % args.source)

    with open(args.source, "rb") as file:
        elf_data = bytearray(file.read())

    elf = ELFObject()
    elf.fromFile(StringIO(elf_data))

    for s in elf.getSections():
        dprint("  %-16s: 0x%08x -> 0x%08x %8d" %
            (s.name, s.lma, s.sh_addr, s.sh_size)
        )

    ##########

    dprint("\nPatching section \"%s\"..." % args.section)

    section = elf.getSection(args.section)
    if section == None:
        raise Exception("Section not found")
    
    info = VersionInfo(elf)

    info.pack_into(elf_data, section.sh_offset)

    info.image_crc = CRC32().forge(
        0x00000000, elf_to_bin(elf, elf_data), 
        section.lma - elf.getSections()[0].lma
    )
    
    info.pack_into(elf_data, section.sh_offset)
    
    dprint("  image_crc  = 0x%08x" % info.image_crc)
    dprint("  image_size = %d"     % info.image_size)

    ##########

    dprint("\nSaving \"%s\"..." % args.target)

    with open(args.target, "wb") as file:
        file.write(elf_data)


if __name__ == '__main__':
    try:
        parse_args()
        patch_elf()

    except Exception as e:
        print e
        exit(1)
