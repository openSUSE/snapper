# -*- coding: utf-8 -*-
# make continuous integration using rubygem-packaging_rake_tasks
# Copyright © 2015 SUSE
# MIT license

require "packaging/tasks"
require "packaging/configuration"
# skip 'tarball' task, it's redefined here
Packaging::Tasks.load_tasks(:exclude => ["tarball.rake"])

require "yast/tasks"
yast_submit = ENV.fetch("YAST_SUBMIT", "factory").to_sym
Yast::Tasks.submit_to(yast_submit)

Packaging.configuration do |conf|
  conf.package_name.sub!(/-.*/, "") # strip branch name
  conf.package_dir    = ".obsdir" # Makefile.ci puts it there
  conf.skip_license_check << /.*/

  # defined in Rakefile in https://github.com/openSUSE/packaging_rake_tasks
  if yast_submit == :factory
    # Override values from
    # https://github.com/yast/yast-rake/blob/master/data/targets.yml
    # loaded by Yast::Tasks.submit_to() for OBS:
    # filesystems:snapper/snapper
    conf.obs_api = "https://api.opensuse.org/"
    conf.obs_project = "filesystems:snapper"
    conf.obs_sr_project = "filesystems:snapper"
    conf.obs_target = "openSUSE_Factory"
  end
end

desc 'Show configuration'
task :show_config do
  Packaging.configuration do |conf|
    puts "API: #{conf.obs_api}"
    puts "Project: #{conf.obs_project}"
    puts "SR Project: #{conf.obs_sr_project}"
    puts "Target: #{conf.obs_target}"
  end
end

desc 'Pretend to run the test suite'
task :test do
  puts 'No tests yet' if verbose
end

desc 'Build a tarball for OBS'
task :tarball do
  sh "make -f Makefile.ci package"
end
