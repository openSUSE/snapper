# -*- coding: utf-8 -*-
# make continuous integration using rubygem-packaging_rake_tasks
# Copyright © 2015 SUSE
# MIT license

require "packaging/tasks"
require "packaging/configuration"
# skip 'tarball' task, it's redefined here
Packaging::Tasks.load_tasks(:exclude => ["tarball.rake"])

Packaging.configuration do |conf|
  conf.obs_project    = "YaST:Head"
  conf.obs_sr_project = "openSUSE:Factory"
  conf.package_dir    = ".obsdir"
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
