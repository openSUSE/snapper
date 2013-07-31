# encoding: utf-8

module Yast
  class GetFileModificationClient < Client
    def main
      # testedfiles: Snapper.ycp

      Yast.import "Snapper"
      Yast.import "Testsuite"

      @READ = {
        "snapper" => { "path" => "/snapshots/1/snapshot" },
        "target"  => { "stat" => { 1 => 2 } }
      }
      @EX = {
        "target" => { "bash_output" => { "stderr" => "error while diffing" } }
      }
      Testsuite.Test(lambda { Snapper.GetFileModification("/etc/passwd", 1, 0) }, [
        @READ,
        {},
        @EX
      ], 0)

      Ops.set(@EX, ["target", "bash_output"], { "stdout" => "+new user line" })
      # status map is wrong, due to 2 calls of target.bash_output with same result...
      Testsuite.Test(lambda { Snapper.GetFileModification("/etc/passwd", 1, 2) }, [
        @READ,
        {},
        @EX
      ], 0)

      Ops.set(@READ, ["target", "stat"], {})

      Testsuite.Test(lambda { Snapper.GetFileModification("/etc/passwd", 1, 2) }, [
        @READ,
        {},
        @EX
      ], 0)

      nil
    end
  end
end

Yast::GetFileModificationClient.new.main
