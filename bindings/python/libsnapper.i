//
// Python interface definition for libsnapper
//

%module libsnapper

%{
using namespace std;

#include <snapper/Factory.h>
#include <snapper/Snapshot.h>
#include <snapper/Snapper.h>
#include <snapper/File.h>
#include <snapper/Comparison.h>
#include <snapper/Exception.h>
%}

using namespace std;

%include "std_string.i"
%include "std_vector.i"
%include "std_list.i"

%typemap(out) std::string* {
    $result = PyString_FromString($1->c_str());
}

%include "../../snapper/Factory.h"
%include "../../snapper/Snapshot.h"
%include "../../snapper/Snapper.h"
%include "../../snapper/File.h"
%include "../../snapper/Comparison.h"
%include "../../snapper/Exception.h"

using namespace snapper;

%template(vectorstring) vector<string>;

