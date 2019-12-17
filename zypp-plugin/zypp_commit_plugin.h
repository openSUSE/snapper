#ifndef ZYPP_COMMIT_PLUGIN_H
#define ZYPP_COMMIT_PLUGIN_H

#include "zypp_plugin.h"

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
