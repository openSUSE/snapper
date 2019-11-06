---
title: Machine-readable Output Formats
author: José Iván López González
layout: post
piwik: true
---

Third party programs and scripts (e.g., YaST) parse the output of the *snapper* cli commands to get some information. For example, to find out the number of all pre and post snapshots, scripts can run the following:

~~~
snapper list | awk '/pre/||/post/{print $3}'
~~~

Output from *snapper list* command is a table intended to be read by humans but not scripts. This could make difficult to parse snapper outputs. But even worse, every change in this table output (e.g., by adding a new column or changing their values format) could ruin scripts that relied on the previous table version.

~~~
snapper --iso list --disable-used-space
 # | Type   | Pre # | Date                | User | Cleanup | Description           | Userdata
---+--------+-------+---------------------+------+---------+-----------------------+--------------
0  | single |       |                     | root |         | current               |
1* | single |       | 2019-10-15 12:45:25 | root |         | first root filesystem |
2  | single |       | 2019-10-15 12:51:06 | root | number  | after installation    | important=yes
3  | pre    |       | 2019-10-15 13:06:08 | root | number  | zypp(zypper)          | important=no
4  | post   |     3 | 2019-10-15 13:07:41 | root | number  |                       | important=no
~~~

All that could make quite error prone to work with *snapper* cli output, and for that reason, *snapper* now offers some new options to generate listing outputs with CSV or JSON format.

Now, the following new global options can be used with *snapper* cli: *--machine-readable*, *--csvout*, *--jsonout* and *--separator*. Option *--machine-readable* requires an argument, and accepted values are *csv* and *json*. *--csvout* and *--jsonout* are only shortcuts for *--machine-readable csv* and *--machine-readable json* respectively. In case that *--machine-readable csv* or *--csvout* is used, the *--separator* option can be indicated to set another CSV char separator.

These new options only affect to the cli commands that generate a table as result, that is: *snapper list*, *snapper list-configs* and *snapper get-config*. Here some usage examples:

~~~
snapper --csvout --separator \; list
config;subvolume;number;default;active;type;pre-number;date;user;used-space;cleanup;description;userdata
root;/;0;no;no;single;;;root;;;current;
root;/;1;yes;yes;single;;2019-10-15 12:45:25;root;88047616;;first root filesystem;
root;/;2;no;no;single;;2019-10-15 12:51:06;root;12226560;number;after installation;important=yes
root;/;3;no;no;pre;;2019-10-15 13:06:08;root;540672;number;zypp(zypper);important=no
root;/;4;no;no;post;3;2019-10-15 13:07:41;root;81920;number;;important=no

snapper --jsonout --separator \; list --type single
{
  "root": [
    {
      "subvolume": "/",
      "number": 0,
      "default": false,
      "active": false,
      "date": "",
      "user": "root",
      "used-space": null,
      "description": "current",
      "userdata": null
    },
    {
      "subvolume": "/",
      "number": 1,
      "default": true,
      "active": true,
      "date": "2019-10-15 12:45:25",
      "user": "root",
      "used-space": 88047616,
      "description": "first root filesystem",
      "userdata": null
    },
    {
      "subvolume": "/",
      "number": 2,
      "default": false,
      "active": false,
      "date": "2019-10-15 12:51:06",
      "user": "root",
      "used-space": 12226560,
      "description": "after installation",
      "userdata": {
        "important": "yes"
      }
    }
  ]
}
~~~

Moreover, those three commands (*list*, *list-configs* and *get-config*) also accept a new *--columns* option. That option allows to filter and sort the columns to show by indicating column names separated by comma (possible columns for each command can be found in the *snapper* help):

~~~
snapper --csvout list --columns number,type,pre-number
number,type,pre-number
0,single,
1,single,
2,single,
3,pre,
4,post,3
5,pre,
6,post,5
7,pre,
8,post,7
~~~

Thanks to new options, now scripts can obtain an easy-to-parse and invariant *snapper* output. For example, extracting the number of all pre and post snapshots can be performed by the following script:

~~~
snapper --csvout list --columns number,type | awk 'BEGIN {FS=","} /pre/||/post/ {print $1}'
~~~

This feature is available in *snapper* since version 0.8.6.
