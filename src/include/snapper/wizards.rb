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

# File:	include/snapper/wizards.ycp
# Package:	Configuration of snapper
# Summary:	Wizards definitions
# Authors:	Jiri Suchomel <jsuchome@suse.cz>
#
# $Id$
module Yast
  module SnapperWizardsInclude
    def initialize_snapper_wizards(include_target)
      Yast.import "UI"

      textdomain "snapper"

      Yast.import "Sequencer"
      Yast.import "Wizard"

      Yast.include include_target, "snapper/dialogs.rb"
    end

    # Main workflow of the snapper configuration
    # @return sequence result
    def MainSequence
      aliases = { "summary" => lambda { SummaryDialog() }, "show" => lambda do
        ShowDialog()
      end }

      sequence = {
        "ws_start" => "summary",
        "summary"  => { :abort => :abort, :next => :next, :show => "show" },
        "show"     => { :abort => :abort, :next => "summary" }
      }

      ret = Sequencer.Run(aliases, sequence)

      deep_copy(ret)
    end

    # Whole configuration of snapper
    # @return sequence result
    def SnapperSequence
      aliases = { "read" => [lambda { ReadDialog() }, true], "main" => lambda do
        MainSequence()
      end }

      sequence = {
        "ws_start" => "read",
        "read"     => { :abort => :abort, :next => "main" },
        "main"     => { :abort => :abort, :next => :next }
      }

      Wizard.CreateDialog
      Wizard.SetDesktopTitleAndIcon("snapper")
      ret = Sequencer.Run(aliases, sequence)

      UI.CloseDialog
      deep_copy(ret)
    end
  end
end
