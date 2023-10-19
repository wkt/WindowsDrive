#ifndef MAC_UTILS_H
#define MAC_UTILS_H
#include <map>
#include <string>
#include <vector>

///extern void mac_disks_info(std::map< std::string, std::map<std::string,std::string> > & disks_info);

extern const std::vector< std::map<std::string,std::string> > disk_info_from_ioreg_xml(const std::string &xml_string);

#endif // MAC_UTILS_H
