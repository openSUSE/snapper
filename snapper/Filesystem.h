/*
 * Copyright (c) 2011 Novell, Inc.
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


#ifndef FILESYSTEM_H
#define FILESYSTEM_H


#include <string>


namespace snapper
{
    using std::string;


    class Snapper;


    class Filesystem
    {
    public:

	Filesystem(const string& subvolume) : subvolume(subvolume) {}
	virtual ~Filesystem() {}

	static Filesystem* create(const string& fstype, const string& subvolume);

	virtual string name() const = 0;

	virtual void addConfig() const = 0;

	virtual string infosDir() const = 0;
	virtual string snapshotDir(unsigned int num) const = 0;

	virtual void createFilesystemSnapshot(unsigned int num) const = 0;
	virtual void deleteFilesystemSnapshot(unsigned int num) const = 0;

	virtual void mountFilesystemSnapshot(unsigned int num) const = 0;
	virtual void umountFilesystemSnapshot(unsigned int num) const = 0;

	virtual bool checkFilesystemSnapshot(unsigned int num) const = 0;

    protected:

	const string subvolume;

    };


    class Btrfs : public Filesystem
    {
    public:

	Btrfs(const string& subvolume) : Filesystem(subvolume) {}

	virtual string name() const { return "btrfs"; }

	virtual void addConfig() const;

	virtual string infosDir() const;
	virtual string snapshotDir(unsigned int num) const;

	virtual void createFilesystemSnapshot(unsigned int num) const;
	virtual void deleteFilesystemSnapshot(unsigned int num) const;

	virtual void mountFilesystemSnapshot(unsigned int num) const;
	virtual void umountFilesystemSnapshot(unsigned int num) const;

	virtual bool checkFilesystemSnapshot(unsigned int num) const;

    };


    class Ext4 : public Filesystem
    {
    public:

	Ext4(const string& subvolume) : Filesystem(subvolume) {}

	virtual string name() const { return "ext4"; }

	virtual void addConfig() const;

	virtual string infosDir() const;
	virtual string snapshotDir(unsigned int num) const;
	virtual string snapshotFile(unsigned int num) const;

	virtual void createFilesystemSnapshot(unsigned int num) const;
	virtual void deleteFilesystemSnapshot(unsigned int num) const;

	virtual void mountFilesystemSnapshot(unsigned int num) const;
	virtual void umountFilesystemSnapshot(unsigned int num) const;

	virtual bool checkFilesystemSnapshot(unsigned int num) const;

    };

}


#endif
