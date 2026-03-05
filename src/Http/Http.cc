#include "Http.h"
#include <strings.h>
namespace DYX{

namespace http{

bool CompareIngnoreCase::operator()(const std::string &lhs ,const std::string &rhs){
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}




}


}

