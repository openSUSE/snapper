#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE selected_files

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <sys/stat.h>
#include <unistd.h>

#include <snapper/File.h>

using namespace snapper;
using namespace std;


class TestTree
{
public:

    TestTree()
    {
	char tmp[] = "/tmp/snapper-selected-files-XXXXXX";
	const char* ret = mkdtemp(tmp);
	BOOST_REQUIRE(ret != nullptr);

	root = ret;
	pre = root + "/pre";
	post = root + "/post";
	system = root + "/system";

	filesystem::create_directories(pre);
	filesystem::create_directories(post);
	filesystem::create_directories(system);
    }

    ~TestTree()
    {
	error_code error;
	filesystem::remove_all(root, error);
    }

    void write(const string& tree, const string& name, const string& contents)
    {
	filesystem::path path = tree + name;
	filesystem::create_directories(path.parent_path());

	ofstream stream(path);
	BOOST_REQUIRE(stream.good());
	stream << contents;
	stream.close();
	BOOST_REQUIRE(stream.good());
    }

    string read(const string& tree, const string& name) const
    {
	ifstream stream(tree + name);
	BOOST_REQUIRE(stream.good());
	return string(istreambuf_iterator<char>(stream), istreambuf_iterator<char>());
    }

    FilePaths paths() const
    {
	return { system, pre, post };
    }

    string root;
    string pre;
    string post;
    string system;
};


BOOST_AUTO_TEST_CASE(compare_selected_files)
{
    TestTree tree;

    tree.write(tree.pre, "/same", "same");
    tree.write(tree.post, "/same", "same");
    tree.write(tree.pre, "/modified", "before");
    tree.write(tree.post, "/modified", "after");
    filesystem::last_write_time(tree.post + "/modified",
	filesystem::last_write_time(tree.pre + "/modified") + chrono::seconds(1));
    tree.write(tree.post, "/created", "created");
    tree.write(tree.pre, "/deleted", "deleted");
    tree.write(tree.post, "/new-parent/created", "created");
    tree.write(tree.pre, "/old-parent/deleted", "deleted");
    tree.write(tree.pre, "/type", "file");
    filesystem::create_directory(tree.post + "/type");
    tree.write(tree.pre, "/permissions", "same");
    tree.write(tree.post, "/permissions", "same");
    chmod((tree.pre + "/permissions").c_str(), 0600);
    chmod((tree.post + "/permissions").c_str(), 0644);

    FilePaths paths = tree.paths();
    Files files = compareFiles(&paths, {
	tree.system + "/same",
	tree.system + "/modified",
	tree.system + "/created",
	tree.system + "/deleted",
	tree.system + "/new-parent/created",
	tree.system + "/old-parent/deleted",
	tree.system + "/type",
	tree.system + "/permissions",
	tree.system + "/created"
    }, {});

    BOOST_CHECK_EQUAL(files.size(), static_cast<Files::size_type>(7));
    BOOST_CHECK(files.find("/same") == files.end());
    BOOST_REQUIRE(files.find("/modified") != files.end());
    BOOST_CHECK(files.find("/modified")->getPreToPostStatus() & CONTENT);
    BOOST_REQUIRE(files.find("/created") != files.end());
    BOOST_CHECK_EQUAL(files.find("/created")->getPreToPostStatus(), CREATED);
    BOOST_REQUIRE(files.find("/deleted") != files.end());
    BOOST_CHECK_EQUAL(files.find("/deleted")->getPreToPostStatus(), DELETED);
    BOOST_REQUIRE(files.find("/new-parent/created") != files.end());
    BOOST_CHECK_EQUAL(files.find("/new-parent/created")->getPreToPostStatus(), CREATED);
    BOOST_REQUIRE(files.find("/old-parent/deleted") != files.end());
    BOOST_CHECK_EQUAL(files.find("/old-parent/deleted")->getPreToPostStatus(), DELETED);
    BOOST_REQUIRE(files.find("/type") != files.end());
    BOOST_CHECK(files.find("/type")->getPreToPostStatus() & TYPE);
    BOOST_REQUIRE(files.find("/permissions") != files.end());
    BOOST_CHECK(files.find("/permissions")->getPreToPostStatus() & PERMISSIONS);

    Files ignored = compareFiles(&paths, { tree.system + "/created" }, { "/created" });
    BOOST_CHECK(ignored.empty());
}


BOOST_AUTO_TEST_CASE(reject_paths_outside_subvolume)
{
    TestTree tree;

    tree.write(tree.post, "/created", "created");
    filesystem::create_directory_symlink(tree.post, tree.pre + "/symlink-parent");

    FilePaths paths = tree.paths();
    Files files = compareFiles(&paths, {
	tree.system + "/../post/created",
	tree.system + "-other/created",
	"relative/path",
	tree.system + "//created",
	tree.system + "/./created",
	tree.system + "/symlink-parent/created",
	tree.system + "/",
	tree.system
    }, {});

    BOOST_CHECK(files.empty());
}


BOOST_AUTO_TEST_CASE(undo_selected_files)
{
    TestTree tree;

    tree.write(tree.pre, "/modified", "before");
    tree.write(tree.post, "/modified", "after");
    tree.write(tree.system, "/modified", "after");
    filesystem::last_write_time(tree.post + "/modified",
	filesystem::last_write_time(tree.pre + "/modified") + chrono::seconds(1));
    filesystem::last_write_time(tree.system + "/modified",
	filesystem::last_write_time(tree.post + "/modified"));

    tree.write(tree.post, "/created", "created");
    tree.write(tree.system, "/created", "created");

    tree.write(tree.pre, "/deleted", "deleted");

    FilePaths paths = tree.paths();
    Files files = compareFiles(&paths, {
	tree.system + "/modified",
	tree.system + "/created",
	tree.system + "/deleted"
    }, {});

    for (File& file : files)
	file.setUndo(true);

    vector<UndoStep> steps = files.getUndoSteps();
    BOOST_REQUIRE_EQUAL(steps.size(), static_cast<vector<UndoStep>::size_type>(3));

    for (const UndoStep& step : steps)
	BOOST_REQUIRE(files.doUndoStep(step));

    BOOST_CHECK_EQUAL(tree.read(tree.system, "/modified"), "before");
    BOOST_CHECK(!filesystem::exists(tree.system + "/created"));
    BOOST_CHECK_EQUAL(tree.read(tree.system, "/deleted"), "deleted");
}
