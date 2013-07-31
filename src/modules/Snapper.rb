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

# File:	modules/Snapper.ycp
# Summary:	Snapper settings, input and output functions
# Authors:	Jiri Suchomel <jsuchome@suse.cz>
#
# $Id$
#
# Representation of the configuration of snapper.
# Input and output routines.
require "yast"

module Yast
  class SnapperClass < Module
    def main
      Yast.import "UI"
      textdomain "snapper"

      Yast.import "FileUtils"
      Yast.import "Label"
      Yast.import "Progress"
      Yast.import "Report"
      Yast.import "String"

      # global list of all snapshot
      @snapshots = []

      @selected_snapshot = {}

      # mapping of snapshot number to index in snapshots list
      @id2index = {}

      # index to snapshots list
      @selected_snapshot_index = 0

      # list of configurations
      @configs = ["root"]

      @current_config = "root"
    end

    # Return map of files modified between given snapshots
    # Return structure has just one level, and maps each modified file to it's modification map
    def ReadModifiedFilesIndex(from, to)
      Convert.convert(
        SCR.Read(path(".snapper.diff_index"), { "from" => from, "to" => to }),
        :from => "any",
        :to   => "map <string, map>"
      )
    end

    # Return map of files modified between given snapshots
    # Map is recursively describing the filesystem structure; helps to build Tree widget contents
    def ReadModifiedFilesMap(from, to)
      Convert.convert(
        SCR.Read(path(".snapper.diff_tree"), { "from" => from, "to" => to }),
        :from => "any",
        :to   => "map <string, map>"
      )
    end

    # Return the path to given snapshot
    def GetSnapshotPath(snapshot_num)
      ret = Convert.to_string(
        SCR.Read(path(".snapper.path"), { "num" => snapshot_num })
      )
      if ret == nil
        ret = ""
        # popup error
        Report.Error(
          Builtins.sformat(_("Snapshot '%1' was not found."), snapshot_num)
        )
      end
      ret
    end

    # Return the full path to the given file from currently selected configuration (subvolume)
    # @param [String] file path, relatively to current config
    # GetFileFullPath ("/testfile.txt") -> /abc/testfile.txt for /abc subvolume
    def GetFileFullPath(file)
      Ops.get_string(
        @snapshots,
        [@selected_snapshot_index, "files_index", file, "full_path"],
        file
      )
    end

    # Describe what was done with given file between given snapshots
    # - when new is 0, meaning is 'current system'
    def GetFileModification(file, old, new)
      ret = {}
      file1 = Builtins.sformat("%1%2", GetSnapshotPath(old), file)
      file2 = Builtins.sformat("%1%2", GetSnapshotPath(new), file)
      file2 = GetFileFullPath(file) if new == 0

      Builtins.y2milestone("comparing '%1' and '%2'", file1, file2)

      if FileUtils.Exists(file1) && FileUtils.Exists(file2)
        status = ["no_change"]
        out = Convert.to_map(
          SCR.Execute(
            path(".target.bash_output"),
            Builtins.sformat(
              "/usr/bin/diff -u '%1' '%2'",
              String.Quote(file1),
              String.Quote(file2)
            )
          )
        )
        if Ops.get_string(out, "stderr", "") != ""
          Builtins.y2warning("out: %1", out)
          Ops.set(ret, "diff", Ops.get_string(out, "stderr", ""))
        # the file diff
        elsif Ops.get(out, "stdout") != ""
          status = ["diff"]
          Ops.set(ret, "diff", Ops.get_string(out, "stdout", ""))
        end

        # check mode and ownerships
        out = Convert.to_map(
          SCR.Execute(
            path(".target.bash_output"),
            Builtins.sformat(
              "ls -ld -- '%1' '%2' | cut -f 1,3,4 -d ' '",
              String.Quote(file1),
              String.Quote(file2)
            )
          )
        )
        parts = Builtins.splitstring(Ops.get_string(out, "stdout", ""), " \n")

        if Ops.get(parts, 0, "") != Ops.get(parts, 3, "")
          status = Builtins.add(status, "mode")
          Ops.set(ret, "mode1", Ops.get(parts, 0, ""))
          Ops.set(ret, "mode2", Ops.get(parts, 3, ""))
        end
        if Ops.get(parts, 1, "") != Ops.get(parts, 4, "")
          status = Builtins.add(status, "user")
          Ops.set(ret, "user1", Ops.get(parts, 1, ""))
          Ops.set(ret, "user2", Ops.get(parts, 4, ""))
        end
        if Ops.get(parts, 2, "") != Ops.get(parts, 5, "")
          status = Builtins.add(status, "group")
          Ops.set(ret, "group1", Ops.get(parts, 2, ""))
          Ops.set(ret, "group2", Ops.get(parts, 5, ""))
        end
        Ops.set(ret, "status", status)
      elsif FileUtils.Exists(file1)
        Ops.set(ret, "status", ["removed"])
      elsif FileUtils.Exists(file2)
        Ops.set(ret, "status", ["created"])
      else
        Ops.set(ret, "status", ["none"])
      end
      deep_copy(ret)
    end

    # Read the list of snapshots
    def ReadSnapshots
      @snapshots = []
      snapshot_maps = Convert.convert(
        SCR.Read(path(".snapper.snapshots")),
        :from => "any",
        :to   => "list <map>"
      )
      snapshot_maps = [] if snapshot_maps == nil
      i = 0
      Builtins.foreach(snapshot_maps) do |snapshot|
        id = Ops.get_integer(snapshot, "num", 0)
        next if id == 0 # ignore the 'current system'
        Ops.set(snapshot, "name", Builtins.tostring(id))
        Builtins.y2debug("snapshot data: %1", snapshot)
        @snapshots = Builtins.add(@snapshots, snapshot)
        Ops.set(@id2index, id, i)
        i = Ops.add(i, 1)
      end
      true
    end

    def LastSnapperErrorMap
      Convert.to_map(SCR.Read(path(".snapper.error")))
    end


    def ReadConfigs
      @configs = Convert.convert(
        SCR.Read(path(".snapper.configs")),
        :from => "any",
        :to   => "list <string>"
      )
      if @configs == nil
        # error popup
        Report.Error(_("File /etc/sysconfig/snapper is not available."))
        @configs = ["root"]
      end
      if !Builtins.contains(@configs, "root") &&
          Ops.greater_than(Builtins.size(@configs), 0)
        @current_config = Ops.get(@configs, 0, "root")
      end
      deep_copy(@configs)
    end



    # Initialize snapper agent
    # Return true on success
    def InitializeSnapper(config)
      init = Convert.to_boolean(
        SCR.Execute(path(".snapper"), { "config" => config })
      )
      if !init
        err_map = LastSnapperErrorMap()
        type = Ops.get_string(err_map, "type", "")
        details = _("Reason not known.")
        if type == "config_not_found"
          details = _("Configuration not found.")
        elsif type == "config_invalid"
          details = _("Configuration is not valid.")
        end

        Builtins.y2warning("init failed with '%1'", err_map)
        # error popup
        Report.Error(
          Builtins.sformat(
            _("Failed to initialize snapper library:\n%1"),
            details
          )
        )
      end
      init
    end

    # Delete existing snapshot
    # Return true on success
    def DeleteSnapshot(args)
      args = deep_copy(args)
      success = Convert.to_boolean(SCR.Execute(path(".snapper.delete"), args))
      if !success
        err_map = LastSnapperErrorMap()
        type = Ops.get_string(err_map, "type", "")
        details = _("Reason not known.")

        details = _("Snapshot was not found.") if type == "not_found"

        Builtins.y2warning("deleting failed with '%1'", err_map)
        # error popup
        Report.Error(
          Builtins.sformat(_("Failed to delete snapshot:\n%1"), details)
        )
      end
      success
    end
    # Modify existing snapshot
    # Return true on success
    def ModifySnapshot(args)
      args = deep_copy(args)
      success = Convert.to_boolean(SCR.Execute(path(".snapper.modify"), args))
      if !success
        err_map = LastSnapperErrorMap()
        type = Ops.get_string(err_map, "type", "")
        details = _("Reason not known.")

        Builtins.y2warning("modification failed with '%1'", err_map)
        # error popup
        Report.Error(
          Builtins.sformat(_("Failed to modify snapshot:\n%1"), details)
        )
      end
      success
    end

    # Create new snapshot
    # Return true on success
    def CreateSnapshot(args)
      args = deep_copy(args)
      success = Convert.to_boolean(SCR.Execute(path(".snapper.create"), args))
      if !success
        err_map = LastSnapperErrorMap()
        type = Ops.get_string(err_map, "type", "")
        details = _("Reason not known.")

        if type == "wrong_snapshot_type"
          details = _("Wrong snapshot type given.")
        elsif type == "pre_not_given"
          details = _("'Pre' snapshot was not given.")
        elsif type == "pre_not_found"
          details = _("Given 'Pre' snapshot was not found.")
        end

        Builtins.y2warning("creating failed with '%1'", err_map)
        # error popup
        Report.Error(
          Builtins.sformat(_("Failed to create new snapshot:\n%1"), details)
        )
      end
      success
    end

    # Read all snapper settings
    # @return true on success
    def Read
      # Snapper read dialog caption
      caption = _("Initializing Snapper")

      steps = 2

      # We do not set help text here, because it was set outside
      Progress.New(
        caption,
        " ",
        steps,
        [
          # Progress stage 1/3
          _("Read the list of snapshots")
        ],
        [
          # Progress step 1/3
          _("Reading the database..."),
          # Progress finished
          _("Finished")
        ],
        ""
      )

      Progress.NextStage

      ReadConfigs()

      return false if !InitializeSnapper(@current_config)

      ReadSnapshots()

      Progress.NextStage
      true
    end

    # Return the given file mode as octal number
    def GetFileMode(file)
      out = Convert.to_map(
        SCR.Execute(
          path(".target.bash_output"),
          Builtins.sformat("/bin/stat --printf=%%a '%1'", String.Quote(file))
        )
      )
      mode = Ops.get_string(out, "stdout", "")
      return 644 if mode == nil || mode == ""
      Builtins.tointeger(mode)
    end

    # Copy given files from selected snapshot to current filesystem
    # @param [Fixnum] snapshot_num snapshot identifier
    # @param [Array<String>] files list of full paths to files
    # @return success
    def RestoreFiles(snapshot_num, files)
      files = deep_copy(files)
      ret = true
      Builtins.y2milestone("going to restore files %1", files)

      UI.OpenDialog(
        Opt(:decorated),
        HBox(
          HSpacing(1.5),
          VBox(
            HSpacing(60),
            # label for log window
            LogView(Id(:log), _("Restoring Files..."), 8, 0),
            ProgressBar(Id(:progress), "", Builtins.size(files), 0),
            PushButton(Id(:ok), Label.OKButton)
          ),
          HSpacing(1.5)
        )
      )

      UI.ChangeWidget(Id(:ok), :Enabled, false)
      progress = 0
      Builtins.foreach(files) do |file|
        UI.ChangeWidget(Id(:progress), :Value, progress)
        orig = Ops.add(GetSnapshotPath(snapshot_num), file)
        full_path = GetFileFullPath(file)
        dir = Builtins.substring(
          full_path,
          0,
          Builtins.findlastof(full_path, "/")
        )
        if !FileUtils.Exists(orig)
          SCR.Execute(
            path(".target.bash"),
            Builtins.sformat("/bin/rm -rf -- '%1'", String.Quote(full_path))
          )
          Builtins.y2milestone("removing '%1' from system", full_path)
          # log entry (%1 is file name)
          UI.ChangeWidget(
            Id(:log),
            :LastLine,
            Builtins.sformat(_("Deleted %1\n"), full_path)
          )
        elsif FileUtils.CheckAndCreatePath(dir)
          Builtins.y2milestone(
            "copying '%1' to '%2' (dir: %3)",
            orig,
            file,
            dir
          )
          if FileUtils.IsDirectory(orig) == true
            stat = Convert.to_map(SCR.Read(path(".target.stat"), orig))
            if !FileUtils.Exists(full_path)
              SCR.Execute(path(".target.mkdir"), full_path)
            end
            SCR.Execute(
              path(".target.bash"),
              Builtins.sformat(
                "/bin/chown -- %1:%2 '%3'",
                Ops.get_integer(stat, "uid", 0),
                Ops.get_integer(stat, "gid", 0),
                String.Quote(full_path)
              )
            )
            SCR.Execute(
              path(".target.bash"),
              Builtins.sformat(
                "/bin/chmod -- %1 '%2'",
                GetFileMode(orig),
                String.Quote(full_path)
              )
            )
          else
            SCR.Execute(
              path(".target.bash"),
              Builtins.sformat(
                "/bin/cp -a -- '%1' '%2'",
                String.Quote(orig),
                String.Quote(full_path)
              )
            )
          end
          UI.ChangeWidget(Id(:log), :LastLine, Ops.add(full_path, "\n"))
        else
          Builtins.y2milestone(
            "failed to copy file '%1' to '%2' (dir: %3)",
            orig,
            full_path,
            dir
          )
          # log entry (%1 is file name)
          UI.ChangeWidget(
            Id(:log),
            :LastLine,
            Builtins.sformat(_("%1 skipped\n"), full_path)
          )
        end
        Builtins.sleep(100)
        progress = Ops.add(progress, 1)
      end

      UI.ChangeWidget(Id(:progress), :Value, progress)
      UI.ChangeWidget(Id(:ok), :Enabled, true)

      UI.UserInput
      UI.CloseDialog

      ret
    end

    publish :variable => :snapshots, :type => "list <map>"
    publish :variable => :selected_snapshot, :type => "map"
    publish :variable => :id2index, :type => "map <integer, integer>"
    publish :variable => :selected_snapshot_index, :type => "integer"
    publish :variable => :configs, :type => "list <string>"
    publish :variable => :current_config, :type => "string"
    publish :function => :ReadModifiedFilesIndex, :type => "map <string, map> (integer, integer)"
    publish :function => :ReadModifiedFilesMap, :type => "map <string, map> (integer, integer)"
    publish :function => :GetSnapshotPath, :type => "string (integer)"
    publish :function => :GetFileFullPath, :type => "string (string)"
    publish :function => :GetFileModification, :type => "map (string, integer, integer)"
    publish :function => :ReadSnapshots, :type => "boolean ()"
    publish :function => :LastSnapperErrorMap, :type => "map ()"
    publish :function => :ReadConfigs, :type => "list <string> ()"
    publish :function => :InitializeSnapper, :type => "boolean (string)"
    publish :function => :DeleteSnapshot, :type => "boolean (map)"
    publish :function => :ModifySnapshot, :type => "boolean (map)"
    publish :function => :CreateSnapshot, :type => "boolean (map)"
    publish :function => :Read, :type => "boolean ()"
    publish :function => :RestoreFiles, :type => "boolean (integer, list <string>)"
  end

  Snapper = SnapperClass.new
  Snapper.main
end
