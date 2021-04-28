---
title: Absolute Sizes for Space and Free Limit
author: Arvin Schnell
layout: post
---

So far the space and free limits (SPACE_LIMIT and FREE_LIMIT config
variables) imposed on snapshots handled by snapper had to be provided
as a fraction of the whole file system size.

On user request it is now also possible to specify absolute sizes:

~~~
# snapper get-config
Key               | Value   
------------------+---------
FREE_LIMIT        | 5.5 GiB 
SPACE_LIMIT       | 15.2 GiB
[...]
~~~

Note that the values must always be provided in the C locale, so with
a period as decimal separator and untranslated unit.

This feature is available in snapper since version 0.9.0.
