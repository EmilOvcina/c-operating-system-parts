#define FUSE_USE_VERSION 25

#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

FILE* handle;

#include "lfs_dir.h"
#include "lfs_file.h"

/*	lfs methods	*/
int lfs_getattr( const char *, struct stat * );
int lfs_setxattr(const char*, const char*, const char*, size_t, int);
int lfs_readdir( const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info * );
int lfs_open( const char *, struct fuse_file_info * );
int lfs_read( const char *, char *, size_t, off_t, struct fuse_file_info * );
int lfs_release(const char *path, struct fuse_file_info *fi);
int lfs_mkdir(const char*, mode_t);
int lfs_mknod( const char *, mode_t , dev_t);
int lfs_write( const char *, const char *, size_t, off_t, struct fuse_file_info *);
int lfs_unlink(const char *);
int lfs_utime( const char *, struct utimbuf *);
int lfs_truncate(const char*, off_t);
int lfs_rmdir(const char* );

/*	fuse operation struct */
static struct fuse_operations lfs_oper = {
	.getattr	= lfs_getattr,
	.setxattr	= lfs_setxattr,
	.readdir	= lfs_readdir,
	.mknod = lfs_mknod,
	.mkdir = lfs_mkdir,
	.unlink = lfs_unlink,
	.rmdir = lfs_rmdir,
	.truncate = lfs_truncate,
	.open	= lfs_open,
	.read	= lfs_read,
	.release = lfs_release,
	.write = lfs_write,
	.rename = NULL,
	.utime = lfs_utime
};

/* Gets called everytime something is done in the filesystem */
int lfs_getattr( const char *path, struct stat *st) {

	memset(st, 0, sizeof(struct stat));

	if (strcmp(substr(path, 0, 2), "/.") == 0) //for /.Trash
		return 0;
	
	//for root and directories
	if ( strcmp( path, "/" ) == 0 || isDir( path ) == 1 )
	{
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; 
		if(isDir(path) == 1) { //directories
			Directory* dir = getDirFromPath(path);
			st->st_mtime = dir->mtime;
			st->st_atime = dir->atime;
			st->st_size = dir->size;
		} else { 
			st->st_mtime = time(NULL);
			st->st_atime = time(NULL);
		}
		return 0;

	} else if(isFile(path) == 1) { //for files
		st->st_mode = S_IFREG | 0644;
	 	st->st_nlink = 1;
	 	File* file = getFile(getFileName(path));
	 	st->st_size = file->size;
	 	st->st_atime = file->atime;
	 	st->st_mtime = file->mtime;
		return 0;
	}
	
	return -ENOENT;
}

/*	gets called when content of directory is listed - ls 	*/
int lfs_readdir( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
	
	printf("READING DIR: (path=%s)\n", path);		

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	if ( strcmp( path, "/" ) == 0 ) //root dir
	{
		for(Directory* dir = dirList->head; dir; dir = dir->next) {
			if(dir->parent == NULL) {
				filler( buf, dir->name, NULL, 0 );
			}
			
		}
		for(File* file = fileList->head; file; file = file->next) {
			if(file->parent == NULL) 
				filler( buf, file->name, NULL, 0 );
			
		}
	} 
	else if(isDirName(getParentName(path)) == 1 || isInRootDir(path)) // not roots dirs
	{
		char* parentName = stradd(path, "/");
		for(Directory* dir = dirList->head; dir; dir = dir->next) {
			if(dir->parent) {
				if(strcmp(dir->parent->name, getParentName(parentName)) == 0) 
					filler( buf, dir->name, NULL, 0 );
			}
		}
		for(File* file = fileList->head; file; file = file->next) {
			if(file->parent)
				if(strcmp(file->parent->name, getParentName(parentName)) == 0)
					filler( buf, file->name, NULL, 0 );
		}
	}

	return 0;
}

//Permission
int lfs_open( const char *path, struct fuse_file_info *fi ) {
    printf("open: (path=%s)\n", path);
	return 0;
}

/* reads content of file */
int lfs_read( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
    if(isFile(path)) {
    	File* file = getFileFromPath(path);
    	char* data = file->data;
    	printf("Trying to read file: %s with strlen = %ld\n", file->path, file->size);

    	file->atime = time(NULL);
    	if(file->parent) {
    		file->parent->atime = time(NULL);
    		writeDirToDisk(handle, file->parent);
    	}

    	if(file->size == 0) { //empty file
    		return 0;
    	} else {
    		memcpy(buf, data, file->size);
    		return strlen(file->data);
    	}
    }
	return -5; //some error code
}

/* Writes to a file */
int lfs_write( const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info ) {
	printf("write: (path=%s)\n", path);

	if(isFile(path))
	{
		File* file = getFileFromPath(path);
		int len = size;
		if(file->size < len + offset) {
			if(!file->data) { //empty file
				file->data = (char*) calloc(len + offset, sizeof(char));
			} else
				file->data = (char*) realloc(file->data, (len + offset) * sizeof(char));
		}
		file->size = len;
		memcpy(file->data + offset, buffer, file->size);
		file->atime = time(NULL);
		file->mtime = time(NULL);
		if(file->parent) {
			file->parent->atime = time(NULL);
			file->parent->mtime = time(NULL);
			writeDirToDisk(handle, file->parent);
		} 
		writeFileToDisk(handle, file);
		return len;
	}
	return -56; //some error code
}

/*Closes file - unimplemented */
int lfs_release(const char *path, struct fuse_file_info *fi) {
	printf("release: (path=%s)\n", path);
	return 0;
}

/* Makes a directory */
int lfs_mkdir(const char* path, mode_t mode) {
	const char* c = getDirName(path);
	addDir(c, path);
	return 0;
}

/* Makes a file */
int lfs_mknod( const char *path, mode_t mode, dev_t dev) {
	const char* c = getFileName(path);
	addFile(c, path);
	return 0;
}

/* Removes a file */
int lfs_unlink(const char * path) {
	printf("removing FILE: %s\n", path);
	removeFile(path);
	
	return 0;
}

/* Removes a directory */
int lfs_rmdir(const char *path) {
	printf("removing DIR: %s, with size: %ld\n", path, getDirFromPath(path)->size);
	if(getDirFromPath(path)->size == 0) { //cant remove a folder with things inside
		removeDir(path);
	}
	return 0;
}

/* unimplemented - not important*/
int lfs_setxattr(const char* c1, const char* c2, const char* c3, size_t size, int i) {
	return 0;
}

/* unimplemented - not important*/
int lfs_utime( const char * path, struct utimbuf *buf) {
	return 0;
}

/* unimplemented - not important*/
int lfs_truncate(const char* path, off_t offset) {
	return 0;
}

int main( int argc, char *argv[] ) {
	dirList = newDirList();
	fileList = newFileList();

	handle = fopen("disk", "wb+");
	fuse_main( argc, argv, &lfs_oper );
	fclose(handle);

	return 0;
}