#ifndef lfsfile
#define lfsfile

typedef struct File File;
typedef struct FileList FileList;

/* Methods inside this file */
void addFile(const char* , const char* ); 
int isFile(const char* );
int isFileName(const char* );
File* getFile(const char* );
char* getFileName(const char* );
File* getFileFromPath(const char* ); 

size_t calculateFileSize(File* );
void writeFileToDisk(FILE* , File* );
void readFileFromDisk(FILE* , File* );

/* Struct for the files*/
struct File {
	char* name;
	char* path;
	size_t size;
	Directory* parent;
	File* next;
	File* prev;
	long int offset;
	time_t atime;
	time_t mtime;
	char* data;
};

/* Struct for the linked list containing all the files*/
struct FileList {
	File* head;
	File* tail;
};

/* initializes the linked list for files*/
FileList* newFileList()
{
	FileList* fl = malloc(sizeof(FileList));
	fl->head = NULL;
	fl->tail = NULL;
	return fl;
}

// The file linked list 
FileList* fileList;

/* Adds a file to the linked list*/
void addFile(const char* name, const char* path) 
{
	File* newFile = malloc(sizeof(File));
	newFile->name = calloc(strlen(name), sizeof(char)); //allocate mem for name
	newFile->path = calloc(strlen(path), sizeof(char));
	strcpy(newFile->name, name); 
	strcpy(newFile->path, path); 
	newFile->parent = NULL;

	newFile->data = NULL;
	newFile->size = 0;

	newFile->atime = time(NULL);
	newFile->mtime = time(NULL);

	if(!fileList->tail) // empty list
	{
		fileList->head = newFile;
		fileList->tail = newFile;
		fileList->tail->next = NULL;
		fileList->tail->prev = NULL;
		newFile->offset = 0;
	} else {
		newFile->prev = fileList->tail;
		fileList->tail->next = newFile;
		fileList->tail = newFile;	
		writeFileToDisk(handle, newFile->prev);
	}
	//files can be head and have a parent directory
	if(isInRootDir(path) != 1) {
		newFile->parent = getDir(getParentName(path));
		newFile->parent->size++;
		newFile->parent->mtime = time(NULL);
		writeDirToDisk(handle, newFile->parent);
	}
	writeFileToDisk(handle, newFile);
}

size_t calculateFileSize(File* file) {
	size_t res = 0;
	res += sizeof(char) * strlen(file->name);
	res += sizeof(char) * strlen(file->path);
	if(file->data) {
		res += sizeof(char) * strlen(file->data);
	}
	res += sizeof(File);
	res -= 24; //Three char pointers
	return res;
}

/*Checks if file exists by path*/
int isFile(const char* path)
{
	for(File* file = fileList->head; file; file = file->next) {
		if(strcmp(path, file->path) == 0)
			return 1;
	}
	return 0;
}

/*Checks if file exists by name*/
int isFileName(const char* name)
{
	for(File* file = fileList->head; file; file = file->next) {
		if(strcmp(name, file->name) == 0)
			return 1;
	}
	return 0;
}

/*Searches for a file by name, and returns it as a File ptr */
File* getFile(const char* name) {

	for(File* file = fileList->head; file; file = file->next) {
		if(strcmp(name, file->name) == 0)
			return file;
	}
	return NULL;
}

/*Searches for a file by path, and returns it as a File ptr */
File* getFileFromPath(const char* path) {
	for(File* file = fileList->head; file; file = file->next) {
		if(strcmp(path, file->path) == 0)
			return file;
	}
	return NULL;
}

/* Removes a file from the linked list*/
int removeFile(const char* path) {
	if(isFile(path)) {
		File* fileToRemove = getFileFromPath(path);

		if(fileList->head == fileList->tail) { //only one element
			fileList->head = NULL;
			fileList->tail = NULL;
			free(fileToRemove->path);
			free(fileToRemove->name);
			free(fileToRemove->data);
			free(fileToRemove);
			return 0;
		}
		if(fileList->head == fileToRemove) { //if head of list
			fileList->head = fileToRemove->next;
		} 

		if(fileToRemove->next) {  //if in the middle somewhere
			File* tmp = fileToRemove->prev;
			if(tmp) {
				fileToRemove->prev->next = fileToRemove->next;
				writeFileToDisk(handle, fileToRemove->prev);
			}
			fileToRemove->next->prev = tmp;
			writeFileToDisk(handle, fileToRemove->next);
		} else { //removing tail
			fileList->tail = fileToRemove->prev;
			fileToRemove->prev->next = NULL;
		}

		if(fileToRemove->parent) { //if the file has a parent directory
			fileToRemove->parent->size--;
			fileToRemove->parent->mtime = time(NULL);
			writeDirToDisk(handle, fileToRemove->parent);
			fileToRemove->parent = NULL;
		}
		free(fileToRemove->path);
		free(fileToRemove->name);
		free(fileToRemove->data);
		free(fileToRemove);
	}
	return 0;
}

/* Gets the file name by looking for the last slash - same as in lfs_dir.h
	Only used for readabilty  */
char* getFileName(const char* path) {
	int a = 0;
	int len = strlen(path);

	for(int i = len; i >= 0; i--)
		if(path[i] == '/') { //first slash
			a = i;
			break;
		}

	return substr(path, a+1, len);
}


/*Reads files from the disk file*/
void readFileFromDisk(FILE* fptr, File* file) {
	int res = 0;
	if(file->prev) 
		file->offset = calculateFileSize(file->prev) + file->prev->offset;
 	
	fseek(fptr, file->offset, SEEK_SET);

	res = fread(file->name, sizeof(char)*strlen(file->name), 1, fptr);
	res = fread(file->path, sizeof(char)*strlen(file->path), 1, fptr);
	res = fread(&(file->size), sizeof(size_t), 1, fptr);
	if(file->parent)
		res = fread(file->parent, 8, 1, fptr);
	else
		fseek(fptr, 8, SEEK_CUR);
	if(file->next)
		res = fread(file->next, 8, 1, fptr);
	else
		fseek(fptr, 8, SEEK_CUR);
	if(file->prev)
		res = fread(file->prev, 8, 1, fptr);
	else
		fseek(fptr, 8, SEEK_CUR);

	res = fread(&(file->offset), sizeof(long int), 1, fptr);
	res = fread(&(file->atime), sizeof(time_t), 1, fptr);
	res = fread(&(file->mtime), sizeof(time_t), 1, fptr);
	if(file->data)
		res = fread(file->data, sizeof(char)*strlen(file->data), 1, fptr);
	else 
		fseek(fptr, 1, SEEK_CUR);
	printf("%d\n", res);
}

/*write files to the disk file*/
void writeFileToDisk(FILE* fptr, File* file) {
	if(file->prev) 
		file->offset = calculateFileSize(file->prev) + file->prev->offset;

	fseek(fptr, file->offset, SEEK_SET);
	fwrite(file->name, sizeof(char)*strlen(file->name), 1, fptr);
	fwrite(file->path, sizeof(char)*strlen(file->path), 1, fptr);
	fwrite(&file->size, sizeof(size_t), 1, fptr);
	if(file->parent)
		fwrite(file->parent, 8, 1, fptr);
	else
		fseek(fptr, 8, SEEK_CUR);
	if(file->next)
		fwrite(file->next, 8, 1, fptr);
	else
		fseek(fptr, 8, SEEK_CUR);
	if(file->prev)
		fwrite(file->prev, 8, 1, fptr);
	else
		fseek(fptr, 8, SEEK_CUR);
	fwrite(&file->offset, sizeof(long int), 1, fptr);
	fwrite(&file->atime, sizeof(time_t), 1, fptr);
	fwrite(&file->mtime, sizeof(time_t), 1, fptr);
	if(file->data)
		fwrite(file->data, sizeof(char)*strlen(file->data), 1, fptr);
	else 
		fseek(fptr, 1, SEEK_CUR);
}

#endif