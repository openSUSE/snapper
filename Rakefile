# -*- coding: utf-8 -*-
# make continuous integration using rubygem-packaging_rake_tasks
# Copyright © 2015 SUSE
# MIT license

require "packaging/tasks"
require "packaging/configuration"
# skip 'tarball' task, it's redefined here
Packaging::Tasks.load_tasks(:exclude => ["tarball.rake"])

require "yast/tasks"
Yast::Tasks.submit_to(:sle12sp2)

Packaging.configuration do |conf|
  conf.package_name.sub!(/-.*/, "") # strip branch name
  conf.package_dir    = ".obsdir" # Makefile.ci puts it there
  conf.skip_license_check << /.*/
end

desc 'Pretend to run the test suite'
task :test do
  puts 'No tests yet' if verbose
end

desc 'Build a tarball for OBS'
task :tarball do
  sh "make -f Makefile.ci package"
end
