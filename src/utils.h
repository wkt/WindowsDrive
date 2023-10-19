#ifndef UTILS_H
#define UTILS_H


#include<string>
#include<vector>
#include<map>
#include <sys/types.h>

#define MAX(a,b) ((a)>(b)?(a):(b))

#define MIN(a,b) ((b)>=(a)?(a):(b))

extern int command_read(const std::string &cmd,std::string &output);
extern int command_read(const std::string &cmd,std::vector<std::string> &output_lines);
extern int command_run(const std::string &cmd);

extern std::string strip(const std::string &s,const std::string &chars="");
extern std::vector<std::string> split(const std::string& s,const std::string &chars="");
/**
static inline std::string strip(const char *s,const char *chars="")
{
    return strip(std::string(s),std::string(chars));
}
**/


extern bool endswith(const std::string &s,const std::string &e,const bool &ignore_case=false);
extern bool startswith(const std::string &s,const std::string &e,const bool &ignore_case=false);

extern std::string hex(const uint8_t *d,const size_t& n,const bool& reverse = false,const bool &lower_case = true);
static inline std::string hex(const char *d,size_t n,bool reverse = false,bool lower_case = true)
{
    return hex((uint8_t*)d,n,reverse,lower_case);
}

extern std::string to_string(const std::map<std::string,std::string> &m);
extern std::string to_string(const std::vector< std::map<std::string,std::string> > &m);
extern std::string to_string(const std::vector<std::string> &lines);

extern std::string to_lower(const std::string& s);
extern std::string to_upper(const std::string& s);

/**
 * Arguments must end with NULL,follow name must be `const char*`
 * @brief find_path_by_names
 * @param path
 * @param name
 * @return 
 */
extern std::string find_path_by_names(const std::string &path, const char *name,...);
extern const std::string path_dirname(const std::string &path);
extern void add_executable_search_paths();

extern std::string sys_name();

extern bool is_sudo_password_correct(const std::string &password);

extern bool is_setuid_enabled(const std::string& path,const uid_t& uid=0);

extern const std::string proc_path(const pid_t& pid=-1);

extern const std::vector<std::string>& exec_search_paths();

extern bool setup_setuid(const std::string& proc_name,const std::string& password);

extern const std::string get_user_displayname();

extern bool can_setuid(const std::string& cmd);

/**
 * @brief get_time_ms
 * @return unix time in milliseconds
 */
extern int64_t get_time_ms();


template<typename K,typename V>
inline static V map_get(const std::map<K,V> &p, const K &key)
{
    if(p.count(key)>0)return p.at(key);
    return V();
}


extern std::string get_appimage_path();
extern std::string get_appimage_runtime_dir();
extern bool is_appimage_executable();
extern bool is_debug_mode();
extern bool has_executable(const std::string &name);
extern bool is_need_su();

extern std::string sys_product_type();
extern std::string sys_product_ver();

extern std::string sys_kernel_type();
extern std::string sys_kernel_ver();

extern std::string sys_cpu_arch();

#define WD_HELPER "wd_helper"

#endif // UTILS_H
