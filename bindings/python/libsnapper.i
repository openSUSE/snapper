//
// Python interface definition for libsnapper
//

%module libsnapper

%{
using namespace std;

#include <snapper/Factory.h>
#include <snapper/Exception.h>
#include <snapper/Snapshot.h>
#include <snapper/Snapper.h>
#include <snapper/File.h>
#include <snapper/Comparison.h>
%}

using namespace std;

%include "std_string.i"
%include "std_vector.i"
%include "std_list.i"
%include "std_map.i"

%typemap(out) std::string* {
    $result = PyString_FromString($1->c_str());
}

%ignore snapper::Snapshots::begin();
%ignore snapper::Snapshots::begin() const;
%ignore snapper::Snapshots::end();
%ignore snapper::Snapshots::end() const;
%ignore snapper::Snapshots::find(unsigned int);
%ignore snapper::Snapshots::find(unsigned int) const;
%ignore snapper::Snapshots::findPre(snapper::Snapshots::const_iterator);
%ignore snapper::Snapshots::findPre(snapper::Snapshots::const_iterator) const;
%ignore snapper::Snapshots::findPost(snapper::Snapshots::const_iterator);
%ignore snapper::Snapshots::findPost(snapper::Snapshots::const_iterator) const;
%ignore snapper::Snapshots::getSnapshotCurrent() const;
%ignore snapper::Comparison::getSnapshot1() const;
%ignore snapper::Comparison::getSnapshot2() const;

%include "../../snapper/Factory.h"
%include "../../snapper/Exception.h"
%include "../../snapper/Snapshot.h"
%include "../../snapper/Snapper.h"
%include "../../snapper/File.h"
%include "../../snapper/Comparison.h"

using namespace snapper;


%template(vectorstring) std::vector<string>;
%std_nodefconst_type(File);
%template(VectorFile) std::vector<File>;
%template(mapstringstring) std::map<string,string>;
%template(pairstringstring) std::pair<string,string>;
%template(paircstringstring) std::pair<const string,string>;
%std_nodefconst_type(ConfigInfo);
%template(listConfigInfo) std::list<ConfigInfo>;
%std_nodefconst_type(Snapshot);
%template(listSnapshot) std::list<Snapshot, allocator< Snapshot > >;

%extend snapper::Snapshots {
%fragment("SwigPyIterator_T");

%{
template<class It>
class MySwigPyIteratorTmpl : public swig::SwigPyIteratorClosed_T<It>
{
public:
    typedef swig::SwigPyIteratorClosed_T<It> base;
    typedef typename It::value_type value_type;

    MySwigPyIteratorTmpl(It curr, It first, It last, PyObject *seq)
        : swig::SwigPyIteratorClosed_T<It>(curr,first,last,seq), nend(last)
	{}
    PyObject *value() const {
      if (base::current == nend) {
	throw swig::stop_iteration();
      } else {
	value_type* p = const_cast<value_type *>(&(*base::current));
	return swig::from_ptr(p,0);
      }
    return NULL;
    }
protected:
    It nend;
};

typedef MySwigPyIteratorTmpl<snapper::Snapshots::iterator> MySnapshotIt;
typedef MySwigPyIteratorTmpl<snapper::Snapshots::const_iterator> MyCSnapshotIt;
typedef MySwigPyIteratorTmpl<snapper::Files::iterator> MyFileIt;
typedef MySwigPyIteratorTmpl<snapper::Files::const_iterator> MyCFileIt;
%}

swig::SwigPyIterator* __iter__(PyObject **PYTHON_SELF)
    {
    return new MySnapshotIt(self->begin(), self->begin(),
                            self->end(), *PYTHON_SELF);
    }
swig::SwigPyIterator* begin(PyObject **PYTHON_SELF)
    {
    return new MySnapshotIt(self->begin(), self->begin(),
	                    self->end(), *PYTHON_SELF);
    }
swig::SwigPyIterator* end(PyObject **PYTHON_SELF)
    {
    return new MySnapshotIt(self->end(), self->begin(),
	                    self->end(), *PYTHON_SELF);
    }
swig::SwigPyIterator* find(PyObject **PYTHON_SELF,unsigned int num)
    {
    return new MySnapshotIt(self->find(num), self->begin(),
			    self->end(), *PYTHON_SELF);
    }
swig::SwigPyIterator* findPre(PyObject **PYTHON_SELF, PyObject *pobj )
    {
    swig::SwigPyIteratorClosed_T<snapper::Snapshots::iterator> *it = NULL;
    void *argp = 0;
    int ret = SWIG_ConvertPtr(pobj, &argp, SWIGTYPE_p_swig__SwigPyIterator, 0 );
    if( SWIG_IsOK(ret))
	{
	swig::SwigPyIterator *pi = reinterpret_cast<swig::SwigPyIterator*>(argp);
	it = dynamic_cast<swig::SwigPyIteratorClosed_T<snapper::Snapshots::iterator> *>(pi);
	}
    else
	SWIG_exception_fail(SWIG_ArgError(ret), "in method '" "swig::SwigPyIterator* findPre" "', argument not of type swig::SwigPyIterator");

    if( it )
	return new MySnapshotIt(self->findPre(it->get_current()),
				self->begin(), self->end(), *PYTHON_SELF);
    else

	SWIG_exception_fail( SWIG_ArgError(ret),"argument of wrong swig::SwigPyIterator derived class in method '" "swig::SwigPyIterator* findPre" "'" );
fail:
    return NULL;
    }

swig::SwigPyIterator* findPost(PyObject **PYTHON_SELF, PyObject *pobj )
    {
    swig::SwigPyIteratorClosed_T<snapper::Snapshots::iterator> *it = NULL;
    void *argp = 0;
    int ret = SWIG_ConvertPtr(pobj, &argp, SWIGTYPE_p_swig__SwigPyIterator, 0 );
    if( SWIG_IsOK(ret))
	{
	swig::SwigPyIterator *pi = reinterpret_cast<swig::SwigPyIterator*>(argp);
	it = dynamic_cast<swig::SwigPyIteratorClosed_T<snapper::Snapshots::iterator> *>(pi);
	}
    else
	SWIG_exception_fail(SWIG_ArgError(ret), "in method '" "swig::SwigPyIterator* findPre" "', argument not of type swig::SwigPyIterator");

    if( it )
	return new MySnapshotIt(self->findPost(it->get_current()),
				self->begin(), self->end(), *PYTHON_SELF);
    else

	SWIG_exception_fail( SWIG_ArgError(ret),"argument of wrong swig::SwigPyIterator derived class in method '" "swig::SwigPyIterator* findPre" "'" );
fail:
    return NULL;
    }

swig::SwigPyIterator* getSnapshotCurrent(PyObject **PYTHON_SELF)
    {
    return new MySnapshotIt(self->begin(), self->begin(),
			    self->end(), *PYTHON_SELF);
    }
}

%extend snapper::Comparison {

Comparison( const snapper::Snapper* sn, PyObject *pobj1, PyObject *pobj2 )
    {
    swig::SwigPyIteratorClosed_T<snapper::Snapshots::iterator> *it1 = NULL;
    swig::SwigPyIteratorClosed_T<snapper::Snapshots::iterator> *it2 = NULL;
    void *argp = 0;
    int ret = SWIG_ConvertPtr(pobj1, &argp, SWIGTYPE_p_swig__SwigPyIterator, 0 );
    if( SWIG_IsOK(ret))
	{
	swig::SwigPyIterator *pi = reinterpret_cast<swig::SwigPyIterator*>(argp);
	it1 = dynamic_cast<swig::SwigPyIteratorClosed_T<snapper::Snapshots::iterator> *>(pi);
	}
    else
	SWIG_exception_fail(SWIG_ArgError(ret), "in method '" "swig::SwigPyIterator* findPre" "', argument 2 not of type swig::SwigPyIterator");

    ret = SWIG_ConvertPtr(pobj2, &argp, SWIGTYPE_p_swig__SwigPyIterator, 0 );
    if( SWIG_IsOK(ret))
	{
	swig::SwigPyIterator *pi = reinterpret_cast<swig::SwigPyIterator*>(argp);
	it2 = dynamic_cast<swig::SwigPyIteratorClosed_T<snapper::Snapshots::iterator> *>(pi);
	}
    else
	SWIG_exception_fail(SWIG_ArgError(ret), "in method '" "swig::SwigPyIterator* findPre" "', argument 3 not of type swig::SwigPyIterator");

    if( it1 && it2 )
	return new snapper::Comparison( sn, it1->get_current(), it2->get_current() );
    else if( it1 )

	SWIG_exception_fail( SWIG_ArgError(ret),"argument 3 of wrong swig::SwigPyIterator derived class in method '" "swig::SwigPyIterator* findPre" "'" );
    else
	SWIG_exception_fail( SWIG_ArgError(ret),"argument 2 of wrong swig::SwigPyIterator derived class in method '" "swig::SwigPyIterator* findPre" "'" );
fail:
    return NULL;
    }

swig::SwigPyIterator* getSnapshot1(PyObject **PYTHON_SELF)
    {
    snapper::Snapshots::const_iterator c = self->getSnapshot1();
    snapper::Snapshots::const_iterator b = self->getSnapper()->getSnapshots().begin();
    snapper::Snapshots::const_iterator e = self->getSnapper()->getSnapshots().end();

    return new MyCSnapshotIt(c, b, e, *PYTHON_SELF);
    }

swig::SwigPyIterator* getSnapshot2(PyObject **PYTHON_SELF)
    {
    snapper::Snapshots::const_iterator c = self->getSnapshot2();
    snapper::Snapshots::const_iterator b = self->getSnapper()->getSnapshots().begin();
    snapper::Snapshots::const_iterator e = self->getSnapper()->getSnapshots().end();

    return new MyCSnapshotIt(c, b, e, *PYTHON_SELF);
    }
}

%extend snapper::Files {

swig::SwigPyIterator* __iter__(PyObject **PYTHON_SELF)
    {
    return new MyFileIt(self->begin(), self->begin(),
                        self->end(), *PYTHON_SELF);
    }
}

