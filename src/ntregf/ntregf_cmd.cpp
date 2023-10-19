#include <libregf.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <iostream>
#include <map>
#include <vector>
#include <libgen.h>
#include <dirent.h>

inline static const std::string _path_dirname(const std::string &path){
    std::string s = path.substr(0); //copy string
    return std::string(dirname((char*)s.data()));
}

inline static const std::string _path_by_name(const std::string &path,const std::string& name)
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

inline static const std::string _find_path_by_names(const std::string &path, const char *name,...)
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
        ret = _path_by_name(ret,names[i]);
        if(ret.size() == 0){
            ret = "";
            break;
        }
    }
    return ret;
}


inline static const std::string _get_value(libregf_key_t *key,const char *name)
{
    libregf_value_t *value = NULL;
    char _data[1024] = {0};
    size_t n_data = 0;
    size_t sf_data = sizeof(_data);
    libregf_key_get_value_by_utf8_name(key,(const uint8_t*)name,strlen(name),&value,NULL);
    
    libregf_value_get_value_data_size(value,&n_data,NULL);
    if(n_data >sf_data) n_data = sf_data;
    libregf_value_get_value_utf8_string(value,(uint8_t*)_data,n_data,NULL);
    libregf_value_free(&value,NULL);

    return std::string(_data,n_data);
}

inline const std::string _strip(const std::string &_s)
{
    std::string dup_s(_s);
    char*s = (char*)(dup_s.c_str());
    std::string c=" \t\r\n\x0b\x0c\x7F";;

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

inline static const std::string _win_ver(const std::string& rgfn)
{
    libregf_file_t *regf = NULL;
    libregf_key_t *root = NULL;
    libregf_key_t *dev = NULL;

    const char key_path[] = "\\Microsoft\\Windows NT\\CurrentVersion";
    const size_t sf_key_path = sizeof (key_path);

    std::string sysd = _path_dirname(rgfn);
    std::string rgf = _find_path_by_names(sysd,"software",NULL);

    libregf_file_initialize(&regf, NULL);
    libregf_file_open(regf, rgf.data(), LIBREGF_OPEN_READ, NULL);
    libregf_file_get_root_key(regf,&root,NULL);

    libregf_key_get_sub_key_by_utf8_path(root,(uint8_t*)key_path,sf_key_path-1,&dev,NULL);

    std::string product_name = _get_value(dev,"ProductName");
    std::string csd_version = _get_value(dev,"CSDVersion");
    std::string build_lab_ex = _get_value(dev,"BuildLabEx");

    std::string bits = "";
    if (build_lab_ex.find("x86") != std::string::npos){
        bits = "x86 32bit";
    }else if(build_lab_ex.find("amd64") != std::string::npos){
        bits = "x86 64bit";
    }else if(build_lab_ex.find("arm") != std::string::npos){
        bits = "arm";
    }
    
    std::string win_ver = product_name;
    win_ver = _strip(win_ver);
    if(csd_version.length()>1){
        win_ver = win_ver +" "+csd_version;
    }

    if(bits.length()>0){
        win_ver = win_ver +" "+bits;
    }

    libregf_key_free(&dev,NULL);
    libregf_key_free(&root,NULL);

    libregf_file_close(regf, NULL);
    libregf_file_free(&regf, NULL);
    return win_ver;
}

inline static bool _endswith(const std::string &s,const std::string &e)
{
    bool ignore_case = true;
    size_t ns = s.length();
    size_t ne = e.length();
    if(ns<ne)return false;
    const char *ss = s.data()+(ns-ne);
    const char *ee = e.data();
    return ignore_case ? strncasecmp(ss,ee,ns) == 0: strncmp(ss,ee,ns) == 0;
}

inline static bool _startswith(const std::string &s,const std::string &e)
{
    bool ignore_case = true;
    size_t ns = s.length();
    size_t ne = e.length();
    if(ns<ne)return false;
    const char *ss = s.data();
    const char *ee = e.data();
    return ignore_case ? strncasecmp(ss,ee,ne) == 0: strncmp(ss,ee,ne) == 0;
}

inline static const std::string to_hex(uint8_t*d,size_t len)
{
    std::string ret;
    char buf[4] = {};
    for(size_t i=0;i<len;i++){
        snprintf(buf,3,"%02x",d[i]);
        ret += buf;
        ret +=" ";
    }
    ret = _strip(ret);
    return ret;
}

inline static const std::string
_to_string(const std::map<std::string,std::string>&m)
{
    std::string ret = "";
    for(std::map<std::string,std::string>::const_iterator it = m.begin();it != m.end();it++){
        ret.append(it->first+"="+it->second+"\n");
    }
    return ret;
}

inline static const std::string
_read_mounted_devices(const std::string& rgfn)
{
    libregf_file_t *regf = NULL;
    libregf_key_t *root = NULL;
    libregf_key_t *dev = NULL;
    int n_values = 0;
    const char md_name[] = "MountedDevices";

    libregf_file_initialize(&regf, NULL);
    libregf_file_open(regf, rgfn.data(), LIBREGF_OPEN_READ, NULL);
    libregf_file_get_root_key(regf,&root,NULL);
    
    libregf_key_get_sub_key_by_utf8_name(root,(const uint8_t*)md_name,sizeof(md_name)-1,&dev,NULL);
    libregf_key_get_number_of_values(dev,&n_values,NULL);
    
    size_t vn_size = 0;
    size_t vd_size = 0;
    char vn[1024] = {0};
    uint8_t vd[1024] = {0};
    const size_t sf_vn = sizeof (vn);
    const size_t sf_vd = sizeof (vd);
    std::map<std::string,std::string>m;

    for(int i=0;i<n_values;i++){
        libregf_value_t *value = NULL;
        memset(vn,0,sf_vn);
    
        libregf_key_get_value(dev,i,&value,NULL);
        libregf_value_get_utf8_name_size(value,&vn_size,NULL);
        if(vn_size>sf_vn)vn_size=sf_vn;
        libregf_value_get_utf8_name(value,(uint8_t*)vn,vn_size,NULL);
    
        if(_endswith(vn,":")){
            libregf_value_get_value_data_size(value,&vd_size,NULL);
            if(vd_size>sf_vd)vd_size = sf_vd;
            memset(vd,0,sf_vd);
            libregf_value_get_value_data(value,(uint8_t*)vd,vd_size,NULL);
            if(_startswith((char*)vd,"DMIO:ID:")){
                //std::cerr<<"vn: "<<vn<<",vd: "<<vd<<std::endl;
                m["gpt:"+to_hex(vd+8,vd_size-8)] = vn;
            }else if(vd_size == 12){
                m["mbr:"+to_hex(vd,vd_size)] = vn;
            }
        }
        libregf_value_free(&value,NULL);
    }
    libregf_key_free(&dev,NULL);
    libregf_key_free(&root,NULL);
    libregf_file_close(regf, NULL);
    libregf_file_free(&regf, NULL);

    return _to_string(m);
}

int main(int argc,char **argv)
{
    const char *vn = "_NT_REGF";
    char *v = getenv(vn);
    if(v == NULL)return 1;
    std::string rgfn(v);
    unsetenv(vn);
    std::cout<<"os="<<_win_ver(rgfn)<<std::endl;
    std::cout<<_read_mounted_devices(rgfn);
    return 0;
}
