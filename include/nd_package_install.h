#ifndef ND_PACKAGE_INSTALL_H
#define ND_PACKAGE_INSTALL_H
/* Install several files as one change (plugin packages).  Every file is
 * first copied beside its destination as <name>.tmp; only when all copies
 * succeeded are they swapped in, in order, keeping any previous version as
 * <name>.bak until the end.  If any step fails, every destination is put
 * back when rollback succeeds. If rollback fails, .bak originals are kept
 * and the caller must report ND_PACKAGE_RECOVERY. Order matters:
 * list a plugin's manifest last so it never points at a missing program. */
#define ND_PACKAGE_PATH 300
#define ND_PACKAGE_OK 1
#define ND_PACKAGE_FAILED 0
#define ND_PACKAGE_RECOVERY (-1)
struct NDPackageFile {
    char source[ND_PACKAGE_PATH];
    char destination[ND_PACKAGE_PATH];
    /* used internally */
    char temporary[ND_PACKAGE_PATH],backup[ND_PACKAGE_PATH];
    int had_old,committed,recovery;
};
int nd_package_install(struct NDPackageFile *files,int count);
int nd_package_file_exists(const char *path);
/* DRAWER + NAME into OUT (AmigaDOS path rules); 0 if it does not fit. */
int nd_package_join(char *out,int size,const char *drawer,const char *name);
#ifdef ND_PACKAGE_TEST
extern int nd_package_fail_close;
extern int nd_package_fail_restore_at;
extern int nd_package_fail_rename_at; /* test hook: make the Nth rename fail */
#endif
#endif
