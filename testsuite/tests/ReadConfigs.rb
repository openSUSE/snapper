# encoding: utf-8

module Yast
  class ReadConfigsClient < Client
    def main
      # testedfiles: Snapper.ycp

      Yast.import "Testsuite"
      Yast.import "Snapper"

      @READ = { "snapper" => { "configs" => [] } }

      Testsuite.Test(lambda { Snapper.ReadConfigs }, [@READ, {}, {}], 0)
      Testsuite.Test(lambda { Snapper.current_config }, [@READ, {}, {}], 0)

      Ops.set(@READ, ["snapper", "configs"], nil)

      Testsuite.Test(lambda { Snapper.ReadConfigs }, [@READ, {}, {}], 0)
      Testsuite.Test(lambda { Snapper.current_config }, [@READ, {}, {}], 0)

      Ops.set(@READ, ["snapper", "configs"], ["opt", "var", "root"])

      Testsuite.Test(lambda { Snapper.ReadConfigs }, [@READ, {}, {}], 0)
      Testsuite.Test(lambda { Snapper.current_config }, [@READ, {}, {}], 0)

      Ops.set(@READ, ["snapper", "configs"], ["opt", "var"])

      Testsuite.Test(lambda { Snapper.ReadConfigs }, [@READ, {}, {}], 0)
      Testsuite.Test(lambda { Snapper.current_config }, [@READ, {}, {}], 0)

      nil
    end
  end
end

Yast::ReadConfigsClient.new.main
