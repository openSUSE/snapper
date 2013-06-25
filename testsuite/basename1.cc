
#include "common.h"

#include <snapper/AppUtil.h>

using namespace snapper;


int
main()
{
    check_equal(basename("/hello/world"), string("world"));
    check_equal(basename("hello/world"), string("world"));
    check_equal(basename("/hello"), string("hello"));
    check_equal(basename("hello"), string("hello"));
    check_equal(basename("/"), string(""));
    check_equal(basename(""), string(""));
    check_equal(basename("."), string("."));
    check_equal(basename(".."), string(".."));
    check_equal(basename("../.."), string(".."));
}
