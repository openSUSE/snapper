
#include "common.h"

#include <snapper/AppUtil.h>

using namespace snapper;


int
main()
{
    check_equal(dirname("/hello/world"), string("/hello"));
    check_equal(dirname("hello/world"), string("hello"));
    check_equal(dirname("/hello"), string("/"));
    check_equal(dirname("hello"), string("."));
    check_equal(dirname("/"), string("/"));
    check_equal(dirname(""), string("."));
    check_equal(dirname("."), string("."));
    check_equal(dirname(".."), string("."));
    check_equal(dirname("../.."), string(".."));
}
