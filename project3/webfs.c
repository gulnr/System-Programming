
#define FUSE_USE_VERSION 26

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/xattr.h>
#include <dirent.h>
#include <unistd.h>
#include <fuse.h>

#include <tidy.h>
#include <tidybuffio.h>


char *rw_path;
static char* translate_path(const char* path)
{

    char *rPath= malloc(sizeof(char)*(strlen(path)+strlen(rw_path)+1));

    strcpy(rPath,rw_path);
    if (rPath[strlen(rPath)-1]=='/') {
        rPath[strlen(rPath)-1]='\0';
    }
    strcat(rPath,path);

    return rPath;
}

static int webfs_getattr(const char *path, struct stat *st_data)
{
    int res;
    char *upath=translate_path(path);

    res = lstat(upath, st_data);
    free(upath);
    if(res == -1) {
        return -errno;
    }
    return 0;
}

static int webfs_readlink(const char *path, char *buf, size_t size)
{
    int res;
    char *upath=translate_path(path);

    res = readlink(upath, buf, size - 1);
    free(upath);
    if(res == -1) {
        return -errno;
    }
    buf[res] = '\0';
    return 0;
}

static int webfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
    DIR *dp;
    struct dirent *de;
    int res;

    (void) offset;
    (void) fi;

    char *upath=translate_path(path);

    dp = opendir(upath);
    free(upath);
    if(dp == NULL) {
        res = -errno;
        return res;
    }

    while((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

static int webfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    (void)path;
    (void)mode;
    (void)rdev;
    return -EROFS;
}

static int webfs_mkdir(const char *path, mode_t mode)
{
    (void)path;
    (void)mode;
    return -EROFS;
}

static int webfs_unlink(const char *path)
{
    (void)path;
    return -EROFS;
}


static int webfs_symlink(const char *from, const char *to)
{
    (void)from;
    (void)to;
    return -EROFS;
}

static int webfs_rename(const char *from, const char *to)
{
    (void)from;
    (void)to;
    return -EROFS;
}

static int webfs_link(const char *from, const char *to)
{
    (void)from;
    (void)to;
    return -EROFS;
}

static int webfs_chmod(const char *path, mode_t mode)
{
    (void)path;
    (void)mode;
    return -EROFS;

}

static int webfs_chown(const char *path, uid_t uid, gid_t gid)
{
    (void)path;
    (void)uid;
    (void)gid;
    return -EROFS;
}

static int webfs_utime(const char *path, struct utimbuf *buf)
{
    (void)path;
    (void)buf;
    return -EROFS;
}

static int webfs_open(const char *path, struct fuse_file_info *finfo)
{
    int res;

    int flags = finfo->flags;

    if ((flags & O_WRONLY) || (flags & O_RDWR) || (flags & O_CREAT) || (flags & O_EXCL) || (flags & O_TRUNC) || (flags & O_APPEND)) {
        return -EROFS;
    }

    char *upath=translate_path(path);

    res = open(upath, flags);

    free(upath);
    if(res == -1) {
        return -errno;
    }
    close(res);
    return 0;
}

static int webfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *finfo)
{

    int fd;
    int res;
    (void)finfo;

    char *upath=translate_path(path);

    fd = open(upath, O_RDONLY);

    if(fd == -1) {
        res = -errno;
        return res;
    }

    res = pread(fd, buf, size, offset);
  
    char *word = ".html";
    
	if(strstr(path, word ) != NULL) {
		TidyBuffer output = {0};
		TidyBuffer errbuf = {0};
		int rc = -1;
		char *cleansed_buffer_; 
		Bool ok;
		
		TidyDoc tdoc = tidyCreate();                     
        tidyLoadConfig(tdoc, "myconfig.txt" );
		
		ok = tidyOptSetBool( tdoc, TidyXhtmlOut, yes ); 
		if ( ok )
			rc = tidySetErrorBuffer( tdoc, &errbuf ); 
		if ( rc >= 0 )
			rc = tidyParseString( tdoc, buf);  
		if ( rc >= 0 )
			rc = tidyCleanAndRepair( tdoc );
		if ( rc >= 0 )
			rc = tidyRunDiagnostics( tdoc ); 
		if ( rc > 1 )
			rc = ( tidyOptSetBool(tdoc, TidyForceOutput, yes) ? rc : -1 );
		if ( rc >= 0 )
		{
			rc = tidySaveBuffer( tdoc, &output );
			cleansed_buffer_ = (char *)malloc(1);
			unsigned int size2 = 0;
			rc = tidySaveString(tdoc, cleansed_buffer_, &size2);
			
			// now size is the required size
			free(cleansed_buffer_);
			cleansed_buffer_ = (char *)malloc(size2+1);
			rc = tidySaveString(tdoc, cleansed_buffer_, &size2 );
		}
		
		printf( "\nDiagnostics:\n\n%s", errbuf.bp );
		tidyBufFree(&output);
		tidyBufFree(&errbuf);
		tidyRelease(tdoc);

		int charCnt = 0;
		char *tmp_buf_ptr = cleansed_buffer_;
		while (*tmp_buf_ptr != '\0'){
			  charCnt++;
			  tmp_buf_ptr++;
		}
		size = charCnt;

		strcpy(buf, cleansed_buffer_);
		close(fd);

		fd = truncate(upath, charCnt);
	}
	
	if(res == -1) {
        res = -errno;
    }
    close(fd);
    return res;
}

static int webfs_statfs(const char *path, struct statvfs *st_buf)
{
    int res;
    char *upath=translate_path(path);

    res = statvfs(upath, st_buf);
    free(upath);
    if (res == -1) {
        return -errno;
    }
    return 0;
}

static int webfs_release(const char *path, struct fuse_file_info *finfo)
{
    (void) path;
    (void) finfo;
    return 0;
}

static int webfs_fsync(const char *path, int crap, struct fuse_file_info *finfo)
{
    (void) path;
    (void) crap;
    (void) finfo;
    return 0;
}

static int webfs_access(const char *path, int mode)
{
    int res;
    char *upath=translate_path(path);
    if (mode & W_OK)
        return -EROFS;

    res = access(upath, mode);
    free(upath);
    if (res == -1) {
        return -errno;
    }
    return res;
}


struct fuse_operations webfs_oper = {
    .getattr     = webfs_getattr,
    .readlink    = webfs_readlink,
    .readdir     = webfs_readdir,
    .mknod       = webfs_mknod,
    .mkdir       = webfs_mkdir,
    .symlink     = webfs_symlink,
    .unlink      = webfs_unlink,
    .rename      = webfs_rename,
    .link        = webfs_link,
    .chmod       = webfs_chmod,
    .chown       = webfs_chown,
    .utime       = webfs_utime,
    .open        = webfs_open,
    .read        = webfs_read,
    .statfs      = webfs_statfs,
    .release     = webfs_release,
    .fsync       = webfs_fsync,
    .access      = webfs_access,
};


static int webfs_parse_opt(void *data, const char *arg, int key,
                          struct fuse_args *outargs)
{
    (void) data;
    (void) outargs;

    switch (key)
    {
    case FUSE_OPT_KEY_NONOPT:
        if (rw_path == 0)
        {
            rw_path = strdup(arg);
            return 0;
        }
        else
        {
            return 1;
        }
    case FUSE_OPT_KEY_OPT:
        return 1;
    }
    return 1;
}


int main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    int res;

    res = fuse_opt_parse(&args, &rw_path, NULL, webfs_parse_opt);
    if (res != 0)
    {
        fprintf(stderr, "Invalid arguments\n");
        exit(1);
    }
    if (rw_path == 0)
    {
        fprintf(stderr, "Missing readwritepath\n");
        exit(1);
    }

    fuse_main(args.argc, args.argv, &webfs_oper, NULL);

    return 0;
}

