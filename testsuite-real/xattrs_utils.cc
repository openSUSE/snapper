#include <string.h>
#include <sys/xattr.h>

#include <snapper/XAttributes.h>
#include "xattrs_utils.h"
#include "common.h"

void xattr_create(const char *name, const char *value, const char *path)
{
    check_zero(setxattr(path, name, (void *) value, strlen(value), XATTR_CREATE));
}

void xattr_remove(const char *name, const char *path)
{
    check_zero(removexattr(path, name));
}

void xattr_replace(const char *name, const char *value, const char *path)
{
    check_zero(setxattr(path, name, (void *) value, strlen(value), XATTR_REPLACE));
}