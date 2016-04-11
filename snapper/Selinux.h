/*
 * Copyright (c) [2016] Red Hat, Inc.
 *
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may
 * find current contact information at www.novell.com.
 */


#ifndef SNAPPER_SELINUX_H
#define SNAPPER_SELINUX_H

#include <string>
#include <selinux/context.h>
#include <selinux/label.h>
#include <selinux/selinux.h>

#include <boost/noncopyable.hpp>

#include "snapper/Exception.h"


namespace snapper {

    struct SelinuxException : public Exception
    {
	explicit SelinuxException() : Exception("SELinux error") {}
	explicit SelinuxException(const std::string& msg) : Exception(msg) {}
    };

    using std::string;

    const static string selinux_snapperd_data = "snapperd_data";

    int _is_selinux_enabled();

    class SnapperContexts
    {
    public:
	char* subvolume_context() const { return context_str(subvolume_ctx); }
	SnapperContexts();
	~SnapperContexts() { context_free(subvolume_ctx); }
    private:
	context_t subvolume_ctx;
    };

    class DefaultSelinuxFileContext : private boost::noncopyable
    {
    public:
	DefaultSelinuxFileContext(char* context);
	~DefaultSelinuxFileContext();
    };


    class SelinuxLabelHandle : public boost::noncopyable
    {
    public:
	static SelinuxLabelHandle* get_selinux_handle();

	char* selabel_lookup(const string& path, int mode);

	~SelinuxLabelHandle() { selabel_close(handle); }
    private:
	SelinuxLabelHandle();

	struct selabel_handle* handle;
    };

}

#endif
