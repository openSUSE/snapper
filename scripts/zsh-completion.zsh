#compdef snapper

local options=(
	{--quiet,-q}'[Suppress normal output]'
	{--verbose,-v}'[Increase verbosity]'
	--utc'[Display dates and times in UTC]'
	--iso'[Display dates and times in ISO format]'
	{--table-style,-t}'[Table style (integer)]:style:({0..11})'
	--abbreviate'[Allow to abbreviate table columns]'
	--machine-readable'[Set a machine-readable output format]:format:(csv json)'
	--csvout --no-headers'[Set CSV output format]'
	--jsonout'[Set JSON output format]'
	--separator'[Character separator for CSV output format]:separator'
  {--config,-c}'[Set name of config to use]:config:('${(f)$(command snapper --csvout --no-headers list-configs --columns config)}')'
	--no-dbus'[Operate without DBus]'
	{--root,-r}'[Operate on target root (works only without DBus)]: :_dirs'
	{--ambit,-a}'[Operate in the specified ambit]:ambit:(auto classic transactional)'
	"(- : *)"{--help,-h}'[Prints help information]'
	"(- : *)"--version'[Print version and exit]'
)

local subcommands=(
  'list-configs:List\ configs'
  'create-config:Create\ config'
  'delete-config:Delete\ config'
  'get-config:Get\ config'
  'set-config:Set\ config'
  'list:List\ snapshots'
  'create:Create\ snapshot'
  'modify:Modify\ snapshot'
  {delete,remove,rm}':Delete\ snapshot'
  'mount:Mount\ snapshot'
  'umount:Umount\ snapshot'
  'status:Comparing\ snapshots'
  'diff:Comparing\ snapshots'
  'xadiff:Comparing\ snapshots\ extended\ attributes'
  'undochange:Undo\ changes'
  'rollback:Rollback'
  'setup-quota:Setup\ quota'
  'cleanup:Cleanup\ snapshots'
)

_arguments -s -S $options \
  ": :(($subcommands))" \
	"*:: :->option-or-argument"

if [[ $state != option-or-argument ]]; then
  return
fi
local curcontext=${curcontext%:*:*}:snapper-$words[1]:
local algorithms=(
  'number:Deletes\ old\ snapshots\ when\ a\ certain\ number\ of\ snapshots\ is\ reached'
  'timeline:Deletes\ old\ snapshots\ but\ keeps\ a\ number\ of\ hourly,\ daily,\ weekly,\ monthly\ and\ yearly\ snapshots'
  'empty-pre-post:Deletes\ pre/post\ snapshot\ pairs\ with\ empty\ diffs'
)
local type=(single pre post)
local _number=$(command snapper --csvout --no-headers --separator : list --columns number,description)
local number=${(f)_number}
local number_without_current=${${(f)_number}[2,-1]}
case $words[1] in
list-configs)
  _arguments -s -S \
    '--columns[Columns to show separated by comma]:columns:(config subvolume)'
  ;;
create-config)
  # nixOS doesn't have /usr/share while it has /run/current-system/sw
  # other distributions (except Android) have /usr/share or /usr/local/share
  _arguments -s -S \
    {--fstype,-f}'[Manually set filesystem type]:filesystem:(btrfs ext4 lvm)' \
    {--template,-t}"[Name of config template to use]:template:(${$(echo /{usr{/local,},run/current-system/sw}/share/snapper/config-templates/*(N))##*/})" \
    ":subvolume"
  ;;
get-config)
  _arguments -s -S \
    --columns'[Columns to show separated by comma]:columns:(key value)'
  ;;
set-config)
  _arguments -s -S \
    ":configdata"
  ;;
list)
  _arguments -s -S \
  {--type,-t}'[Type of snapshots to list]:type:(($type))' \
  --disable-used-space'[Disable showing used space]' \
  {--all,-a-configs}'[List snapshots from all accessible configs]' \
  --columns'[Columns to show separated by comma]:columns:(config subvolume number default active type date user used-space cleanup description userdata pre-number post-number post-date read-only)'
  ;;
create)
  _arguments -s -S \
  {--type,-t}'[Type for snapshot]:type:(($type))' \
  --pre-number'[Number of corresponding pre snapshot]:' \
  {--print-number,-p}'[Print number of created snapshot]' \
  {--description,-d}'[Description for snapshot]:description' \
  {--cleanup-algorithm,-c}'[Cleanup algorithm for snapshot]:algorithms:(($algorithms))' \
  {--userdata,-u}'[Userdata for snapshot]:userdata' \
  --command'[Run command and create pre and post snapshots]: :{_command_names -e}' \
  --read-only'[Create read-only snapshot]' \
  --read-write'[Create read-write snapshot]' \
  --from"[Create a snapshot from the specified snapshot]:number:(($number))"
  ;;
modify)
  _arguments -s -S \
    {--description,-d}'[Description for snapshot]:description' \
    {--cleanup-algorithm,-c}'[Cleanup algorithm for snapshot]:algorithms:(($algorithms))' \
    {--userdata,-u}'[Userdata for snapshot]:userdata' \
    ": :_files"
  ;;
(delete|remove|rm))
  _arguments -s -S \
    {--sync,-s}'[Sync after deletion]' \
    "*::number without current:(($number_without_current))"
  ;;
(u|)mount|xadiff)
  _arguments -s -S \
    ":number:(($number))" \
    "*:: :_files"
    ;;
status)
  _arguments -s -S \
    {--output,-o}'[Save status to file]: :_files' \
    ":number:(($number))"
  ;;
diff)
  _arguments -s -S \
    {--input,-i}'[Read files to diff from file]: :_files' \
    --diff-cmd'[Command used for comparing files]: :{_command_names -e}' \
    {--extensions,-x}'[Extra options passed to the diff command]:diff options:_diff'
    ":number:(($number))" \
    "*:: :_files"
  ;;
undochange)
  _arguments -s -S \
    {--input,-i}'[Read files for which to undo changes from file]: :_files'
    ":number:(($number))" \
    "*:: :_files"
  ;;
rollback)
  _arguments -s -S \
    {--print-number,-p}'[Print number of second created snapshot]' \
    {--description,-d}'[Description for snapshot]:description' \
    {--cleanup-algorithm,-c}'[Cleanup algorithm for snapshot]:algorithms:(($algorithms))' \
    {--userdata,-u}'[Userdata for snapshot]:userdata' \
    ":number:(($number))"
  ;;
cleanup)
  _arguments -s -S \
    --path'[Cleanup all configs affecting path.]: :_files' \
    --free-space'[Try to make space available.]:space' \
    ":algorithms:(($algorithms))"
  ;;
esac
