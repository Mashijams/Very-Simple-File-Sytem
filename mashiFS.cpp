// Main FS Implementation

#include "MashiFS.h"


FileSystem::FileSystem()
{
	buffer = new char[BLOCK_SIZE];
}


FileSystem::~FileSystem()
{
	delete buffer;
}


status_t
FileSystem::Init(Disk& disk, size_t TotalBlocks)
{
	this->disk = disk;

	/*
		Zero block of disk contains
		Superblock
		Inode Bitmap
		Data Block Bitmap header
	*/
	status_t status = _CreateSuperBlock(TotalBlocks);
	if (status != F_SUCCESS)
		return status;

	// Create Inode BitMap
	InodeMap map;
	for (int i = 0; i < 124; i++) {
		map.Map[i] = 0;
	}

	// Root dir Inode is initialized
	map.Map[0] = 1;

	// Create BitMapHeader
	BitMapHeader header;
	header.Magic = BITMAP_MAGIC;
	header.TotalBlocks = ceil((double)TotalBlocks / 1024);

	// write first block structures to disk now
	char* data = new char[BLOCK_SIZE];
	size_t offset = 0;

	char* ptr;
	ptr = (char*)&sb;
	memcpy(data + offset, ptr, sizeof(SuperBlock));
	offset += sizeof(SuperBlock);

	ptr = (char*)&map;
	memcpy(data + offset, ptr, sizeof(InodeMap));
	offset += sizeof(InodeMap);

	ptr = (char*)&header;
	memcpy(data + offset, ptr, sizeof(BitMapHeader));
	offset += sizeof(BitMapHeader);

	// Write to disk at block no zero
	status = disk.Write(data, 0);
	if (status != F_SUCCESS)
		return status;

	/*
		First block for file system is reserved
		for Inodes.
		It will get filled accordingly in Create Inode
		function.
	*/


	/*
		From second onwards we have data block bitmap
		Its header is in zeroth block
	*/
	for (int i = 2; i < 2 + header.TotalBlocks; i++) {
		uint8_t	direct[POINTERS_PER_BLOCK];
		for (int j  = 0; j < POINTERS_PER_BLOCK; j++) {
			direct[j] = 0;
		}
		ptr = (char*)direct;
		memcpy(data, ptr, BLOCK_SIZE);

		// Write to disk at block no i
		status = disk.Write(data, i);
		if (status != F_SUCCESS)
			return status;
	}

	// Free Data memory as its no longer needed
	delete data;

	// We had succesfully initialised FS on disk
	return F_SUCCESS;
}


status_t
FileSystem::_CreateSuperBlock(size_t TotalBlocks)
{
	sb.Magic			= SB_MAGIC;
	sb.TotalBlocks		= TotalBlocks;
	sb.Root				= 0;
	sb.TotalFreeBlocks	= TotalBlocks - 2 - ceil((double)TotalBlocks / 1024);

	return F_SUCCESS;
}


status_t
File::Mount(Disk& disk)
{
	this->disk = disk;

	status_t status = disk.Read(buffer, 0);
	if (status != F_SUCCESS)
		return status;

	// Initialise and check superblock
	sb = *((SuperBlock*)buffer);

	if (sb.Magic != SB_MAGIC) {
		fprintf(stderr, "Wrong magic number for superblock\n\n");
		return F_FAIL;
	}

	// We successfully mounted FS
	return F_SUCCESS;
}