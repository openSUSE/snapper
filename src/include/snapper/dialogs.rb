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

# File:	include/snapper/dialogs.ycp
# Package:	Configuration of snapper
# Summary:	Dialogs definitions
# Authors:	Jiri Suchomel <jsuchome@suse.cz>
#
# $Id$
module Yast
  module SnapperDialogsInclude
    def initialize_snapper_dialogs(include_target)
      Yast.import "UI"

      textdomain "snapper"

      Yast.import "Confirm"
      Yast.import "FileUtils"
      Yast.import "Label"
      Yast.import "Popup"
      Yast.import "Wizard"
      Yast.import "Snapper"
      Yast.import "String"

      Yast.include include_target, "snapper/helps.rb"
    end

    def ReallyAbort
      Popup.ReallyAbort(true)
    end

    # Read settings dialog
    # @return `abort if aborted and `next otherwise
    def ReadDialog
      return :abort if !Confirm.MustBeRoot

      Wizard.RestoreHelp(Ops.get_string(@HELPS, "read", ""))
      ret = Snapper.Read
      ret ? :next : :abort
    end

    # convert map of userdata to string
    # $[ "a" : "b", "1" : "2" ] -> "a=b,1=2"
    def userdata2string(userdata)
      userdata = deep_copy(userdata)
      Builtins.mergestring(Builtins.maplist(userdata) do |key, val|
        Builtins.sformat("%1=%2", key, val)
      end, ",")
    end

    # transform userdata from widget to map
    def get_userdata(id)
      u = {}
      user_s = Convert.to_string(UI.QueryWidget(Id(id), :Value))
      Builtins.foreach(Builtins.splitstring(user_s, ",")) do |line|
        split = Builtins.splitstring(line, "=")
        if Ops.greater_than(Builtins.size(split), 1)
          Ops.set(u, Ops.get(split, 0, ""), Ops.get(split, 1, ""))
        end
      end
      deep_copy(u)
    end

    # generate list of items for Cleanup combo box
    def cleanup_items(current)
      Builtins.maplist(["timeline", "number", ""]) do |cleanup|
        Item(Id(cleanup), cleanup, cleanup == current)
      end
    end

    # compare editable parts of snapshot maps
    def snapshot_modified(orig, new)
      orig = deep_copy(orig)
      new = deep_copy(new)
      ret = false
      Builtins.foreach(
        Convert.convert(new, :from => "map", :to => "map <string, any>")
      ) { |key, value| ret = ret || Ops.get(orig, key) != value }
      ret
    end

    # Popup for modification of existing snapshot
    # @return true if new snapshot was created
    def ModifySnapshotPopup(snapshot)
      snapshot = deep_copy(snapshot)
      modified = false
      num = Ops.get_integer(snapshot, "num", 0)
      previous_num = Ops.get_integer(snapshot, "pre_num", num)
      type = Ops.get_symbol(snapshot, "type", :none)

      pre_index = Ops.get(Snapper.id2index, previous_num, 0)
      pre_snapshot = Ops.get(Snapper.snapshots, pre_index, {})

      snapshot_term = lambda do |prefix, data|
        data = deep_copy(data)
        HBox(
          HSpacing(),
          Frame(
            "",
            HBox(
              HSpacing(0.4),
              VBox(
                # text entry label
                TextEntry(
                  Id(Ops.add(prefix, "description")),
                  _("Description"),
                  Ops.get_string(data, "description", "")
                ),
                # text entry label
                TextEntry(
                  Id(Ops.add(prefix, "userdata")),
                  _("User data"),
                  userdata2string(Ops.get_map(data, "userdata", {}))
                ),
                Left(
                  ComboBox(
                    Id(Ops.add(prefix, "cleanup")),
                    Opt(:editable, :hstretch),
                    # combo box label
                    _("Cleanup algorithm"),
                    cleanup_items(Ops.get_string(data, "cleanup", ""))
                  )
                )
              ),
              HSpacing(0.4)
            )
          ),
          HSpacing()
        )
      end

      cont = VBox(
        # popup label, %1 is number
        Label(Builtins.sformat(_("Modify Snapshot %1"), num)),
        snapshot_term.call("", snapshot)
      )

      if type == :POST
        cont = VBox(
          # popup label, %1, %2 are numbers (range)
          Label(
            Builtins.sformat(_("Modify Snapshots %1 - %2"), previous_num, num)
          ),
          # label
          Left(Label(Builtins.sformat(_("Pre (%1)"), previous_num))),
          snapshot_term.call("pre_", pre_snapshot),
          VSpacing(),
          # label
          Left(Label(Builtins.sformat(_("Post (%1)"), num))),
          snapshot_term.call("", snapshot)
        )
      end

      UI.OpenDialog(
        Opt(:decorated),
        HBox(
          HSpacing(1),
          VBox(
            VSpacing(0.5),
            HSpacing(65),
            cont,
            VSpacing(0.5),
            ButtonBox(
              PushButton(Id(:ok), Label.OKButton),
              PushButton(Id(:cancel), Label.CancelButton)
            ),
            VSpacing(0.5)
          ),
          HSpacing(1)
        )
      )

      ret = nil
      args = {}
      pre_args = {}

      while true
        ret = UI.UserInput
        args = {
          "num"         => num,
          "description" => UI.QueryWidget(Id("description"), :Value),
          "cleanup"     => UI.QueryWidget(Id("cleanup"), :Value),
          "userdata"    => get_userdata("userdata")
        }
        if type == :POST
          pre_args = {
            "num"         => previous_num,
            "description" => UI.QueryWidget(Id("pre_description"), :Value),
            "cleanup"     => UI.QueryWidget(Id("pre_cleanup"), :Value),
            "userdata"    => get_userdata("pre_userdata")
          }
        end
        break if ret == :ok || ret == :cancel
      end
      UI.CloseDialog
      if ret == :ok
        if snapshot_modified(snapshot, args)
          modified = Snapper.ModifySnapshot(args)
        end
        if type == :POST && snapshot_modified(pre_snapshot, pre_args)
          modified = Snapper.ModifySnapshot(pre_args) || modified
        end
      end

      modified
    end

    # Popup for creating new snapshot
    # @return true if new snapshot was created
    def CreateSnapshotPopup(pre_snapshots)
      pre_snapshots = deep_copy(pre_snapshots)
      created = false
      pre_items = Builtins.maplist(pre_snapshots) do |s|
        Item(Id(s), Builtins.tostring(s))
      end

      UI.OpenDialog(
        Opt(:decorated),
        HBox(
          HSpacing(1),
          VBox(
            VSpacing(0.5),
            HSpacing(65),
            # popup label
            Label(_("Create New Snapshot")),
            # text entry label
            TextEntry(Id("description"), _("Description"), ""),
            RadioButtonGroup(
              Id(:rb_type),
              Left(
                HVSquash(
                  VBox(
                    Left(
                      RadioButton(
                        Id("single"),
                        Opt(:notify),
                        # radio button label
                        _("Single snapshot"),
                        true
                      )
                    ),
                    Left(
                      RadioButton(
                        Id("pre"),
                        Opt(:notify),
                        # radio button label
                        _("Pre"),
                        false
                      )
                    ),
                    VBox(
                      Left(
                        RadioButton(
                          Id("post"),
                          Opt(:notify),
                          # radio button label, snapshot selection will follow
                          _("Post, paired with:"),
                          false
                        )
                      ),
                      HBox(
                        HSpacing(2),
                        Left(
                          ComboBox(Id(:pre_list), Opt(:notify), "", pre_items)
                        )
                      )
                    )
                  )
                )
              )
            ),
            # text entry label
            TextEntry(Id("userdata"), _("User data"), ""),
            # text entry label
            ComboBox(
              Id("cleanup"),
              Opt(:editable, :hstretch),
              _("Cleanup algorithm"),
              cleanup_items("")
            ),
            VSpacing(0.5),
            ButtonBox(
              PushButton(Id(:ok), Label.OKButton),
              PushButton(Id(:cancel), Label.CancelButton)
            ),
            VSpacing(0.5)
          ),
          HSpacing(1)
        )
      )

      UI.ChangeWidget(
        Id("post"),
        :Enabled,
        Ops.greater_than(Builtins.size(pre_items), 0)
      )
      UI.ChangeWidget(
        Id(:pre_list),
        :Enabled,
        Ops.greater_than(Builtins.size(pre_items), 0)
      )

      ret = nil
      args = {}
      while true
        ret = UI.UserInput
        args = {
          "type"        => UI.QueryWidget(Id(:rb_type), :Value),
          "description" => UI.QueryWidget(Id("description"), :Value),
          "pre"         => UI.QueryWidget(Id(:pre_list), :Value),
          "cleanup"     => UI.QueryWidget(Id("cleanup"), :Value),
          "userdata"    => get_userdata("userdata")
        }
        break if ret == :ok || ret == :cancel
      end
      UI.CloseDialog
      created = Snapper.CreateSnapshot(args) if ret == :ok
      created
    end

    # Popup for deleting existing snapshot
    # @return true if snapshot was deleted
    def DeleteSnapshotPopup(snapshot)
      snapshot = deep_copy(snapshot)
      # yes/no popup question
      if Popup.YesNo(
          Builtins.sformat(
            _("Really delete snapshot '%1'?"),
            Ops.get_integer(snapshot, "num", 0)
          )
        )
        return Snapper.DeleteSnapshot(snapshot)
      end
      false
    end

    # Summary dialog
    # @return dialog result
    def SummaryDialog
      # summary dialog caption
      caption = _("Snapshots")

      snapshots = deep_copy(Snapper.snapshots)
      configs = deep_copy(Snapper.configs)

      snapshot_items = []
      # lonely pre snapshots
      pre_snapshots = []

      # generate list of snapshot table items
      get_snapshot_items = lambda do
        i = -1
        snapshot_items = []
        pre_snapshots = []

        Builtins.foreach(snapshots) do |s|
          i = Ops.add(i, 1)
          num = Ops.get_integer(s, "num", 0)
          date = ""
          if num != 0
            date = Builtins.timestring(
              "%c",
              Ops.get_integer(s, "date", 0),
              false
            )
          end
          userdata = userdata2string(Ops.get_map(s, "userdata", {}))
          if Ops.get_symbol(s, "type", :none) == :SINGLE
            snapshot_items = Builtins.add(
              snapshot_items,
              Item(
                Id(i),
                num,
                _("Single"),
                date,
                "",
                Ops.get_string(s, "description", ""),
                userdata
              )
            )
          elsif Ops.get_symbol(s, "type", :none) == :POST
            pre = Ops.get_integer(s, "pre_num", 0) # pre canot be 0
            index = Ops.get(Snapper.id2index, pre, -1)
            if pre == 0 || index == -1
              Builtins.y2warning(
                "something wrong - pre:%1, index:%2",
                pre,
                index
              )
              next
            end
            desc = Ops.get_string(Snapper.snapshots, [index, "description"], "")
            pre_date = Builtins.timestring(
              "%c",
              Ops.get_integer(Snapper.snapshots, [index, "date"], 0),
              false
            )
            snapshot_items = Builtins.add(
              snapshot_items,
              Item(
                Id(i),
                Builtins.sformat("%1 - %2", pre, num),
                _("Pre & Post"),
                pre_date,
                date,
                desc,
                userdata
              )
            )
          else
            post = Ops.get_integer(s, "post_num", 0) # 0 means there's no post
            if post == 0
              Builtins.y2milestone("pre snappshot %1 does not have post", num)
              snapshot_items = Builtins.add(
                snapshot_items,
                Item(
                  Id(i),
                  num,
                  _("Pre"),
                  date,
                  "",
                  Ops.get_string(s, "description", ""),
                  userdata
                )
              )
              pre_snapshots = Builtins.add(pre_snapshots, num)
            else
              Builtins.y2milestone("skipping pre snapshot: %1", num)
            end
          end
        end
        deep_copy(snapshot_items)
      end

      # update list of snapshots
      update_snapshots = lambda do
        # busy popup message
        Popup.ShowFeedback("", _("Reading list of snapshots..."))

        Snapper.InitializeSnapper(Snapper.current_config)
        Snapper.ReadSnapshots
        snapshots = deep_copy(Snapper.snapshots)
        Popup.ClearFeedback

        UI.ChangeWidget(Id(:snapshots_table), :Items, get_snapshot_items.call)
        UI.ChangeWidget(
          Id(:show),
          :Enabled,
          Ops.greater_than(Builtins.size(snapshot_items), 0)
        )

        nil
      end


      contents = VBox(
        HBox(
          # combo box label
          Label(_("Current Configuration")),
          ComboBox(Id(:configs), Opt(:notify), "", Builtins.maplist(configs) do |config|
            Item(Id(config), config, config == Snapper.current_config)
          end),
          HStretch()
        ),
        Table(
          Id(:snapshots_table),
          Opt(:notify, :keepSorting),
          Header(
            # table header
            _("ID"),
            _("Type"),
            _("Start Date"),
            _("End Date"),
            _("Description"),
            _("User Data")
          ),
          get_snapshot_items.call
        ),
        HBox(
          # button label
          PushButton(Id(:show), Opt(:default), _("Show Changes")),
          PushButton(Id(:create), Label.CreateButton),
          # button label
          PushButton(Id(:modify), _("Modify")),
          PushButton(Id(:delete), Label.DeleteButton),
          HStretch()
        )
      )

      Wizard.SetContentsButtons(
        caption,
        contents,
        Ops.get_string(@HELPS, "summary", ""),
        Label.BackButton,
        Label.CloseButton
      )
      Wizard.HideBackButton
      Wizard.HideAbortButton

      UI.SetFocus(Id(:snapshots_table))
      UI.ChangeWidget(Id(:show), :Enabled, false) if snapshot_items == []
      UI.ChangeWidget(
        Id(:configs),
        :Enabled,
        Ops.greater_than(Builtins.size(configs), 1)
      )

      ret = nil
      while true
        ret = UI.UserInput

        selected = Convert.to_integer(
          UI.QueryWidget(Id(:snapshots_table), :CurrentItem)
        )

        if ret == :abort || ret == :cancel || ret == :back
          if ReallyAbort()
            break
          else
            next
          end
        elsif ret == :show
          if Ops.get(snapshots, [selected, "type"]) == :PRE
            # popup message
            Popup.Message(
              _(
                "This 'Pre' snapshot is not paired with any 'Post' one yet.\nShowing differences is not possible."
              )
            )
            next
          end
          # `POST snapshot is selected from the couple
          Snapper.selected_snapshot = Ops.get(snapshots, selected, {})
          Snapper.selected_snapshot_index = selected
          break
        elsif ret == :configs
          config = Convert.to_string(UI.QueryWidget(Id(ret), :Value))
          if config != Snapper.current_config
            Snapper.current_config = config
            update_snapshots.call
            next
          end
        elsif ret == :create
          if CreateSnapshotPopup(pre_snapshots)
            update_snapshots.call
            next
          end
        elsif ret == :modify
          if ModifySnapshotPopup(Ops.get(snapshots, selected, {}))
            update_snapshots.call
            next
          end
        elsif ret == :delete
          if DeleteSnapshotPopup(Ops.get(snapshots, selected, {}))
            update_snapshots.call
            next
          end
        elsif ret == :next
          break
        else
          Builtins.y2error("unexpected retcode: %1", ret)
          next
        end
      end

      deep_copy(ret)
    end

    # @return dialog result
    def ShowDialog
      # dialog caption
      caption = _("Selected Snapshot Overview")

      display_info = UI.GetDisplayInfo
      textmode = Ops.get_boolean(display_info, "TextMode", false)
      previous_file = ""
      current_file = ""

      # map of already read files
      files = {}
      # currently read subtree
      subtree = []
      tree_items = []
      open_items = {}

      snapshot = deep_copy(Snapper.selected_snapshot)
      snapshot_num = Ops.get_integer(snapshot, "num", 0)
      # map of whole tree (recursive)
      tree_map = Ops.get_map(snapshot, "tree_map", {})
      previous_num = Ops.get_integer(snapshot, "pre_num", snapshot_num)
      pre_index = Ops.get(Snapper.id2index, previous_num, 0)
      description = Ops.get_string(
        Snapper.snapshots,
        [pre_index, "description"],
        ""
      )
      pre_date = Builtins.timestring(
        "%c",
        Ops.get_integer(Snapper.snapshots, [pre_index, "date"], 0),
        false
      )
      date = Builtins.timestring(
        "%c",
        Ops.get_integer(snapshot, "date", 0),
        false
      )
      type = Ops.get_symbol(snapshot, "type", :NONE)
      combo_items = []
      Builtins.foreach(Snapper.snapshots) do |s|
        id = Ops.get_integer(s, "num", 0)
        if id != snapshot_num
          # '%1: %2' means 'ID: description', adapt the order if necessary
          combo_items = Builtins.add(
            combo_items,
            Item(
              Id(id),
              Builtins.sformat(
                _("%1: %2"),
                id,
                Ops.get_string(s, "description", "")
              )
            )
          )
        end
      end

      from = snapshot_num
      to = 0 # current system
      if Ops.get_symbol(snapshot, "type", :NONE) == :POST
        from = Ops.get_integer(snapshot, "pre_num", 0)
        to = snapshot_num
      elsif Ops.get_symbol(snapshot, "type", :NONE) == :PRE
        to = Ops.get_integer(snapshot, "post_num", 0)
      end

      # busy popup message
      Popup.ShowFeedback("", _("Calculating changed files..."))

      if !Builtins.haskey(snapshot, "tree_map")
        Ops.set(snapshot, "tree_map", Snapper.ReadModifiedFilesMap(from, to))
        tree_map = Ops.get_map(snapshot, "tree_map", {})
      end
      # full paths of files marked as modified, mapping to changes string
      files_index = {}
      if !Builtins.haskey(snapshot, "files_index")
        Ops.set(
          snapshot,
          "files_index",
          Snapper.ReadModifiedFilesIndex(from, to)
        )
        Ops.set(Snapper.snapshots, Snapper.selected_snapshot_index, snapshot)
      end
      Popup.ClearFeedback
      files_index = Ops.get_map(snapshot, "files_index", {})

      # update the global snapshots list
      Ops.set(Snapper.snapshots, Snapper.selected_snapshot_index, snapshot)

      snapshot_name = Builtins.tostring(snapshot_num)

      # map of all items in tree (just one level)
      selected_items = {}

      file_was_created = lambda do |file|
        Builtins.substring(
          Ops.get_string(files_index, [file, "status"], ""),
          0,
          1
        ) == "+"
      end
      file_was_removed = lambda do |file|
        Builtins.substring(
          Ops.get_string(files_index, [file, "status"], ""),
          0,
          1
        ) == "-"
      end
      # go through the map defining filesystem tree and create the widget items
      generate_tree_items = lambda do |current_path, current_branch|
        current_branch = deep_copy(current_branch)
        ret2 = []
        Builtins.foreach(current_branch) do |node, branch|
          new_path = Ops.add(Ops.add(current_path, "/"), node)
          if Builtins.haskey(files_index, new_path)
            icon_f = "16x16/apps/gdu-smart-unknown.png"
            if file_was_created.call(new_path)
              icon_f = "16x16/apps/gdu-smart-healthy.png"
            elsif file_was_removed.call(new_path)
              icon_f = "16x16/apps/gdu-smart-failing.png"
            end
            ret2 = Builtins.add(
              ret2,
              Item(
                Id(new_path),
                term(:icon, icon_f),
                node,
                false,
                generate_tree_items.call(
                  new_path,
                  Convert.convert(
                    branch,
                    :from => "map",
                    :to   => "map <string, map>"
                  )
                )
              )
            )
          else
            ret2 = Builtins.add(
              ret2,
              Item(
                Id(new_path),
                node,
                false,
                generate_tree_items.call(
                  new_path,
                  Convert.convert(
                    branch,
                    :from => "map",
                    :to   => "map <string, map>"
                  )
                )
              )
            )
          end
        end
        deep_copy(ret2)
      end

      # helper function: show the specific modification between snapshots
      show_file_modification = lambda do |file, from2, to2|
        content = VBox()
        # busy popup message
        Popup.ShowFeedback("", _("Calculating file modifications..."))
        modification = Snapper.GetFileModification(file, from2, to2)
        Popup.ClearFeedback
        status = Ops.get_list(modification, "status", [])
        if Builtins.contains(status, "created")
          # label
          content = Builtins.add(
            content,
            Left(Label(_("New file was created.")))
          )
        elsif Builtins.contains(status, "removed")
          # label
          content = Builtins.add(content, Left(Label(_("File was removed."))))
        elsif Builtins.contains(status, "no_change")
          # label
          content = Builtins.add(
            content,
            Left(Label(_("File content was not changed.")))
          )
        elsif Builtins.contains(status, "none")
          # label
          content = Builtins.add(
            content,
            Left(Label(_("File does not exist in either snapshot.")))
          )
        elsif Builtins.contains(status, "diff")
          # label
          content = Builtins.add(
            content,
            Left(Label(_("File content was modified.")))
          )
        end
        if Builtins.contains(status, "mode")
          content = Builtins.add(
            content,
            Left(
              Label(
                # text label, %1, %2 are file modes (like '-rw-r--r--')
                Builtins.sformat(
                  _("File mode was changed from '%1' to '%2'."),
                  Ops.get_string(modification, "mode1", ""),
                  Ops.get_string(modification, "mode2", "")
                )
              )
            )
          )
        end
        if Builtins.contains(status, "user")
          content = Builtins.add(
            content,
            Left(
              Label(
                # text label, %1, %2 are user names
                Builtins.sformat(
                  _("File user ownership was changed from '%1' to '%2'."),
                  Ops.get_string(modification, "user1", ""),
                  Ops.get_string(modification, "user2", "")
                )
              )
            )
          )
        end
        if Builtins.contains(status, "group")
          # label
          content = Builtins.add(
            content,
            Left(
              Label(
                # text label, %1, %2 are group names
                Builtins.sformat(
                  _("File group ownership was changed from '%1' to '%2'."),
                  Ops.get_string(modification, "group1", ""),
                  Ops.get_string(modification, "group2", "")
                )
              )
            )
          )
        end

        if Builtins.haskey(modification, "diff")
          diff = String.EscapeTags(Ops.get_string(modification, "diff", ""))
          l = Builtins.splitstring(diff, "\n")
          if !textmode
            # colorize diff output
            l = Builtins.maplist(l) do |line|
              first = Builtins.substring(line, 0, 1)
              if first == "+"
                line = Builtins.sformat("<font color=blue>%1</font>", line)
              elsif first == "-"
                line = Builtins.sformat("<font color=red>%1</font>", line)
              end
              line
            end
          end
          diff = Builtins.mergestring(l, "<br>")
          if !textmode
            # show fixed font in diff
            diff = Ops.add(Ops.add("<pre>", diff), "</pre>")
          end
          content = Builtins.add(content, RichText(Id(:diff), diff))
        else
          content = Builtins.add(content, VStretch())
        end

        # button label
        restore_label = _("R&estore from First")
        # button label
        restore_label_single = _("Restore")

        if file_was_created.call(file)
          restore_label = Label.RemoveButton
          restore_label_single = Label.RemoveButton
        end

        UI.ReplaceWidget(
          Id(:diff_content),
          HBox(
            HSpacing(0.5),
            VBox(
              content,
              VSquash(
                HBox(
                  HStretch(),
                  type == :SINGLE ?
                    Empty() :
                    PushButton(Id(:restore_pre), restore_label),
                  PushButton(
                    Id(:restore),
                    type == :SINGLE ?
                      restore_label_single :
                      _("Res&tore from Second")
                  )
                )
              )
            ),
            HSpacing(0.5)
          )
        )
        if type != :SINGLE && file_was_removed.call(file)
          # file removed in 2nd snapshot cannot be restored from that snapshot
          UI.ChangeWidget(Id(:restore), :Enabled, false)
        end

        nil
      end


      # create the term for selected file
      set_entry_term = lambda do
        if current_file != "" && Builtins.haskey(files_index, current_file)
          if type == :SINGLE
            UI.ReplaceWidget(
              Id(:diff_chooser),
              HBox(
                HSpacing(0.5),
                VBox(
                  VSpacing(0.2),
                  RadioButtonGroup(
                    Id(:rd),
                    Left(
                      HVSquash(
                        VBox(
                          Left(
                            RadioButton(
                              Id(:diff_snapshot),
                              Opt(:notify),
                              # radio button label
                              _(
                                "Show the difference between snapshot and current system"
                              ),
                              true
                            )
                          ),
                          VBox(
                            Left(
                              RadioButton(
                                Id(:diff_arbitrary),
                                Opt(:notify),
                                # radio button label, snapshot selection will follow
                                _(
                                  "Show the difference between current and selected snapshot:"
                                ),
                                false
                              )
                            ),
                            HBox(
                              HSpacing(2),
                              # FIXME without label, there's no shortcut!
                              Left(
                                ComboBox(
                                  Id(:selection_snapshots),
                                  Opt(:notify),
                                  "",
                                  combo_items
                                )
                              )
                            )
                          )
                        )
                      )
                    )
                  ),
                  VSpacing()
                ),
                HSpacing(0.5)
              )
            )
            show_file_modification.call(current_file, snapshot_num, 0)
            UI.ChangeWidget(Id(:selection_snapshots), :Enabled, false)
          else
            UI.ReplaceWidget(
              Id(:diff_chooser),
              HBox(
                HSpacing(0.5),
                VBox(
                  VSpacing(0.2),
                  RadioButtonGroup(
                    Id(:rd),
                    Left(
                      HVSquash(
                        VBox(
                          Left(
                            RadioButton(
                              Id(:diff_snapshot),
                              Opt(:notify),
                              # radio button label
                              _(
                                "Show the difference between first and second snapshot"
                              ),
                              true
                            )
                          ),
                          Left(
                            RadioButton(
                              Id(:diff_pre_current),
                              Opt(:notify),
                              # radio button label
                              _(
                                "Show the difference between first snapshot and current system"
                              ),
                              false
                            )
                          ),
                          Left(
                            RadioButton(
                              Id(:diff_post_current),
                              Opt(:notify),
                              # radio button label
                              _(
                                "Show the difference between second snapshot and current system"
                              ),
                              false
                            )
                          )
                        )
                      )
                    )
                  ),
                  VSpacing()
                ),
                HSpacing(0.5)
              )
            )
            show_file_modification.call(
              current_file,
              previous_num,
              snapshot_num
            )
          end
        else
          UI.ReplaceWidget(Id(:diff_chooser), VBox(VStretch()))
          UI.ReplaceWidget(Id(:diff_content), HBox(HStretch()))
        end

        nil
      end

      tree_label = Builtins.sformat("%1 - %2", previous_num, snapshot_num)
      # find out the path to current subvolume
      subtree_path = Snapper.GetSnapshotPath(snapshot_num)
      subtree_path = Builtins.substring(
        subtree_path,
        0,
        Builtins.find(subtree_path, ".snapshots/")
      )

      date_widget = VBox(
        HBox(
          # label, date string will follow at the end of line
          Label(Id(:pre_date), _("Time of taking the first snapshot:")),
          Right(Label(pre_date))
        ),
        HBox(
          # label, date string will follow at the end of line
          Label(Id(:post_date), _("Time of taking the second snapshot:")),
          Right(Label(date))
        )
      )
      if type == :SINGLE
        tree_label = Builtins.tostring(snapshot_num)
        date_widget = HBox(
          # label, date string will follow at the end of line
          Label(Id(:date), _("Time of taking the snapshot:")),
          Right(Label(date))
        )
      end

      contents = HBox(
        HWeight(
          1,
          VBox(
            HBox(
              HSpacing(),
              ReplacePoint(
                Id(:reptree),
                VBox(Left(Label(subtree_path)), Tree(Id(:tree), tree_label, []))
              ),
              HSpacing()
            ),
            HBox(
              HSpacing(1.5),
              HStretch(),
              textmode ?
                # button label
                PushButton(Id(:open), Opt(:key_F6), _("&Open")) :
                Empty(),
              HSpacing(1.5)
            )
          )
        ),
        HWeight(
          2,
          VBox(
            Left(Label(Id(:desc), description)),
            VSquash(VWeight(1, VSquash(date_widget))),
            VWeight(
              2,
              Frame(
                "",
                HBox(
                  HSpacing(0.5),
                  VBox(
                    VSpacing(0.5),
                    VWeight(
                      1,
                      ReplacePoint(Id(:diff_chooser), VBox(VStretch()))
                    ),
                    VWeight(
                      4,
                      ReplacePoint(Id(:diff_content), HBox(HStretch()))
                    ),
                    VSpacing(0.5)
                  ),
                  HSpacing(0.5)
                )
              )
            )
          )
        )
      )

      # show the dialog contents with empty tree, compute items later
      Wizard.SetContentsButtons(
        caption,
        contents,
        type == :SINGLE ?
          Ops.get_string(@HELPS, "show_single", "") :
          Ops.get_string(@HELPS, "show_couple", ""),
        # button label
        Label.CancelButton,
        _("Restore Selected")
      )

      tree_items = generate_tree_items.call("", tree_map)

      if Ops.greater_than(Builtins.size(tree_items), 0)
        UI.ReplaceWidget(
          Id(:reptree),
          VBox(
            Left(Label(subtree_path)),
            Tree(
              Id(:tree),
              Opt(:notify, :immediate, :multiSelection, :recursiveSelection),
              tree_label,
              tree_items
            )
          )
        )
        # no item is selected
        UI.ChangeWidget(:tree, :CurrentItem, nil)
      end

      current_file = ""

      set_entry_term.call

      UI.SetFocus(Id(:tree)) if textmode

      ret = nil
      while true
        event = UI.WaitForEvent
        ret = Ops.get_symbol(event, "ID")

        previous_file = current_file
        current_file = Convert.to_string(
          UI.QueryWidget(Id(:tree), :CurrentItem)
        )
        current_file = "" if current_file == nil

        # other tree events
        if ret == :tree
          # seems like tree widget emits 2 SelectionChanged events
          if current_file != previous_file
            set_entry_term.call
            UI.SetFocus(Id(:tree)) if textmode
          end
        elsif ret == :diff_snapshot
          if type == :SINGLE
            UI.ChangeWidget(Id(:selection_snapshots), :Enabled, false)
            show_file_modification.call(current_file, snapshot_num, 0)
          else
            show_file_modification.call(
              current_file,
              previous_num,
              snapshot_num
            )
          end
        elsif ret == :diff_arbitrary || ret == :selection_snapshots
          UI.ChangeWidget(Id(:selection_snapshots), :Enabled, true)
          selected_num = Convert.to_integer(
            UI.QueryWidget(Id(:selection_snapshots), :Value)
          )
          show_file_modification.call(current_file, previous_num, selected_num)
        elsif ret == :diff_pre_current
          show_file_modification.call(current_file, previous_num, 0)
        elsif ret == :diff_post_current
          show_file_modification.call(current_file, snapshot_num, 0)
        elsif ret == :abort || ret == :cancel || ret == :back
          break
        elsif (ret == :restore_pre || ret == :restore && type == :SINGLE) &&
            file_was_created.call(current_file)
          # yes/no question, %1 is file name, %2 is number
          if Popup.YesNo(
              Builtins.sformat(
                _(
                  "Do you want to delete the file\n" +
                    "\n" +
                    "%1\n" +
                    "\n" +
                    "from current system?"
                ),
                Snapper.GetFileFullPath(current_file)
              )
            )
            Snapper.RestoreFiles(
              ret == :restore_pre ? previous_num : snapshot_num,
              [current_file]
            )
          end
          next
        elsif ret == :restore_pre
          # yes/no question, %1 is file name, %2 is number
          if Popup.YesNo(
              Builtins.sformat(
                _(
                  "Do you want to copy the file\n" +
                    "\n" +
                    "%1\n" +
                    "\n" +
                    "from snapshot '%2' to current system?"
                ),
                Snapper.GetFileFullPath(current_file),
                previous_num
              )
            )
            Snapper.RestoreFiles(previous_num, [current_file])
          end
          next
        elsif ret == :restore
          # yes/no question, %1 is file name, %2 is number
          if Popup.YesNo(
              Builtins.sformat(
                _(
                  "Do you want to copy the file\n" +
                    "\n" +
                    "%1\n" +
                    "\n" +
                    "from snapshot '%2' to current system?"
                ),
                Snapper.GetFileFullPath(current_file),
                snapshot_num
              )
            )
            Snapper.RestoreFiles(snapshot_num, [current_file])
          end
          next
        elsif ret == :next
          files2 = Convert.convert(
            UI.QueryWidget(Id(:tree), :SelectedItems),
            :from => "any",
            :to   => "list <string>"
          )
          to_restore = []
          files2 = Builtins.filter(files2) do |file|
            if Builtins.haskey(files_index, file)
              to_restore = Builtins.add(
                to_restore,
                String.EscapeTags(Snapper.GetFileFullPath(file))
              )
              next true
            else
              next false
            end
          end

          if to_restore == []
            # popup message
            Popup.Message(_("No file was selected for restoring"))
            next
          end
          # popup headline
          if Popup.AnyQuestionRichText(
              _("Restoring files"),
              # popup message, %1 is snapshot number, %2 list of files
              Builtins.sformat(
                _(
                  "<p>These files will be restored from snapshot '%1':</p>\n" +
                    "<p>\n" +
                    "%2\n" +
                    "</p>\n" +
                    "<p>Files existing in original snapshot will be copied to current system.</p>\n" +
                    "<p>Files that did not exist in the snapshot will be deleted.</p>Are you sure?"
                ),
                previous_num,
                Builtins.mergestring(to_restore, "<br>")
              ),
              60,
              20,
              Label.YesButton,
              Label.NoButton,
              :focus_no
            )
            Snapper.RestoreFiles(previous_num, files2)
            break
          end
          next
        else
          Builtins.y2error("unexpected retcode: %1", ret)
          next
        end
      end

      deep_copy(ret)
    end
  end
end
