# -*- coding: utf-8 -*-
# make continuous integration using rubygem-packaging_rake_tasks
# Copyright © 2015 SUSE
# MIT license

require "packaging/tasks"
require "packaging/configuration"
# skip 'tarball' task, it's redefined here
Packaging::Tasks.load_tasks(:exclude => ["tarball.rake"])

require "yast/tasks"
Yast::Tasks.submit_to(ENV.fetch("YAST_SUBMIT", "factory").to_sym)

Packaging.configuration do |conf|
  conf.package_name.sub!(/-.*/, "") # strip branch name
  conf.package_dir    = ".obsdir" # Makefile.ci puts it there
  conf.skip_license_check << /.*/

  # defined in Rakefile in https://github.com/openSUSE/packaging_rake_tasks
  conf.obs_api = "https://api.opensuse.org/"
  conf.obs_project = "filesystems:snapper"
  conf.obs_target = "openSUSE_Factory"
  conf.obs_sr_project = "filesystems:snapper"
end

desc 'Pretend to run the test suite'
task :test do
  puts 'No tests yet' if verbose
end

desc 'Build a tarball for OBS'
task :tarball do
  sh "make -f Makefile.ci package"
end
