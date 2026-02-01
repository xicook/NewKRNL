#ifndef VFS_H
#define VFS_H

#include <stdint.h>

#define MAX_FILES 64
#define MAX_FILENAME 32
#define MAX_CONTENT 512

typedef enum { FS_FILE, FS_DIR } FSType;

typedef struct FSNode {
    char name[MAX_FILENAME];
    FSType type;
    char content[MAX_CONTENT];
    struct FSNode* parent;
    struct FSNode* children[MAX_FILES];
    int child_count;
    int size;
} FSNode;

void vfs_init();
FSNode* vfs_get_root();
FSNode* vfs_get_cwd();
void vfs_ls();
int vfs_mkdir(const char* name);
int vfs_mkfile(const char* name, const char* content);
int vfs_cd(const char* name);
int vfs_rm(const char* name);
char* vfs_read(const char* name);

#endif
