#include <iostream>
#include <string.h>
#include <sys/xattr.h>

#include "xattrs_utils.h"
#include "common.h"

void xattr_create(const char *name, const char *value, const char *path)
{
    std::cout << "xa create: file:" << path << "," << name << "=" << value << "... ";
    check_zero(lsetxattr(path, name, (void *) value, strlen(value), XATTR_CREATE));
    std::cout << "done" << std::endl;
}

void xattr_remove(const char *name, const char *path)
{
    std::cout << "xa remove: file:" << path << "," << name << "... ";
    check_zero(lremovexattr(path, name));
    std::cout << "done" << std::endl;
}

void xattr_replace(const char *name, const char *value, const char *path)
{
    std::cout << "xa replace: file:" << path << "," << name << "=" << value << "... ";
    check_zero(lsetxattr(path, name, (void *) value, strlen(value), XATTR_REPLACE));
    std::cout << "done" << std::endl;
}
