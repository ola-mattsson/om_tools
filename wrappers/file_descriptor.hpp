#pragma once

#include "descriptor_base.h"
namespace om_tools {
namespace descriptors {
inline namespace v1_0_0 {

/**
* THe descriptor_base is sufficient to handle the descriptor from open("filename.txt", int);
*
* might make something more of this eventually. read, write, lock files and such perhaps
*/


using file_descriptor = om_tools::descriptor_base<int32_t>;


}



}
// export to om_tools
using descriptors::file_descriptor;
}