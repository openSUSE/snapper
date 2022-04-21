
#include <fcntl.h>
#include <iostream>
#include <vector>

#include <snapper/AsciiFile.h>

using namespace std;
using namespace snapper;


int
main()
{
    vector<string> lines;

    /***** AsciiFileReader *****/

    try
    {
	Compression compression = Compression::GZIP;

	string name = add_extension(compression, "big.txt");

#if 1

	int fd = open(name.c_str(), O_RDONLY | O_CLOEXEC | O_LARGEFILE);
	if (fd < 0)
	    SN_THROW(Exception("open '" + name + "' for reading failed"));

	AsciiFileReader tmp(fd, compression);

#elif 1

	FILE* file = fopen(name.c_str(), "re");
	if (!file)
	    SN_THROW(Exception("fopen '" + name + "' for reading failed"));

	AsciiFileReader tmp(file, compression);

#else

	AsciiFileReader tmp(name, compression);

#endif

	string line;

	while (tmp.read_line(line))
	    lines.push_back(line);

#if 1
	tmp.close();
#endif
    }
    catch (const Exception& e)
    {
	SN_CAUGHT(e);

	cerr << e.what() << '\n';
    }

#if 0

    for (const string& line : lines)
	cout << line << '\n';

#endif

    /***** AsciiFileWriter *****/

    try
    {
	Compression compression = Compression::GZIP;

	string name = add_extension(compression, "tmp.txt");

#if 1

	int fd = open(name.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_LARGEFILE, 0666);
	if (fd < 0)
	    SN_THROW(Exception("open '" + name + "' for writing failed"));

	AsciiFileWriter tmp(fd, compression);

#elif 1

	FILE* file = fopen(name.c_str(), "we");
	if (!file)
	    SN_THROW(Exception("fopen '" + name + "' for writing failed"));

	AsciiFileWriter tmp(file, compression);

#else

	AsciiFileWriter tmp(name, compression);

#endif

	for (const string& line : lines)
	    tmp.write_line(line);

#if 1
	tmp.close();
#endif
    }
    catch (const Exception& e)
    {
	SN_CAUGHT(e);

	cerr << e.what() << '\n';
    }
}
