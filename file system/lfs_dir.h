#ifndef lfsdir
#define lfsdir

typedef struct Directory Directory;
typedef struct DirList DirList;

/* Methods inside this file */
char* substr(const char*, int, int);
void addDir(const char*, const char*);
int isDir(const char*);
int isDirName(const char*);
Directory* getDir(const char*);
char* getParentName(const char* );
int isInRootDir(const char* );
char* getDirName(const char*);
char* stradd(const char* , const char* );
Directory* getDirFromPath(const char* );
int removeDir(const char* path );

size_t calculateDirSize(Directory* );
void writeDirToDisk(FILE* , Directory* );
void readDirFromDisk(FILE*, Directory* );

/* Struct for each directory */
struct Directory {
	char* name;
	char* path;
	size_t size; 
	Directory* parent;
	Directory* next;
	Directory* prev;
	long int offset;
	time_t atime;
	time_t mtime;
};

/* Struct for the Linked List*/
struct DirList {
	Directory* head;
	Directory* tail;
};

/* initializes the linked list*/
DirList* newDirList()
{
	DirList* dl = malloc(sizeof(struct DirList));
	dl->head = NULL;
	dl->tail = NULL;
	return dl;
}

//LinkedList containing all directories in file system
DirList* dirList;

/* Adds a directory*/
void addDir(const char* name, const char* path) 
{
	Directory* newDir = malloc(sizeof(Directory));
	newDir->name = calloc(strlen(name), sizeof(char)); //allocate mem for name
	newDir->path = calloc(strlen(path), sizeof(char));
	strcpy(newDir->name, name); 
	strcpy(newDir->path, path); 
	newDir->parent = NULL;
	newDir->size = 0;

	//sets m and a time
	newDir->atime = time(NULL);
	newDir->mtime = time(NULL);

	if(!dirList->tail) // empty list
	{
		dirList->head = newDir;
		dirList->tail = newDir;
		dirList->tail->next = NULL;
		dirList->tail->prev = NULL;
		newDir->offset = 0;
	} else {
		newDir->prev = dirList->tail;
		dirList->tail->next = newDir;
		dirList->tail = newDir;
		writeDirToDisk(handle, newDir->prev);

		if(isInRootDir(path) != 1) { // points to the parent directory if one exists
			newDir->parent = getDir(getParentName(path));
			newDir->parent->size++;
			newDir->parent->mtime = time(NULL);
			writeDirToDisk(handle, newDir->parent);
		}
	}
	writeDirToDisk(handle, newDir);
}

/* Calculates the amount of bytes to fill the disk*/
size_t calculateDirSize(Directory* dir) 
{
	size_t res = 0;
	res += sizeof(char) * strlen(dir->name);
	res += sizeof(char) * strlen(dir->path);
	res += sizeof(Directory);
	res -= 16; //two pointers
	return res;
}

/*Searches for a directory by path*/
int isDir(const char* path)
{
	for(Directory* dir = dirList->head; dir; dir = dir->next) {
		if(strcmp(path, dir->path) == 0)
			return 1;
	}
	return 0;
}

/*Searches for a directory by name*/
int isDirName(const char* name)
{
	for(Directory* dir = dirList->head; dir; dir = dir->next) {
		if(strcmp(name, dir->name) == 0)
			return 1;
	}
	return 0;
}

/*Searches for a directory by name - returns Directory ptr */
Directory* getDir(const char* name) {

	for(Directory* dir = dirList->head; dir; dir = dir->next) {
		if(strcmp(name, dir->name) == 0) 
			return dir;
		
	}
	return NULL;
}

/*Searches for a directory by path - returns Directory ptr */
Directory* getDirFromPath(const char* path) {
	for(Directory* dir = dirList->head; dir; dir = dir->next) {
		if(strcmp(path, dir->path) == 0) 
			return dir;
		
	}
	return NULL;
}

/* Gets the directory name by looking for the last slash */
char* getDirName(const char* path) {
	int a = 0;
	int len = strlen(path);

	for(int i = len; i >= 0; i--)
		if(path[i] == '/') { //first slash
			a = i;
			break;
		}

	return substr(path, a+1, len);
}

/* Gets the parent directory name by looking for the last, and second to last, slash */
char* getParentName(const char* path) {
	int a = 0;
	int b = 0;
	int len = strlen(path);

	for(int i = len; i >= 0; i--)
		if(path[i] == '/') { //first slash
			a = i;
			break;
		}

	for(int i = a-1; i >= 0 ||b > 0; i--)
		if(path[i] == '/') {//second slash
			b = i;
			break;
		}

	char* parentName = substr(path, 0, a);

	return getDirName(parentName);
}

/*Checks if a path is in root by counting slashes*/
int isInRootDir(const char* path) {
	int len = strlen(path);
	if(len == 1)
		return 1;
	int c = 0;
	for(int i = 0; i < len; i++) 
		if(path[i] == '/')
			c++;
	return c;
}

/* Removes a directory from the linked list*/
int removeDir(const char* path) 
{
	if(isDir(path)) {
		Directory* dirToRemove = getDirFromPath(path);

		if(dirList->head == dirList->tail) { //only one element
			dirList->head = NULL;
			dirList->tail = NULL;
			free(dirToRemove->path);
			free(dirToRemove->name);
			free(dirToRemove);
			return 0;
		}

		if(dirList->head == dirToRemove) { //if its the first element
			dirList->head = dirToRemove->next;
		} 

		if(dirToRemove->next) {  //if its in the middle somewhere
			Directory* tmp = dirToRemove->prev;
			if(tmp) {
				dirToRemove->prev->next = dirToRemove->next;
				writeDirToDisk(handle,dirToRemove->prev);
			}
			dirToRemove->next->prev = tmp;
			writeDirToDisk(handle, dirToRemove->next);
		} else { //removing tail
			dirList->tail = dirToRemove->prev;
			dirToRemove->prev->next = NULL;
		}

		if(dirToRemove->parent) { //if it has a parent directory
			dirToRemove->parent->size--;
			dirToRemove->parent->mtime = time(NULL);
			writeDirToDisk(handle, dirToRemove->parent);
			dirToRemove->parent = NULL;
		}
		free(dirToRemove->path);
		free(dirToRemove->name);
		free(dirToRemove);
			
	}
	return 0;
}

/* Helper function - gets a substring */
char* substr(const char *src, int m, int n)
{
	// get length of the destination string
	int len = n - m;

	// allocate (len + 1) chars for destination (+1 for extra null character)
	char *dest = (char*)malloc(sizeof(char) * (len + 1));

	// extracts characters between m'th and n'th index from source string
	// and copy them into the destination string
	for (int i = m; i < n && (*(src + i) != '\0'); i++)
	{
		*dest = *(src + i);
		dest++;
	}

	// null-terminate the destination string
	*dest = '\0';

	// return the destination string
	return dest - len;
}

/* Helper function - concatonates a string to another*/
char* stradd(const char* a, const char* b){
    size_t len = strlen(a) + strlen(b);
    char *ret = (char*)malloc(len * sizeof(char) + 1);
    *ret = '\0';
    return strcat(strcat(ret, a) ,b);
}

/* writes dir to disk file*/
void writeDirToDisk(FILE* fptr, Directory* dir) {
	if(dir->prev)
		dir->offset = calculateDirSize(dir->prev) + dir->prev->offset;


	fseek(fptr, dir->offset + 5000000, SEEK_SET);

	fwrite(dir->name, sizeof(char)*strlen(dir->name), 1, fptr);
	fwrite(dir->path, sizeof(char)*strlen(dir->path), 1, fptr);
	fwrite(&dir->size, sizeof(size_t), 1, fptr);
	if(dir->parent)
		fwrite(dir->parent, 8, 1, fptr);
	else
		fseek(fptr, 8, SEEK_CUR);
	if(dir->next)
		fwrite(dir->next, 8, 1, fptr);
	else
		fseek(fptr, 8, SEEK_CUR);
	if(dir->prev)
		fwrite(dir->prev, 8, 1, fptr);
	else
		fseek(fptr, 8, SEEK_CUR);	
	fwrite(&dir->offset, sizeof(long int), 1, fptr);
	fwrite(&dir->atime, sizeof(time_t), 1, fptr);
	fwrite(&dir->mtime, sizeof(time_t), 1, fptr);
}

/* reads dir from file disk*/
void readDirFromDisk(FILE* fptr, Directory* dir) {
	if(dir->prev)
		dir->offset = calculateDirSize(dir->prev) + dir->prev->offset;

	fseek(fptr, dir->offset + 5000000, SEEK_SET);
	int res = 0;
	res = fread(dir->name, sizeof(char)*strlen(dir->name), 1, fptr);
	res = fread(dir->path, sizeof(char)*strlen(dir->path), 1, fptr);
	res = fread(&(dir->size), sizeof(size_t), 1, fptr);
	if(dir->parent)
		res = fread(dir->parent, 8, 1, fptr);
	else
		fseek(fptr, 8, SEEK_CUR);
	if(dir->next)
		res = fread(dir->next, 8, 1, fptr);
	else
		fseek(fptr, 8, SEEK_CUR);
	if(dir->prev)
		res = fread(dir->prev, 8, 1, fptr);
	else
		fseek(fptr, 8, SEEK_CUR);
	res = fread(&(dir->offset), sizeof(long int), 1, fptr);
	res = fread(&(dir->atime), sizeof(time_t), 1, fptr);
	res = fread(&(dir->mtime), sizeof(time_t), 1, fptr);
	printf("%d", res);
}

#endif