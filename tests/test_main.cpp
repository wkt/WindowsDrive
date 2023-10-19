#include<iostream>
#include<string>
#include<vector>

#include "utils.h"
#include "ntregf.h"
#include "ntmounteddevices.h"

int main(int argc,char *argv[])
{
    std::string s="";
    std::vector<std::string> ll;
    command_read("ls -l",s);
    std::cerr<<s<<std::endl;
    command_read("ls -l",ll);
    for(size_t i=0;i<ll.size();i++){
        std::cerr<<ll.at(i)<<std::endl;
    }
    //NTRegf reg("/home/wkt/PycharmProjects/NTMountedDevices/SYSTEM");
    NTMountedDevices ntmd;
    char h[] = "s000s\x0c\x46\x03\x00\x00\r\n";
    std::cerr<<"strip: `"<<strip("  s\tdsd \x0x  \t b \t\r\n")<<"`"<<std::endl;
    std::cerr<<hex(h,sizeof (h))<<std::endl;
    std::cerr<<hex(h,sizeof (h),true)<<std::endl;
/*
    std::cerr<<to_string(reg.mounted_devices())<<std::endl;
    std::cerr<<"MountedDevices: "<<to_string(reg.mounted_devices())<<std::endl;
    std::cerr<<"Windows: "<<reg.nt_version()<<std::endl;
*/

#ifdef __APPLE__
    const std::string& password = "123456";
    if(is_sudo_password_correct(password)){
        setup_setuid("wd_helper",password);
    }
    ntmd.read_dos_disk_id();
#endif
    ntmd.scan_paritions();
    ntmd.mount_paritions();
    std::cerr<<"Windows drives:\n"<<to_string(ntmd.windows_drives())<<std::endl;
    std::cerr<<"is_sudo_password_correct:"<<is_sudo_password_correct("123456")<<std::endl;
    return 0;
}
