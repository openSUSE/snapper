# encoding: utf-8

# ------------------------------------------------------------------------------
# Copyright (c) 2006-2012 Novell, Inc. All Rights Reserved.
#
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of version 2 of the GNU General Public License as published by the
# Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, contact Novell, Inc.
#
# To contact Novell about this file by physical or electronic mail, you may find
# current contact information at www.novell.com.
# ------------------------------------------------------------------------------

# File:	include/snapper/helps.ycp
# Package:	Configuration of snapper
# Summary:	Help texts of all the dialogs
# Authors:	Jiri Suchomel <jsuchome@suse.cz>
#
# $Id$
module Yast
  module SnapperHelpsInclude
    def initialize_snapper_helps(include_target)
      textdomain "snapper"

      # All helps are here
      @HELPS = {
        # Read dialog help
        "read"        => _(
          "<p><b><big>Reading the list of snapshots</big></b><br>\n</p>\n"
        ),
        # Summary dialog help:
        "summary"     => _(
          "<p><b><big>Snapshots Configuration</big></b><p>\n" +
            "<p>The table shows a list of root filesystem snapshots. There are three types\n" +
            "of snapshots, <b>single</b>, <b>pre</b> and <b>post</b>. Single snapshots are\n" +
            "used for storing the file system state in a certain time, while Pre and Post are used to define the changes done by special operation performed between taking those two snapshots. Pre and Post snapshots are coupled together in the table.</p>\n" +
            "<p>Select a snapshot or snapshot couple and click <b>Show Changes</b> to see the\n" +
            "new file system changes in the specified snapshot.</p>\n"
        ),
        # Show snapshot dialog help
        "show_couple" => _(
          "<p><b><big>Snapshot Overview</big></b><p>\n" +
            "<p>\n" +
            "The tree shows all the files that were modified between creating the first ('pre') and second ('post') snapshot. On the right side, you see the description generated when the first snapshot was created and the time of creation for both snapshots.\n" +
            "</p>\n" +
            "<p>\n" +
            "When a file is selected in the tree, you see the changes done to it. By default, changes between selected coupled snapshots are shown, but it is possible to compare the file with different versions.\n" +
            "</p>\n"
        ),
        # Show snapshot dialog help, alternative for single snapshots
        "show_single" => _(
          "<p><b><big>Snapshot Overview</big></b><p>\n" +
            "<p>\n" +
            "The tree shows all the files that differ in a selected snapshot and the current system. On the right side, you see the snapshot description and time of its creation.\n" +
            "</p>\n" +
            "<p>\n" +
            "When a file is selected in the tree, you can see the its difference between snapshot version and current system.\n" +
            "</p>\n"
        )
      } 

      # EOF
    end
  end
end
