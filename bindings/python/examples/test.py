#!/usr/bin/python

import libsnapper

def print_snap_info( s ):
  if isinstance( s, libsnapper.SwigPyIterator ): 
    try:
      s = s.value()
    except StopIteration:
      print "end iterator"
      return
  print s.getNum(), s.getType(), s.getDescription()

sh = libsnapper.createSnapper("root", False)

# testing getConfigs
cl = libsnapper.Snapper.getConfigs()
print cl.size()
for i in cl: print i.config_name, i.subvolume

# testing getIgnorePatterns
pl = sh.getIgnorePatterns()
print pl.size()
for i in pl: print i

# testing iterators over container
sl = sh.getSnapshots()
for i in sl: print_snap_info( i )
print sl.size()
i = sl.begin()
while i!=sl.end():
  print_snap_info( i )
  i.incr()

# testing find functions
i = sl.find(3)
print_snap_info( i )
i = sl.getSnapshotCurrent()
print_snap_info( i )
i = sl.find(11)
print_snap_info( i )
j = sl.findPre(i)
print_snap_info( j )
i = sl.find(10)
print_snap_info( i )

# testing handling description
j = sl.findPost(i)
print_snap_info( j )
s=j.value().getDescription()
j.value().setDescription(s+" 1")
j = sl.findPost(i)
print_snap_info( j )
s=j.value().getDescription()
j.value().setDescription(s[:-2])

# testing handling userdata
j = sl.findPost(i)
print_snap_info( j )
s=j.value().getUserdata()
print s.items()
s["key1"] = "value1"
s["key2"] = "value2"
s["key3"] = "value3"
j.value().setUserdata(s)
j = sl.findPost(i)
print_snap_info( j )
t=j.value().getUserdata()
print t.items()
t.clear()
j.value().setUserdata(t)
j = sl.findPost(i)
print_snap_info( j )
print j.value().getUserdata().items()

# testing compare functionality
j=sl.find(11);
i=sl.findPre(j);
cmp=libsnapper.Comparison(sh,i,j)
i1=cmp.getSnapshot1()
i2=cmp.getSnapshot2()
print_snap_info( i1 )
print_snap_info( i2 )
flist=cmp.getFiles()
print flist.size()
for f in flist:
    print f.getAbsolutePath(libsnapper.LOC_SYSTEM), f.getAbsolutePath(libsnapper.LOC_PRE), f.getAbsolutePath(libsnapper.LOC_POST)
    sl = f.getDiff("-u")
    for s in sl: print s

# testing set/getUndo
f=flist.begin()
if f != flist.end():
  print f.value().getUndo()
  f.value().setUndo(True)
  print f.value().getUndo()
  f.value().setUndo(False)
  print f.value().getUndo()
  f.value().setUndo(True)
  print f.value().getUndo()

# testing doUndo
if f != flist.end():
  print cmp.doUndo()
