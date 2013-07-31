# encoding: utf-8

module Yast
  class ReadSnapshotsClient < Client
    def main
      # testedfiles: Snapper.ycp

      Yast.import "Testsuite"
      Yast.import "Snapper"

      @READ = { "snapper" => { "snapshots" => nil } }

      Testsuite.Test(lambda { Snapper.ReadSnapshots }, [@READ, {}, {}], 0)
      Testsuite.Test(lambda { Snapper.snapshots }, [@READ, {}, {}], 0)

      Ops.set(
        @READ,
        ["snapper", "snapshots"],
        [
          {
            "date"        => 1297364138,
            "description" => "current system",
            "num"         => 0,
            "type"        => :SINGLE
          },
          {
            "date"        => 1297364138,
            "description" => "before yast2-users",
            "num"         => 1,
            "post_num"    => 2,
            "type"        => :PRE
          }
        ]
      )

      Testsuite.Test(lambda { Snapper.ReadSnapshots }, [@READ, {}, {}], 0)
      Testsuite.Test(lambda { Snapper.snapshots }, [@READ, {}, {}], 0)
      Testsuite.Test(lambda { Snapper.id2index }, [@READ, {}, {}], 0)

      nil
    end
  end
end

Yast::ReadSnapshotsClient.new.main
