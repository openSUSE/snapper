# encoding: utf-8

module Yast
  class GetFileModeClient < Client
    def main
      # testedfiles: Snapper.ycp

      Yast.import "Snapper"
      Yast.import "Testsuite"

      @EX = { "target" => { "bash_output" => { "stdout" => "755" } } }
      Testsuite.Test(lambda { Snapper.GetFileMode("/tmp/1") }, [{}, {}, @EX], 0)

      Ops.set(
        @EX,
        ["target", "bash_output"],
        {
          "stdout" => "",
          "stderr" => "/bin/stat: cannot stat `/tmp/2': No such file or directory\n"
        }
      )

      Testsuite.Test(lambda { Snapper.GetFileMode("/tmp/2") }, [{}, {}, @EX], 0)

      nil
    end
  end
end

Yast::GetFileModeClient.new.main
