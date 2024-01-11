/*
 * Copyright (c) [2004-2015] Novell, Inc.
 * Copyright (c) [2020-2022] SUSE LLC
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


#include <unistd.h>
#include <fcntl.h>
#include <zlib.h>
#include <regex>
#include <boost/algorithm/string.hpp>

#include "snapper/Log.h"
#include "snapper/AppUtil.h"
#include "snapper/AsciiFile.h"
#include "snapper/Exception.h"


namespace snapper
{
    using namespace std;


    bool
    is_available(Compression compression)
    {
	switch (compression)
	{
	    case Compression::NONE:
		return true;

	    case Compression::GZIP:
		return true;

	    case Compression::ZSTD:
		return false;
	}

	return false;
    }


    string
    add_extension(Compression compression, const string& name)
    {
	switch (compression)
	{
	    case Compression::NONE:
		return name;

	    case Compression::GZIP:
		return name + ".gz";

	    case Compression::ZSTD:
		return name + ".zst";
	}

	SN_THROW(LogicErrorException("unknown or unsupported compression"));
	__builtin_unreachable();
    }


    class AsciiFileReader::Impl
    {
    public:

	class None;
	class Gzip;
	class Zstd;

	template <typename T>
	static std::unique_ptr<AsciiFileReader::Impl> factory(T t, Compression compression);

	virtual ~Impl() = default;

	virtual bool read_line(string& line) = 0;

	virtual void close() = 0;

    };


    class AsciiFileReader::Impl::None : public AsciiFileReader::Impl
    {
    public:

	None(int fd);
	None(FILE* fin);
	None(const string& name);

	virtual ~None();

	virtual bool read_line(string& line) override;

	virtual void close() override;

    private:

	FILE* fin = nullptr;

	char* buffer = nullptr;
	size_t len = 0;

    };


    AsciiFileReader::Impl::None::None(int fd)
    {
	fin = fdopen(fd, "r");
	if (!fin)
	    SN_THROW(IOErrorException(sformat("fdopen failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));
    }


    AsciiFileReader::Impl::None::None(FILE* fin)
	: fin(fin)
    {
    }


    AsciiFileReader::Impl::None::None(const string& name)
    {
	fin = fopen(name.c_str(), "re");
	if (!fin)
	    SN_THROW(IOErrorException(sformat("fopen '%s' for reading failed, errno:%d (%s)",
					      name.c_str(), errno, stringerror(errno).c_str())));
    }


    AsciiFileReader::Impl::None::~None()
    {
	free(buffer);

	try
	{
	    close();
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);

	    y2err("exception ignored");
	}
    }


    bool
    AsciiFileReader::Impl::None::read_line(string& line)
    {
	ssize_t n = getline(&buffer, &len, fin);
	if (n == -1)
	    return false;

	if (n > 0 && buffer[n - 1] == '\n')
	    n--;

	line = string(buffer, 0, n);

	return true;
    }


    void
    AsciiFileReader::Impl::None::close()
    {
	if (!fin)
	    return;

	FILE* tmp = fin;
	fin = nullptr;

	if (fclose(tmp) != 0)
	    SN_THROW(IOErrorException(sformat("fclose failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));
    }


    class AsciiFileReader::Impl::Gzip : public AsciiFileReader::Impl
    {
    public:

	Gzip(int fd);
	Gzip(FILE* fin);
	Gzip(const string& name);

	virtual ~Gzip();

	virtual bool read_line(string& line) override;

	virtual void close() override;

    private:

	Gzip();

	gzFile gz_file = nullptr;

	vector<char> buffer;
	size_t buffer_read = 0;		// position to which the buffer has been read
	size_t buffer_fill = 0;		// position to which the buffer is filled

	bool read_buffer();

    };


    AsciiFileReader::Impl::Gzip::Gzip()
    {
	buffer.resize(16 * 1024);
    }


    AsciiFileReader::Impl::Gzip::Gzip(int fd)
	: Gzip()
    {
	gz_file = gzdopen(fd, "r");
	if (!gz_file)
	    SN_THROW(IOErrorException(sformat("gzdopen failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));
    }


    AsciiFileReader::Impl::Gzip::Gzip(FILE* fin)
	: Gzip()
    {
	int fd = fileno(fin);
	if (fd < 0)
	    SN_THROW(IOErrorException(sformat("fileno failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));

	fd = dup(fd);
	if (fd < 0)
	    SN_THROW(IOErrorException(sformat("dup failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));

	gz_file = gzdopen(fd, "r");
	if (!gz_file)
	    SN_THROW(IOErrorException(sformat("gzdopen failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));

	fclose(fin);
    }


    AsciiFileReader::Impl::Gzip::Gzip(const string& name)
	: Gzip()
    {
	int fd = open(name.c_str(), O_RDONLY | O_CLOEXEC | O_LARGEFILE);
	if (fd < 0)
	    SN_THROW(IOErrorException(sformat("open '%s' for reading failed, errno:%d (%s)",
					      name.c_str(), errno, stringerror(errno).c_str())));

	gz_file = gzdopen(fd, "r");
	if (!gz_file)
	    SN_THROW(IOErrorException(sformat("gzdopen failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));
    }


    AsciiFileReader::Impl::Gzip::~Gzip()
    {
	try
	{
	    close();
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);

	    y2err("exception ignored");
	}
    }


    void
    AsciiFileReader::Impl::Gzip::close()
    {
	if (!gz_file)
	    return;

	gzFile tmp = gz_file;
	gz_file = nullptr;

	int r = gzclose(tmp);
	if (r != Z_OK)
	    SN_THROW(IOErrorException(sformat("gzclose failed, errnum:%d", r)));
    }


    bool
    AsciiFileReader::Impl::Gzip::read_buffer()
    {
	int r = gzread(gz_file, buffer.data(), buffer.size());
	if (r <= 0)
	{
	    if (gzeof(gz_file))
		return false;

	    int errnum = 0;
	    const char* msg = gzerror(gz_file, &errnum);
	    SN_THROW(IOErrorException(sformat("gzread failed, errnum:%d (%s)", errnum, msg)));
	}

	buffer_read = 0;
	buffer_fill = r;

	return true;
    }


    bool
    AsciiFileReader::Impl::Gzip::read_line(string& line)
    {
	line.clear();

	while (true)
	{
	    // check if all of the output buffer has been used
	    if (buffer_read == buffer_fill)
	    {
		if (!read_buffer())
		    return !line.empty();
	    }

	    const char* p1 = buffer.data() + buffer_read;
	    size_t remaining = buffer_fill - buffer_read;

	    const char* p2 = (const char*) memchr(p1, '\n', remaining);

	    if (p2)
	    {
		line += string(p1, p2 - p1);
		buffer_read = p2 - buffer.data() + 1;
		return true;
	    }

	    line += string(p1, remaining);
	    buffer_read += remaining;
	}
    }


    template <typename T>
    std::unique_ptr<AsciiFileReader::Impl>
    AsciiFileReader::Impl::factory(T t, Compression compression)
    {
	switch (compression)
	{
	    case Compression::NONE:
		return unique_ptr<Impl::None>(new Impl::None(t));

	    case Compression::GZIP:
		return unique_ptr<Impl::Gzip>(new Impl::Gzip(t));

	    case Compression::ZSTD:
		break;
	}

	SN_THROW(LogicErrorException("unknown or unsupported compression"));
	__builtin_unreachable();
    }


    AsciiFileReader::AsciiFileReader(int fd, Compression compression)
	: impl(AsciiFileReader::Impl::factory(fd, compression))
    {
    }


    AsciiFileReader::AsciiFileReader(FILE* fin, Compression compression)
	: impl(AsciiFileReader::Impl::factory(fin, compression))
    {
    }


    AsciiFileReader::AsciiFileReader(const string& name, Compression compression)
	: impl(AsciiFileReader::Impl::factory(name, compression))
    {
    }


    AsciiFileReader::~AsciiFileReader() = default;


    bool
    AsciiFileReader::read_line(string& line)
    {
	return impl->read_line(line);
    }


    void
    AsciiFileReader::close()
    {
	impl->close();
    }


    class AsciiFileWriter::Impl
    {
    public:

	class None;
	class Gzip;
	class Zstd;

	template <typename T>
	static std::unique_ptr<AsciiFileWriter::Impl> factory(T t, Compression compression);

	virtual ~Impl() = default;

	virtual void write_line(const string& line) = 0;

	virtual void close() = 0;

    };


    class AsciiFileWriter::Impl::None : public AsciiFileWriter::Impl
    {
    public:

	None(int fd);
	None(FILE* fout);
	None(const string& name);

	virtual ~None();

	virtual void write_line(const string& line) override;

	virtual void close() override;

    private:

	FILE* fout = nullptr;

    };


    AsciiFileWriter::Impl::None::None(int fd)
    {
	fout = fdopen(fd, "w");
	if (!fout)
	    SN_THROW(IOErrorException(sformat("fdopen failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));
    }


    AsciiFileWriter::Impl::None::None(FILE* fout)
	: fout(fout)
    {
    }


    AsciiFileWriter::Impl::None::None(const string& name)
    {
	fout = fopen(name.c_str(), "we");
	if (!fout)
	    SN_THROW(IOErrorException(sformat("fopen '%s' for writing failed, errno:%d (%s)",
					      name.c_str(), errno, stringerror(errno).c_str())));
    }


    AsciiFileWriter::Impl::None::~None()
    {
	try
	{
	    close();
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);

	    y2err("exception ignored");
	}
    }


    void
    AsciiFileWriter::Impl::None::write_line(const string& line)
    {
	if (fprintf(fout, "%s\n", line.c_str()) != (int)(line.size() + 1))
	    SN_THROW(IOErrorException(sformat("fprintf failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));
    }


    void
    AsciiFileWriter::Impl::None::close()
    {
	if (!fout)
	    return;

	FILE* tmp = fout;
	fout = nullptr;

	if (fclose(tmp) != 0)
	    SN_THROW(IOErrorException(sformat("fclose failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));
    }


    class AsciiFileWriter::Impl::Gzip : public AsciiFileWriter::Impl
    {
    public:

	Gzip(int fd);
	Gzip(FILE* fout);
	Gzip(const string& name);

	virtual ~Gzip();

	virtual void write_line(const string& line) override;

	virtual void close() override;

    private:

	Gzip();

	gzFile gz_file = nullptr;

	vector<char> buffer;
	size_t buffer_fill = 0;		// position to which the buffer is filled

	void write_buffer();

    };


    AsciiFileWriter::Impl::Gzip::Gzip()
    {
	buffer.resize(16 * 1024);
    }


    AsciiFileWriter::Impl::Gzip::Gzip(int fd)
	: Gzip()
    {
	gz_file = gzdopen(fd, "w");
	if (!gz_file)
	    SN_THROW(IOErrorException(sformat("gzdopen failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));
    }


    AsciiFileWriter::Impl::Gzip::Gzip(FILE* fout)
	: Gzip()
    {
	int fd = fileno(fout);
	if (fd < 0)
	    SN_THROW(IOErrorException(sformat("fileno failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));

	fd = dup(fd);
	if (fd < 0)
	    SN_THROW(IOErrorException(sformat("dup failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));

	gz_file = gzdopen(fd, "w");
	if (!gz_file)
	    SN_THROW(IOErrorException(sformat("gzdopen failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));

	fclose(fout);
    }


    AsciiFileWriter::Impl::Gzip::Gzip(const string& name)
	: Gzip()
    {
	int fd = open(name.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_LARGEFILE, 0666);
	if (fd < 0)
	    SN_THROW(IOErrorException(sformat("open '%s' for writing failed, errno:%d (%s)",
					      name.c_str(), errno, stringerror(errno).c_str())));

	gz_file = gzdopen(fd, "w");
	if (!gz_file)
	    SN_THROW(IOErrorException(sformat("gzdopen failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));
    }


    AsciiFileWriter::Impl::Gzip::~Gzip()
    {
	try
	{
	    close();
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);

	    y2err("exception ignored");
	}
    }


    void
    AsciiFileWriter::Impl::Gzip::close()
    {
	if (!gz_file)
	    return;

	write_buffer();

	gzFile tmp = gz_file;
	gz_file = nullptr;

	int r = gzclose(tmp);
	if (r != Z_OK)
	    SN_THROW(IOErrorException(sformat("gzclose failed, errnum:%d", r)));
    }


    void
    AsciiFileWriter::Impl::Gzip::write_buffer()
    {
	if (buffer_fill == 0)
	    return;

	int r = gzwrite(gz_file, buffer.data(), buffer_fill);
	if (r < (int)(buffer_fill))
	{
	    int errnum = 0;
	    const char* msg = gzerror(gz_file, &errnum);
	    SN_THROW(IOErrorException(sformat("gzwrite failed, errnum:%d (%s)", errnum, msg)));
	}

	buffer_fill = 0;
    }


    void
    AsciiFileWriter::Impl::Gzip::write_line(const string& line)
    {
	string tmp = line + "\n";

	while (!tmp.empty())
	{
	    // still available in buffer
	    size_t avail = buffer.size() - buffer_fill;

	    // how much to copy into buffer
	    size_t to_copy = min(avail, tmp.size());

	    // copy into buffer and erase in tmp
	    memcpy(buffer.data() + buffer_fill, tmp.data(), to_copy);
	    buffer_fill += to_copy;
	    tmp.erase(0, to_copy);

	    // if buffer is full, compress it and write to disk
	    if (buffer_fill == buffer.size())
		write_buffer();
	}
    }


#if 0

    // Needs libzstd >= 1.4. So unfortunately currently not suitable.

    class AsciiFileWriter::Impl::Zstd : public AsciiFileWriter::Impl
    {
    public:

	Zstd(int fd);
	Zstd(FILE* fout);
	Zstd(const string& name);

	virtual ~Zstd();

	virtual void write_line(const string& line) override;

	virtual void close() override;

    private:

	Zstd();

	FILE* fout = nullptr;

	ZSTD_CCtx* cctx = nullptr;

	vector<char> in_buffer;
	size_t in_buffer_fill = 0;		// position to which in_buffer is filled

	vector<char> out_buffer;

	void write_buffer(bool last);

    };


    AsciiFileWriter::Impl::Zstd::Zstd()
    {
	cctx = ZSTD_createCCtx();
	if (!cctx)
	    SN_THROW(Exception("ZSTD_createCCtx failed"));

	size_t r1 = ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, 8);
	if (ZSTD_isError(r1))
	    SN_THROW(Exception("ZSTD_CCtx_setParameter with ZSTD_c_compressionLevel failed"));

	size_t r2 = ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 1);
	if (ZSTD_isError(r2))
	    SN_THROW(Exception("ZSTD_CCtx_setParameter with ZSTD_c_checksumFlag failed"));

	in_buffer.resize(ZSTD_CStreamInSize());
	out_buffer.resize(ZSTD_CStreamOutSize());
    }


    AsciiFileWriter::Impl::Zstd::Zstd(int fd)
	: Zstd()
    {
	fout = fdopen(fd, "w");
	if (!fout)
	    SN_THROW(IOErrorException(sformat("fdopen failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));
    }


    AsciiFileWriter::Impl::Zstd::Zstd(FILE* fout)
	: Zstd()
    {
	Zstd::fout = fout;
    }


    AsciiFileWriter::Impl::Zstd::Zstd(const string& name)
	: Zstd()
    {
	fout = fopen(name.c_str(), "we");
	if (!fout)
	    SN_THROW(IOErrorException(sformat("fopen '%s' for writing failed, errno:%d (%s)",
					      name.c_str(), errno, stringerror(errno).c_str())));
    }


    AsciiFileWriter::Impl::Zstd::~Zstd()
    {
	try
	{
	    close();
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);

	    y2err("exception ignored");
	}

	ZSTD_freeCCtx(cctx);
    }


    void
    AsciiFileWriter::Impl::Zstd::close()
    {
	if (!fout)
	    return;

	write_buffer(true);

	FILE* tmp = fout;
	fout = nullptr;

	if (fclose(tmp) != 0)
	    SN_THROW(IOErrorException(sformat("fclose failed, errno:%d (%s)", errno,
					      stringerror(errno).c_str())));
    }


    void
    AsciiFileWriter::Impl::Zstd::write_buffer(bool last)
    {
	ZSTD_EndDirective mode = last ? ZSTD_e_end : ZSTD_e_continue;

	ZSTD_inBuffer input = { in_buffer.data(), in_buffer_fill, 0 };

	while (true)
        {
	    ZSTD_outBuffer output = { out_buffer.data(), out_buffer.size(), 0 };

	    size_t remaining = ZSTD_compressStream2(cctx, &output, &input, mode);
	    if (ZSTD_isError(remaining))
		SN_THROW(Exception("ZSTD_compressStream2 failed"));

	    size_t written = fwrite(output.dst, 1, output.pos, fout);
	    if (written != output.pos)
		SN_THROW(IOErrorException(""));

	    if (last ? (remaining == 0) : (input.pos == input.size))
                break;
	}

	in_buffer_fill = 0;
    }


    void
    AsciiFileWriter::Impl::Zstd::write_line(const string& line)
    {
	string tmp = line + "\n";

	while (!tmp.empty())
	{
	    // still available in input buffer
	    size_t in_avail = in_buffer.size() - in_buffer_fill;

	    // how much to copy into input buffer
	    size_t to_copy = min(in_avail, tmp.size());

	    // copy into input buffer and erase in tmp
	    memcpy(in_buffer.data() + in_buffer_fill, tmp.data(), to_copy);
	    in_buffer_fill += to_copy;
	    tmp.erase(0, to_copy);

	    // if input buffer is full, compress it and write to disk
	    if (in_buffer_fill == in_buffer.size())
		write_buffer(false);
	}
    }

#endif


    template <typename T>
    std::unique_ptr<AsciiFileWriter::Impl>
    AsciiFileWriter::Impl::factory(T t, Compression compression)
    {
	switch (compression)
	{
	    case Compression::NONE:
		return unique_ptr<Impl::None>(new Impl::None(t));

	    case Compression::GZIP:
		return unique_ptr<Impl::Gzip>(new Impl::Gzip(t));

	    case Compression::ZSTD:
		break;
	}

	SN_THROW(LogicErrorException("unknown or unsupported compression"));
	__builtin_unreachable();
    }


    AsciiFileWriter::AsciiFileWriter(int fd, Compression compression)
	: impl(AsciiFileWriter::Impl::factory(fd, compression))
    {
    }


    AsciiFileWriter::AsciiFileWriter(FILE* fout, Compression compression)
	: impl(AsciiFileWriter::Impl::factory(fout, compression))
    {
    }


    AsciiFileWriter::AsciiFileWriter(const string& name, Compression compression)
	: impl(AsciiFileWriter::Impl::factory(name, compression))
    {
    }


    AsciiFileWriter::~AsciiFileWriter()
    {
    }


    void
    AsciiFileWriter::write_line(const string& line)
    {
	impl->write_line(line);
    }


    void
    AsciiFileWriter::close()
    {
	impl->close();
    }


    AsciiFile::AsciiFile(const string& name, bool remove_empty)
	: name(name), remove_empty(remove_empty)
    {
	reload();
    }


    AsciiFile::~AsciiFile()
    {
    }


    void
    AsciiFile::reload()
    {
	y2mil("loading file " << name);

	clear();

	AsciiFileReader ascii_file_reader(name, Compression::NONE);

	string line;
	while (ascii_file_reader.read_line(line))
	    lines.push_back(line);

	ascii_file_reader.close();
    }


    void
    AsciiFile::save()
    {
	if (remove_empty && empty())
	{
	    y2mil("removing file " << name);

	    if (access(name.c_str(), F_OK) != 0)
		return;

	    if (unlink(name.c_str()) != 0)
		SN_THROW(IOErrorException(sformat("unlink failed, errno:%d (%s)", errno,
						  stringerror(errno).c_str())));
	}
	else
	{
	    y2mil("saving file " << name);

	    AsciiFileWriter ascii_file_writer(name, Compression::NONE);

	    for (const string& line : lines)
		ascii_file_writer.write_line(line);

	    ascii_file_writer.close();
	}
    }


    void
    AsciiFile::log_content() const
    {
	y2mil("content of " << (name.empty() ? "<nameless>" : name));

	for (const string& line : lines)
	    y2mil(line);
    }


    SysconfigFile::SysconfigFile(const string& name)
	: AsciiFile(name)
    {
    }


    SysconfigFile::~SysconfigFile()
    {
	if (!modified)
	    return;

	try
	{
	    save();
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);

	    y2err("exception ignored");
	}
    }


    void
    SysconfigFile::save()
    {
	if (!modified)
	    return;

	AsciiFile::save();
	modified = false;
    }


    void
    SysconfigFile::check_key(const string& key) const
    {
	static const regex rx("([0-9A-Z_]+)", regex::extended);

	if (!regex_match(key, rx))
	    SN_THROW(InvalidKeyException());
    }


    void
    SysconfigFile::set_value(const string& key, bool value)
    {
	set_value(key, value ? "yes" : "no");
    }


    bool
    SysconfigFile::get_value(const string& key, bool& value) const
    {
	string tmp;
	if (!get_value(key, tmp))
	    return false;

	value = tmp == "yes";
	return true;
    }


    void
    SysconfigFile::set_value(const string& key, const char* value)
    {
	set_value(key, string(value));
    }


    void
    SysconfigFile::set_value(const string& key, const string& value)
    {
	check_key(key);

	modified = true;

	for (vector<string>::iterator it = lines.begin(); it != lines.end(); ++it)
	{
	    ParsedLine parsed_line;

	    if (parse_line(*it, parsed_line) && parsed_line.key == key)
	    {
		*it = key + "=\"" + value + "\"" + parsed_line.comment;
		return;
	    }
	}

	string line = key + "=\"" + value + "\"";
	push_back(line);
    }


    bool
    SysconfigFile::get_value(const string& key, string& value) const
    {
	for (const string& line : lines)
	{
	    ParsedLine parsed_line;

	    if (parse_line(line, parsed_line) && parsed_line.key == key)
	    {
		value = parsed_line.value;
		y2mil("key:" << key << " value:" << value);
		return true;
	    }
	}

	return false;
    }


    void
    SysconfigFile::set_value(const string& key, const vector<string>& values)
    {
	string tmp;
	for (vector<string>::const_iterator it = values.begin(); it != values.end(); ++it)
	{
	    if (it != values.begin())
		tmp.append(" ");
	    tmp.append(boost::replace_all_copy(*it, " ", "\\ "));
	}
	set_value(key, tmp);
    }


    bool
    SysconfigFile::get_value(const string& key, vector<string>& values) const
    {
	string tmp;
	if (!get_value(key, tmp))
	    return false;

	values.clear();

	string buffer;

	for (string::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
	{
	    switch (*it)
	    {
		case ' ':
		    if (!buffer.empty())
			values.push_back(buffer);
		    buffer.clear();
		    continue;

		case '\\':
		    if (++it == tmp.end())
			return false;
		    if (*it != '\\' && *it != ' ')
			return false;
		    break;
	    }

	    buffer.push_back(*it);
	}

	if (!buffer.empty())
	    values.push_back(buffer);

	return true;
    }


    map<string, string>
    SysconfigFile::get_all_values() const
    {
	map<string, string> ret;

	for (const string& line : lines)
	{
	    ParsedLine parsed_line;

	    if (parse_line(line, parsed_line))
		ret[parsed_line.key] = parsed_line.value;
	}

	return ret;
    }


    bool
    SysconfigFile::parse_line(const string& line, ParsedLine& parsed_line) const
    {
	const string whitespace = "[ \t]*";
	const string comment = "(#.*)?";

	// Note: Avoid back references. Whether they work depend on the regex flags. Also
	// the old regcomp/regexec based implementation did not work with them with some
	// libc implementations.

	static const regex rx1(whitespace + "([0-9A-Z_]+)" + '=' + "\"([^\"]*)\"" +
			       '(' + whitespace + comment + ")");

	static const regex rx2(whitespace + "([0-9A-Z_]+)" + '=' + "'([^']*)'" +
			       '(' + whitespace + comment + ")");

	static const regex rx3(whitespace + "([0-9A-Z_]+)" + '=' + "([^ \t]*)" +
			       '(' + whitespace + comment + ")");

	smatch match;

	if (!regex_match(line, match, rx1) && !regex_match(line, match, rx2) && !regex_match(line, match, rx3))
	    return false;

	parsed_line.key = match[1];
	parsed_line.value = match[2];
	parsed_line.comment = match[3];

	return true;
    }

}
