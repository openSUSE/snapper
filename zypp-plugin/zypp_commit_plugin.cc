#include "zypp_commit_plugin.h"

ZyppPlugin::Message ZyppCommitPlugin::dispatch(const Message& msg) {
    if (msg.command == "PLUGINBEGIN") {
	return plugin_begin(msg);
    }
    if (msg.command == "PLUGINEND") {
	return plugin_end(msg);
    }
    if (msg.command == "COMMITBEGIN") {
	return commit_begin(msg);
    }
    if (msg.command == "COMMITEND") {
	return commit_end(msg);
    }

    return ZyppPlugin::dispatch(msg);
}