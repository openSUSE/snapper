
// g++ client-qt.cc -o client-qt -Wall -O2 -lQtCore -lQtDBus


#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtDBus/QtDBus>


enum SnapshotType { SINGLE, PRE, POST };

struct Snapshot
{
    unsigned int num;
    SnapshotType type;
    unsigned int date;
    QString description;
    QMap<QString, QString> userdata;
};


Q_DECLARE_METATYPE(Snapshot)


QDBusArgument& operator<<(QDBusArgument& argument, const Snapshot& mystruct)
{
    argument.beginStructure();
    argument << mystruct.num << static_cast<unsigned short>(mystruct.type) << mystruct.date
	     << mystruct.description << mystruct.userdata;
    argument.endStructure();
    return argument;
}


const QDBusArgument& operator>>(const QDBusArgument& argument, Snapshot& mystruct)
{
    unsigned short tmp1;

    argument.beginStructure();
    argument >> mystruct.num >> tmp1 >> mystruct.date >> mystruct.description >> mystruct.userdata;
    argument.endStructure();

    mystruct.type = static_cast<SnapshotType>(tmp1);

    return argument;
}


void
command_list_snapshots()
{
    QDBusInterface dbus_iface("org.opensuse.snapper", "/org/opensuse/snapper",
			      "org.opensuse.snapper", QDBusConnection::systemBus());

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
	printf("%d %d %d %s", snapshot.num, snapshot.type, snapshot.date,
	       qPrintable(snapshot.description));
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
