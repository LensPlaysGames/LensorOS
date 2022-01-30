#ifndef LENSOR_OS_VFS_INODE_H
#define LENSOR_OS_VFS_INODE_H

#include "integers.h"

class FileSystem;

struct Inode {
	// TODO: File data.
	FileSystem* fs;
};

#endif
