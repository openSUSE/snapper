/*
 * Copyright (c) 2019 SUSE LLC
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
 * with this program; if not, contact SUSE LLC.
 *
 * To contact SUSE about this file by physical or electronic mail, you may
 * find current contact information at www.suse.com.
 */

#ifndef ZYPP_COMMIT_PLUGIN_H
#define ZYPP_COMMIT_PLUGIN_H

#include "zypp-plugin.h"

/// Dispatches begin+end of plugin+commit in dedicated methods.
// The default implementations just ack.
class ZyppCommitPlugin : public ZyppPlugin {
public:
    Message dispatch(const Message& msg) override;

    virtual Message plugin_begin(const Message& m) {
	return ack();
    }
    virtual Message plugin_end(const Message& m) {
	return ack();
    }
    virtual Message commit_begin(const Message& m) {
	return ack();
    }
    virtual Message commit_end(const Message& m) {
	return ack();
    }
};

#endif //ZYPP_COMMIT_PLUGIN_H
