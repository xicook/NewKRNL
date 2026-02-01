#include "vfs.h"
#include "lib.h"
#include "../drivers/vga.h"
#include <stddef.h>

static FSNode nodes[256];
static int node_count = 0;
static FSNode* root;
static FSNode* cwd;

// No local helpers needed

static FSNode* new_node(const char* name, FSType type) {
    if (node_count >= 256) return NULL;
    FSNode* n = &nodes[node_count++];
    kstrcpy(n->name, name);
    n->type = type;
    n->child_count = 0;
    n->size = 0;
    n->parent = NULL;
    for(int i=0; i<MAX_FILES; i++) n->children[i] = NULL;
    return n;
}

void vfs_init() {
    node_count = 0;
    root = new_node("/", FS_DIR);
    cwd = root;
}

FSNode* vfs_get_root() { return root; }
FSNode* vfs_get_cwd() { return cwd; }

void vfs_ls() {
    for (int i = 0; i < cwd->child_count; i++) {
        if (cwd->children[i]->type == FS_DIR) vga_set_color(11, 0); // Cyan for dirs
        else vga_set_color(15, 0); // White for files
        vga_puts(cwd->children[i]->name);
        vga_puts("  ");
    }
    vga_set_color(15, 0);
    vga_puts("\n");
}

int vfs_mkdir(const char* name) {
    if (cwd->child_count >= MAX_FILES) return -1;
    FSNode* n = new_node(name, FS_DIR);
    if (!n) return -1;
    n->parent = cwd;
    cwd->children[cwd->child_count++] = n;
    return 0;
}

int vfs_mkfile(const char* name, const char* content) {
    if (cwd->child_count >= MAX_FILES) return -1;
    FSNode* n = new_node(name, FS_FILE);
    if (!n) return -1;
    n->parent = cwd;
    if (content) kstrcpy(n->content, content);
    n->size = content ? 0 : 0; // Simplified
    cwd->children[cwd->child_count++] = n;
    return 0;
}

int vfs_cd(const char* name) {
    if (kstrcmp(name, "..") == 0) {
        if (cwd->parent) cwd = cwd->parent;
        return 0;
    }
    if (kstrcmp(name, "/") == 0) {
        cwd = root;
        return 0;
    }
    for (int i = 0; i < cwd->child_count; i++) {
        if (kstrcmp(cwd->children[i]->name, name) == 0 && cwd->children[i]->type == FS_DIR) {
            cwd = cwd->children[i];
            return 0;
        }
    }
    return -1;
}

int vfs_rm(const char* name) {
    int found = -1;
    for (int i = 0; i < cwd->child_count; i++) {
        if (kstrcmp(cwd->children[i]->name, name) == 0) {
            found = i;
            break;
        }
    }
    if (found != -1) {
        for (int i = found; i < cwd->child_count - 1; i++) {
            cwd->children[i] = cwd->children[i+1];
        }
        cwd->child_count--;
        return 0;
    }
    return -1;
}

char* vfs_read(const char* name) {
    for (int i = 0; i < cwd->child_count; i++) {
        if (kstrcmp(cwd->children[i]->name, name) == 0 && cwd->children[i]->type == FS_FILE) {
            return cwd->children[i]->content;
        }
    }
    return NULL;
}
