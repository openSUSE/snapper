/*
 * Copyright (c) [2015] Red Hat, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef SNAPPER_FILES_TRANSFER_H
#define SNAPPER_FILES_TRANSFER_H

#include <vector>

#include "pipe/AsyncWriteStream.h"
#include "RefCounter.h"
#include "RefComparison.h"

using namespace pipe_stream;
using namespace snapper;


class FilesTransferTask : public RefHolder
{
public:

    FilesTransferTask(XComparison& xcmp);

    FileDescriptor& get_read_end() { return _pipe.read_end(); }

    void init();
    void start() { ws.run(); }

private:

    struct addressor {
	const File* operator()(const File & f) const { return &f; }
    };

    void append_next();

    XComparison& xcmp;
    Files::const_iterator cit;

    Pipe _pipe;
    AsyncWriteStream<vector<const File *>> ws;
};

#endif
