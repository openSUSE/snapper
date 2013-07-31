# encoding: utf-8

module Yast
  class GetSnapshotPathClient < Client
    def main
      # testedfiles: Snapper.ycp

      Yast.import "Snapper"
      Yast.import "Testsuite"

      @READ = { "snapper" => { "path" => nil } }
      Testsuite.Test(lambda { Snapper.GetSnapshotPath(0) }, [@READ, {}, {}], 0)

      Ops.set(@READ, ["snapper", "path"], "/snapshots/0/snapshot")

      Testsuite.Test(lambda { Snapper.GetSnapshotPath(0) }, [@READ, {}, {}], 0)

      nil
    end
  end
end

Yast::GetSnapshotPathClient.new.main
