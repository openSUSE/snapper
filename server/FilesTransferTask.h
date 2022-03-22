/*
 * Copyright (c) 2015 Red Hat, Inc.
 * Copyright (c) 2022 SUSE LLC
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
 */


#ifndef SNAPPER_FILES_TRANSFER_TASK_H
#define SNAPPER_FILES_TRANSFER_TASK_H


#include <dbus/DBusPipe.h>

#include <snapper/File.h>


using namespace snapper;


struct StreamException : Exception
{
    explicit StreamException() : Exception("stream exception") {}
};


class FilesTransferTask
{
public:

    FilesTransferTask(const Files& files);

    DBus::FileDescriptor& get_read_end() { return pipe.get_read_end(); }
    DBus::FileDescriptor& get_write_end() { return pipe.get_write_end(); }

    void run();

private:

    // Keep a copy of the files. This way the Comparison can be deleted before the
    // transfer is complete. If that copy is ever a problem it could be turned into some
    // shared object.
    const Files files;

    DBus::Pipe pipe;

};


#endif
