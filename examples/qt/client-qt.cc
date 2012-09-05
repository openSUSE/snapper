
// g++ client-qt.cc -o client-qt -Wall -O2 -lQtCore -lQtDBus


#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtDBus/QtDBus>


enum SnapshotType { SINGLE, PRE, POST };

struct Snapshot
{
    unsigned int num;
    SnapshotType type;
    unsigned int pre_num;
    QDateTime date;
    QString description;
    QString cleanup;
    QMap<QString, QString> userdata;
};


Q_DECLARE_METATYPE(Snapshot)


QDBusArgument& operator<<(QDBusArgument& argument, const Snapshot& mystruct)
{
    argument.beginStructure();
    argument << mystruct.num << static_cast<unsigned short>(mystruct.type) << mystruct.pre_num
	     << mystruct.date.toTime_t() << mystruct.description << mystruct.cleanup
	     << mystruct.userdata;
    argument.endStructure();
    return argument;
}


const QDBusArgument& operator>>(const QDBusArgument& argument, Snapshot& mystruct)
{
    unsigned short tmp1;
    unsigned long long tmp2;

    argument.beginStructure();
    argument >> mystruct.num >> tmp1 >> mystruct.pre_num >> tmp2 >> mystruct.description
	     >> mystruct.cleanup >> mystruct.userdata;
    argument.endStructure();

    mystruct.type = static_cast<SnapshotType>(tmp1);
    mystruct.date.setTime_t(tmp2);

    return argument;
}


void
command_list_snapshots()
{
    QDBusInterface dbus_iface("org.opensuse.Snapper", "/org/opensuse/Snapper",
			      "org.opensuse.Snapper", QDBusConnection::systemBus());

    QDBusMessage reply = dbus_iface.call("ListSnapshots", "root");
    // qDebug() << reply;
    // qDebug() << (reply.type() == QDBusMessage::ReplyMessage);

    QList<QVariant> args = reply.arguments();
    // qDebug() << args.size();

    QDBusArgument arg = args.at(0).value<QDBusArgument>();
    // qDebug() << arg.currentSignature();

    QList<Snapshot> snapshots;
    arg >> snapshots;

    QListIterator<Snapshot> it(snapshots);
    while (it.hasNext())
    {
	const Snapshot& snapshot = it.next();
	printf("%d %d %d", snapshot.num, snapshot.type, snapshot.pre_num);
	if (snapshot.date.toTime_t() == (uint)(-1))
	    printf(" %s", "now");
	else
	    printf(" %s", qPrintable(snapshot.date.toString()));
	printf(" %s", qPrintable(snapshot.description));
	QMapIterator<QString, QString> it2(snapshot.userdata);
	while (it2.hasNext())
	{
	    it2.next();
	    printf(" %s=%s", qPrintable(it2.key()), qPrintable(it2.value()));
	}
	printf("\n");
    }
}


int
main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    qDBusRegisterMetaType<Snapshot>();

    if (!QDBusConnection::systemBus().isConnected())
    {
	fprintf(stderr, "Cannot connect to the D-Bus system bus.\n");
	return 1;
    }

    command_list_snapshots();

    return 0;
}
