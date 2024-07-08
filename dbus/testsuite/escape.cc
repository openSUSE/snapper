
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE dbus_escape

#include <boost/test/unit_test.hpp>

#include <dbus/DBusMessage.h>
#include <dbus/DBusPipe.h>


using namespace DBus;


BOOST_AUTO_TEST_CASE(marshaller_escape)
{
    BOOST_CHECK_EQUAL(Marshaller::escape("\\"), "\\\\");

    BOOST_CHECK_EQUAL(Marshaller::escape("ä"), "\\xc3\\xa4");
    BOOST_CHECK_EQUAL(Marshaller::escape("0ä0"), "0\\xc3\\xa40");

    BOOST_CHECK_EQUAL(Marshaller::escape("\xff"), "\\xff");
}


BOOST_AUTO_TEST_CASE(unmarshaller_unescape)
{
    BOOST_CHECK_EQUAL(Unmarshaller::unescape("\\\\"), "\\");

    BOOST_CHECK_EQUAL(Unmarshaller::unescape("\\xc3\\xa4"), "ä");
    BOOST_CHECK_EQUAL(Unmarshaller::unescape("0\\xc3\\xa40"), "0ä0");

    BOOST_CHECK_EQUAL(Unmarshaller::unescape("\\xff"), "\xff");

    BOOST_CHECK_THROW(Unmarshaller::unescape("\\"), MarshallingException);
    BOOST_CHECK_THROW(Unmarshaller::unescape("\\x"), MarshallingException);
    BOOST_CHECK_THROW(Unmarshaller::unescape("\\x0"), MarshallingException);
    BOOST_CHECK_THROW(Unmarshaller::unescape("\\x0?"), MarshallingException);
}


BOOST_AUTO_TEST_CASE(pipe_escape)
{
    BOOST_CHECK_EQUAL(Pipe::escape("hello world\n"), "hello\\x20world\\x0a");

    BOOST_CHECK_EQUAL(Pipe::escape("ä"), "ä");
}


BOOST_AUTO_TEST_CASE(pipe_unescape)
{
    BOOST_CHECK_EQUAL(Pipe::unescape("hello\\x20world\\x0a"), "hello world\n");

    BOOST_CHECK_EQUAL(Pipe::unescape("ä"), "ä");
}
