#ifndef ZYPP_PLUGIN_H
#define ZYPP_PLUGIN_H

#include <map>
#include <string>

class ZyppPlugin {
public:
    // Plugin message aka frame
    // https://doc.opensuse.org/projects/libzypp/SLE12SP2/zypp-plugins.html
    struct Message {
	std::string command;
	std::map<std::string, std::string> headers;
	std::string body;
    };

    virtual int main();
    virtual Message dispatch(const Message&);
    void answer(const Message&);
    virtual ~ZyppPlugin() {}

    Message ack() {
	Message a;
	a.command = "ACK";
	return a;
    }
};

#endif //ZYPP_PLUGIN_H
