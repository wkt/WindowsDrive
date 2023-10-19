#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <cstdarg>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/utsname.h>

#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include "config.h"

#ifdef __APPLE__
#include <libproc.h>
#endif


static char TEMP_DIR[128] = {0};
inline static const std::string get_user_temp_dir()
{
    if(TEMP_DIR[0] == 0){
        std::string tmp_dir("/tmp/u");
        tmp_dir+=std::to_string(getuid());
        tmp_dir+="/";
        tmp_dir+=std::to_string(getpid());
        strncpy(TEMP_DIR,tmp_dir.data(),MIN(tmp_dir.length(),sizeof (TEMP_DIR)-1));
        std::string cmd = "mkdir -p '";
        cmd += tmp_dir;
        cmd += "'";
        int st =system(cmd.data());
        if(st != 0){
            char tmpd[] = "/tmp/u_XXXXXX";
            char *p = mkdtemp(tmpd);
            strncpy(TEMP_DIR,p,sizeof(TEMP_DIR)-1);
        }
    }
    return TEMP_DIR;
}

static bool srand_inited = false;
inline static int get_rand_int(int min,int max)
{
    if(!srand_inited){
        srand(time(NULL));
        srand_inited = true;
    }
    if(min >max){
        return -1;
    }
    if(min == max){
        return min;
    }
    return min+int((rand()*1.0/RAND_MAX)*(max-min));
}


inline static int get_rand_int(int max)
{
    return get_rand_int(0,max);
}

static const char LETTERS[]= "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const size_t N_LETTERS = sizeof (LETTERS)-1;
inline static void random_letters(char *letters,size_t n)
{
    for(size_t i=0;i<n;i++){
        int ix = get_rand_int(N_LETTERS);
        //std::cerr<<__PRETTY_FUNCTION__<<",ix: "<<ix<<","<<LETTERS[ix]<<std::endl;
        letters[i] = LETTERS[ix];
    }
}


int command_read(const std::string &cmd,std::string &output)
{
    char buf[1025] = {0};
    int n = 0;
    FILE *fp = popen(cmd.data(),"r");
    if(fp == NULL){
       perror("popen():");
       return -1;
    }

    do {
        n = fread(buf,1,sizeof(buf)-1,fp);
        buf[n]=0;
        if(n>0) output.append(buf,n);
    }while(feof(fp) == 0);
    output = strip(output);
    pclose(fp);
    //std::cerr<<__FUNCTION__<<": "<<cmd<<" , `"<< (output.size()<32?output: std::to_string(output.size())) <<"`"<<std::endl;
    return 0;
}

int command_read(const std::string &cmd,std::vector<std::string> &output_lines)
{

    char *line = NULL;
    size_t len = 0;
    int n = 0;
    FILE *fp = popen(cmd.data(),"r");
    if(fp == NULL){
       perror("popen():");
       return -1;
    }

    std::string l;
    while ((n = getline(&line, &len, fp)) != -1) {
        l = strip(std::string(line,n));
        output_lines.push_back(l);
    }
    //std::cerr<<__PRETTY_FUNCTION__<<": "<<cmd<<" , `"<<(output_lines.size()>0?output_lines[0]:"")<<"`"<<std::endl;
    free(line);
    pclose(fp);
    return 0;
}

int command_run(const std::string &cmd)
{
    std::string out;
    return command_read(cmd,out);
}

std::string strip(const std::string &_s,const std::string &chars)
{
    std::string dup_s(_s);
    char*s = (char*)(dup_s.c_str());
    std::string c=chars;

    if(c.length() == 0){
        c=" \t\r\n\x0b\x0c\x7F";
        //std::cerr<<"c: "<<c<<", "<<c.length()<<std::endl;
    }

    size_t si = 0;
    for(si=0;si<dup_s.length();si++){
        //std::cerr<<"si:"<<si<<", s[si]:"<<s[si]<<std::endl;
        if(c.find(s[si]) == std::string::npos){
            break;
        }
    }

    size_t ei = 0;
    for(ei=dup_s.length()-1;ei>0;ei--){
        //std::cerr<<"ei:"<<ei<<", s[ei]:"<<s[ei]<<std::endl;
        if(c.find(s[ei]) == std::string::npos){
            break;
        }
        s[ei] = 0;
    }
    //std::cerr<<"_s: `"<<_s<<"`"<<std::endl;
    //std::cerr<<"dup_s: `"<<dup_s<<"`"<<std::endl;
    return std::string(s+si);
}

bool endswith(const std::string &s,const std::string &e,const bool &ignore_case)
{
    size_t ns = s.length();
    size_t ne = e.length();
    if(ns<ne)return false;
    const char *ss = s.data()+(ns-ne);
    const char *ee = e.data();
    return ignore_case ? strncasecmp(ss,ee,ns) == 0: strncmp(ss,ee,ns) == 0;
}

bool startswith(const std::string &s,const std::string &e,const bool &ignore_case)
{
    size_t ns = s.length();
    size_t ne = e.length();
    if(ns<ne)return false;
    const char *ss = s.data();
    const char *ee = e.data();
    return ignore_case ? strncasecmp(ss,ee,ne) == 0: strncmp(ss,ee,ne) == 0;
}

std::string hex(const uint8_t *d,const size_t& n,const bool& reverse, const bool& lower_case)
{
    std::string ret;
    char buf[4] = {0};
    const char *fmt = lower_case ? "%02x" : "%02X";
    if(reverse){
        for(int i=n-1;i>-1;i--){
            snprintf(buf,sizeof (buf),fmt,d[i]);
            ret.append(buf);
        }
    }else{
        for(size_t i=0;i< n;i++){
            snprintf(buf,sizeof (buf),fmt,d[i]);
            ret.append(buf);
        }
    }
    return ret;
}

std::string to_string(const std::map<std::string,std::string> &m)
{
    std::string ret = "{";
    for(std::map<std::string,std::string>::const_iterator it = m.begin();it != m.end();it++){
        if(it == m.begin()){
            ret.append("\n");
        }
        ret.append("  \""+it->first+"\": \""+it->second+"\"");
        if(it != m.end()){
            ret.append(",");
        }
        ret.append("\n");
    }
    ret.append("}\n");
    return ret;
}

std::string to_string(const std::vector< std::map<std::string,std::string> > &m)
{
    std::string ret = "[";
    for(std::vector< std::map<std::string,std::string> >::const_iterator it = m.begin();it != m.end();it++){
        if(it == m.begin()){
            ret.append("\n");
        }
        ret.append(to_string(*it));
        if(it != m.end()){
            ret.append(",\n");
        }
    }
    ret.append("]\n");
    return ret;
}

std::string to_string(const std::vector<std::string> &lines)
{
    std::string ret = "[";
    for(std::vector< std::string >::const_iterator it = lines.begin();it != lines.end();it++){
        if(it == lines.begin()){
            ret.append("\n");
        }
        ret.append("\'"+*it+"\'");
        if(it != lines.end()){
            ret.append(",\n");
        }
    }
    ret.append("]\n");
    return ret;
}

std::string to_lower(const std::string& s)
{
    std::string ret = s.substr(0);
    char *p = (char*)ret.data();
    for(size_t i=0;i<ret.length();i++){
        p[i] = tolower(p[i]);
    }
    return ret;
}

std::string to_upper(const std::string& s)
{
    std::string ret = s;
    char *p = (char*)ret.data();
    for(size_t i=0;i<ret.length();i++){
        p[i] = toupper(p[i]);
    }
    return ret;
}


static inline std::string path_by_name(const std::string &path,const std::string& name)
{
    std::string ret;
    DIR *dir = opendir(path.data());
    if(dir == NULL){
        return ret;
    }

    struct dirent *dnt;

    while((dnt = readdir(dir)) != NULL){
        if(strcasecmp(name.data(),dnt->d_name) == 0){
            ret = path +"/" +std::string(dnt->d_name);
            break;
        }
    }
    closedir(dir);
    return ret;
}

std::string find_path_by_names(const std::string &path, const char *name,...)
{
    std::vector<std::string> names;
    const char *n = name;
    if (n == NULL){
        return path;
    }

    va_list ap;
    va_start(ap,name);
    while (n != NULL) {
        names.push_back(n);
        n = va_arg(ap,const char*);
    }
    va_end(ap);

    std::string ret = path;
    for(size_t i =0;i<names.size();i++){
        ret = path_by_name(ret,names[i]);
        if(ret.size() == 0){
            ret = "";
            break;
        }
    }
    return ret;
}

const std::string path_dirname(const std::string &path){
    std::string s = path;
    return std::string(dirname((char*)s.data()));
}

static char _sys_name[66] = {0};
std::string sys_name()
{
    if(_sys_name[0] == 0){
        struct utsname buf;
        uname(&buf);
        strncpy(_sys_name,buf.sysname,sizeof (_sys_name)-1);
    }
    return _sys_name;
}


inline static std::string sudo_exec(const std::string& cmd,const std::string &password)
{
    const std::string& askpass=get_user_temp_dir()+"/askpass.sh";
    char env_name[9] = {0};
    random_letters(env_name,sizeof (env_name)-1);

    FILE *fp = fopen(askpass.data(),"w");
    if(fp == NULL){
        std::string s("fopen(");
        s+=askpass;
        s+="):";
        perror(s.data());
        return std::string();
    }

    fprintf(fp,"#!/bin/bash\n\n");
    fprintf(fp,"echo ${%s}\n\n",env_name);
    fprintf(fp,"rm -rf \"$0\"\n");
    fclose(fp);

    std::string out;
    command_read("chmod +rx-w '"+askpass+"'",out);
    out.clear();

    setenv(env_name,password.data(),1);
    setenv("SUDO_ASKPASS",askpass.data(),1);

    std::string _cmd = "sudo -A "+cmd +" 2>/dev/null";
    command_read(_cmd,out);
    //std::cerr<<__FUNCTION__<<",out:"<<out<<",_cmd:"<<_cmd<<std::endl;
    remove(askpass.data());

    unsetenv(env_name);
    unsetenv("SUDO_ASKPASS");

    return out;
}

bool is_sudo_password_correct(const std::string &password)
{
    return sudo_exec("id -u",password) == "0";
}


bool is_setuid_enabled(const std::string& path,const uid_t& uid)
{
    struct stat st;
    //std::cerr<<__FUNCTION__<<",path:"<<path<<std::endl;
    if(stat(path.data(),&st)<0){
        //fprintf(stderr,"stat(%s): %s\r\n",path.data(),strerror(errno));
        return false;
    }
    if(st.st_uid != uid){
        return false;
    }
    if((st.st_mode & S_IFREG) != S_IFREG){
        return false;
    }
    if((st.st_mode & S_ISUID) != S_ISUID){
        return false;
    }
    if(access(path.data(),R_OK) != 0 || access(path.data(),X_OK) != 0){
        //fprintf(stderr,"access(%s): %s\r\n",path.data(),strerror(errno));
        return false;
    }
    return true;
}

const std::string proc_path(const pid_t& _pid)
{
    char path[1025] = {0};
    pid_t pid = _pid;
    int st = 0;

    if(pid < 0){
        pid = getpid();
    }

#ifdef __APPLE__
    st = proc_pidpath (pid, path, sizeof(path)-1);
    if ( st <= 0 ) {
        fprintf(stderr, "proc_pidpath(): %s\n", strerror(errno));
        return "";
    }
#else
    std::string pef = "/proc/"+std::to_string(pid)+"/exe";
    st = readlink(pef.data(),path,sizeof(path)-1);
    if ( st <= 0 ) {
        fprintf(stderr, "readlink(): %s\n", strerror(errno));
        return "";
    }
#endif
    return std::string(path);

}


static std::vector<std::string>* EXE_PATHS = NULL;
const std::vector<std::string>& exec_search_paths()
{
    if(EXE_PATHS == NULL){
        EXE_PATHS = new std::vector<std::string>();
        EXE_PATHS->push_back(path_dirname(proc_path()));
        EXE_PATHS->push_back("/usr/local/bin");
        EXE_PATHS->push_back("/usr/sbin");
        EXE_PATHS->push_back("/usr/bin");
        EXE_PATHS->push_back("/sbin");
        EXE_PATHS->push_back("/bin");
    }
    return *EXE_PATHS;
}

bool find_executable(const std::string &name,std::string& exec_path)
{
    const std::string& bn = strip(name);
    const std::vector<std::string>&search_paths = exec_search_paths();
    for(size_t i=0;i<search_paths.size();i++){
        const std::string& p = search_paths[i] +"/"+bn;
        if(access(p.data(),X_OK) == 0){
            exec_path = p;
            return true;
        }
    }
    return false;
}

bool can_setuid(const std::string& cmd)
{
    std::string bn = basename((char*)(cmd.substr().data()));
    const std::vector<std::string>&search_paths = exec_search_paths();
    for(size_t i=0;i<search_paths.size();i++){
        const std::string& p = search_paths[i] +"/"+bn;
        if(is_setuid_enabled(p)){
            return true;
        }
    }
    return false;
}

bool has_executable(const std::string &name)
{
    const std::vector<std::string>&search_paths = exec_search_paths();
    for(size_t i=0;i<search_paths.size();i++){
        const std::string& p = search_paths[i] +"/"+name;
        if(access(p.data(),X_OK) == 0){
            return true;
        }
    }
    return false;
}


bool setup_setuid(const std::string& proc_name,const std::string& password)
{
    std::string path = proc_name;
    const std::vector<std::string>&search_paths = exec_search_paths();
    std::string bn = basename((char*)path.data());

    if(access(path.data(),R_OK) != 0){
        for(size_t i=0;i<search_paths.size();i++){
            const std::string& p = search_paths[i] +"/"+bn;
            if(is_setuid_enabled(p)){
                return true;
            }
        }
    }else if(is_setuid_enabled(path)){
        return true;
    }

    bool found = false;
    if(path.at(0) == '/' && access(path.data(),R_OK) == 0){
        found = true;
    }

    bool is_appimage = is_appimage_executable();
    for(size_t i=0;i<search_paths.size();i++){
        const std::string& bd = search_paths[i];
        const std::string& p = bd +"/"+bn;
        //std::cerr<<"path: "<<path<<std::endl;
        //std::cerr<<"p: "<<p<<std::endl;
        if(!found && access(p.data(),R_OK) == 0){
            path = p;
            found = true;
            std::string cmd = "sh -c \"chown root '"+p+"' && chmod u+s '"+p+"';\"";
            sudo_exec(cmd,password);
        }else if(found){
            std::string cmd = "sh -c \"mkdir -p '" +bd+ "'";
            if(is_appimage){
                char tmpf[] = "/tmp/.wd_t-XXXXXX";
                int fd = mkstemp(tmpf);
                if( fd == -1)continue;
                close(fd);
                command_run("cp -f '"+path+"' '"+std::string(tmpf)+"'");
                cmd += " && mv '"+std::string(tmpf)+"' '"+p+"'";
            }else{
                cmd += " && cp -f '"+path+"' '"+p+"'";
            }
            cmd += " && chown root '"+p+"'&& chmod 4755 '"+p+"';\"";
            //std::cerr<<"cmd: "<<cmd<<std::endl;
            sudo_exec(cmd,password);
        }
        if(is_setuid_enabled(p)){
            return true;
        }
    }
    return false;
}


const std::string
get_user_displayname()
{
    const char* name = getenv("USER");
    if (name == NULL || strlen(name) == 0)
        name = getenv("USERNAME");
    if(name == NULL)name = "";
    return name;
}

template<typename T,typename R>
R func(T *t, R (T::*fn)())
{
    return (t->*fn)();
}


template<typename T>
void func(T *t, void (T::*fn)())
{
    (t->*fn)();
}

int64_t
get_time_ms()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec *1000 + int64_t(tv.tv_usec/1000);
}

std::vector<std::string> split(const std::string& s,const std::string &chars)
{
    char *sc = new char[s.length()+2];
    std::string c=chars;
    std::vector<std::string>rets;

    if(c.length() == 0){
        c=" \t\r\n\x0b\x0c\x7F";
        //std::cerr<<"c: "<<c<<", "<<c.length()<<std::endl;
    }
    memcpy(sc,s.data(),s.length());
    sc[s.length()] = 0;

    const char *d = c.data();
    char*p = strtok(sc,d);
    while(p != NULL){
        const std::string& t = strip(p);
        if(t.size()>0)rets.push_back(t);
        p = strtok(NULL,d);
    }
    delete[] sc;
    return rets;
}

void add_executable_search_paths()
{

#if (defined(__APPLE__) && __APPLE__) || (defined(__APPIMAGE__) && __APPIMAGE__)
    const std::string exec_path  = proc_path();
    std::string exec_dir = path_dirname(exec_path);
    const char *env = getenv("PATH");
    if(env == NULL)env = "";
    std::string _env = strip(env);
    if(_env.length() == 0){
        _env=exec_dir;
    }else{
        _env+=":"+exec_dir;
    }
    _env +=":"+exec_dir+"/../Bin";
    setenv("PATH",_env.data(),1);
    //std::cerr<<"env: "<<_env<<", exec_dir: "<<exec_dir<<std::endl;
#endif

}

std::string
get_appimage_path()
{
    const char *p = getenv("APPIMAGE");
    
    if(p == NULL)p = "";
    
    //copy the string
    return std::string(p).substr(0);
}

std::string
get_appimage_runtime_dir()
{
    const char *p = getenv("APPDIR");

    if(p == NULL)p = "";

    //copy the string
    return std::string(p).substr(0);
}

bool
is_appimage_executable()
{
    std::string appimage = get_appimage_path();
    std::string rundir = get_appimage_runtime_dir();

    if(appimage.empty() || rundir.empty())return false;
    
    if(access(appimage.data(),F_OK) == 0 && access(rundir.data(),F_OK) == 0){
        return true;
    }
    return false;
}

bool
is_debug_mode()
{
    const char *e = getenv("DEBUG");
    if(e == NULL)e="";
    const std::string debug = to_lower(e);
    return debug == "1" || debug == "true" || debug == "yes";
}

static int _is_need_su = -1;
bool is_need_su(){
    if(_is_need_su == -1){

#if (defined(__APPLE__) && __APPLE__)
        _is_need_su = 1;
#else
        _is_need_su = 0;
        if(!has_executable("udisksctl")){
            _is_need_su = 1;
        }
#endif
    }
    return _is_need_su == 1?true:false;
}

std::string sys_product_type()
{
#ifdef __APPLE__
    return "macOS";
#else
    return "Linux";
#endif
}

std::string sys_product_ver()
{
    return "unknown";
}

std::string sys_kernel_type()
{
    struct utsname buf;
    int s = uname(&buf);
    ///qDebug()<<"s:"<<s<<",name:"<<buf.sysname<<"\n";
    if(s<0)return "unknown";
    return buf.sysname;
}

std::string sys_kernel_ver()
{
    struct utsname buf;
    int s = uname(&buf);
    ///qDebug()<<"s:"<<s<<",name:"<<buf.version<<"\n";
    if(s<0)return "unknown";
    return buf.version;
}

std::string sys_cpu_arch()
{
    struct utsname buf;
    int s = uname(&buf);
    ///qDebug()<<"s:"<<s<<",name:"<<buf.machine<<"\n";
    if(s<0)return "unknown";
    return buf.machine;
}
