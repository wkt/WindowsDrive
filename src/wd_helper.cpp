/**
# _*_ coding: utf-8 _*_
#
# @File : wd_helper.cpp
# @Time : 2022-04-21 20:26
# Copyright (C) 2022 WeiKeting<weikting@gmail.com>. All rights reserved.
# @Description : Read disk id for MBR disk without root
#
#
**
*/

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>

#ifdef __APPLE__
#include <libproc.h>
#endif


inline static void usage()
{
    fprintf(stderr,"Usage: \r\n    wd_helper mount|disk_id disk_path\r\n");
}

/**
 * @brief is_dir_empty
 * @param dirname
 * @return 1 empty,0 not empty
 */
inline static int is_dir_empty(const char *dname)
{
    int n = 0;
    struct dirent *d;
    DIR *dir = opendir(dname);
    if (dir == NULL)
        return 1;

    while ((d = readdir(dir)) != NULL) {
        if(strcmp(d->d_name,".") == 0)continue;
        if(strcmp(d->d_name,"..") == 0)continue;
        n++;
        break;
    }
    closedir(dir);
    return n>0?0:1;
}

/**
    Change process's user id to @uid
**/
inline static int try_seteuid(uid_t uid)
{
    int ret;
    char path[1025] = {0};

    if (uid == getuid())
        return 0;

#ifdef __APPLE__
    ret = proc_pidpath (getpid(), path, sizeof(path)-1);
    if ( ret <= 0 ) {
        fprintf(stderr, "proc_pidpath(): %s\n", strerror(errno));
        return -1;
    }
#else
    ret = readlink("/proc/self/exe",path,sizeof(path)-1);
    if ( ret <= 0 ) {
        fprintf(stderr, "readlink(/proc/self/exe): %s\n", strerror(errno));
        return -1;
    }
#endif

    struct stat sb;
    if (stat(path,&sb) < 0){
        fprintf(stderr,"stat(): %s\r\n",strerror(errno));
        return -1;
    }

    if(getuid() != uid && S_ISUID & sb.st_mode && sb.st_uid == uid){
        /** change user to root if allowed **/
        if(seteuid(0)<0){
            fprintf(stderr,"seteuid(): %s\r\n",strerror(errno));
            return -1;
        }
        if(setuid(uid)<0){
            fprintf(stderr,"setuid(): %s\r\n",strerror(errno));
            return -1;
        }
    }
    return 0;
}

inline static int read_disk_id(const char *path)
{

    struct stat sb;
    if (stat(path,&sb) < 0){
        fprintf(stderr,"stat(): %s\r\n",strerror(errno));
        return -1;
    }
    if(! S_ISBLK(sb.st_mode)){
        fprintf(stderr,"%s: must be a block file\r\n", path);
        return -1;
    }

    FILE *fp;
    fp = fopen(path,"rb");
    if(fp == NULL){
        fprintf(stderr,"fopen(%s): %s\r\n",path,strerror(errno));
        return -1;
    }
    unsigned char _id[4];
    fseek(fp,440,SEEK_SET);
    size_t n = fread(_id,1,sizeof(_id),fp);
    fclose(fp);
    if(n<=0)return -1;

    fprintf(stdout,"%s: ",path);
    for(int i=sizeof(_id)-1;i>=0;i--){
        fprintf(stdout,"%0x",_id[i]);
    }
    fprintf(stdout,"\r\n");
    return 0;
}

inline static int mount_partition(const char *dev_path,uid_t uid)
{
    char mount_path[1024] = {0};
    char dpath[1024] = {0};
    snprintf(dpath,sizeof(dpath)-1,"%s",dev_path);
    const char *bn = basename(dpath);
    struct stat st;
    for(int i=0;i<32;i++){
        if(i==0){
            snprintf(mount_path,sizeof (mount_path)-1,"/media/wd_helper/%s",bn);
        }else{
            snprintf(mount_path,sizeof (mount_path)-1,"/media/wd_helper/%s_%d",bn,i);
        }
        if(access(mount_path,F_OK) != 0){
            break;
        }
        memset(&st,0,sizeof(st));
        if(stat(mount_path,&st) !=0){
            fprintf(stderr,"stat(%s): %s\n",mount_path,strerror(errno));
            continue;
        }
        if(S_ISDIR(st.st_mode) && is_dir_empty(mount_path)){
            break;
        }
    }
    char cmd[1024*5] = {0};
    snprintf(cmd,
             sizeof(cmd)-1,
             "mkdir -p '%s' && chmod a+rx '%s' && mount -o nosuid,nodev,noexec,uid=%d,fmask=0022,dmask=0022 '%s' '%s'",
             mount_path,mount_path,uid,dev_path,mount_path);
    //printf("%s\n",cmd);
    return system(cmd);
}

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc,char *argv[])
{
    if(argc < 3){
        usage();
        return 1;
    }
    int i = 2;
    char*cmd = argv[1];
    uid_t uid = getuid();
    if(strcasecmp(cmd,"mount") == 0){
        try_seteuid(0);
        int r = 0;
        for(;i<argc;i++){
            r = mount_partition(argv[i],uid);
            if(r!=0)return 4;
        }
    }else if(strcasecmp(cmd,"disk_id") == 0 || strcasecmp(cmd,"disk-id") == 0){
        try_seteuid(0);
        for(;i<argc;i++){
            if(read_disk_id(argv[i]) < 0){
                return 2;
            }
        }
    }else{
        usage();
        return 3;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
