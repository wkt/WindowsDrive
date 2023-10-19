#import <Foundation/Foundation.h>
#include "mac_utils.h"


inline static NSArray<NSObject*>* to_array(NSXMLNode *node);

inline static NSDictionary<NSString*,NSObject*>*
to_dict(NSXMLNode *node)
{
    NSUInteger n_child = [node childCount];
    NSXMLNode *nd;

    NSString *k;
    NSMutableDictionary<NSString*,NSObject*>*ret = nil;

    for(NSUInteger i=0;i<n_child;i++){
        nd = [node childAtIndex:i];
        if([nd.name isEqualToString:@"key"]){
            k = [nd stringValue];
            continue;
        }

        if(ret == nil)
            ret = [NSMutableDictionary<NSString*,NSObject*> new];

        if ([nd.name isEqualToString:@"dict"]){
            [ret setValue:to_dict(nd) forKey:k];
        }else if ([nd.name isEqualToString:@"array"]){
            [ret setValue:to_array(nd) forKey:k];
        }else{
            [ret setValue:nd.stringValue  forKey:k];
        }
        
    }
    return ret;
}

inline static NSArray<NSObject*>* to_array(NSXMLNode *node)
{
    NSUInteger n_child = [node childCount];
    NSXMLNode *nd;
    NSMutableArray<NSObject*>*ret = nil;
    for(NSUInteger i=0;i<n_child;i++){
        nd = [node childAtIndex:i];
        if(ret == nil)
            ret = [NSMutableArray<NSObject*> new];
        if([nd.name isEqualToString:@"dict"]){
            [ret addObject:to_dict(nd)];
        }else if([nd.name isEqualToString:@"array"]){
            [ret addObject:to_array(nd)];
        }else{
            [ret addObject:@[nd.name,nd.objectValue]];
        }
    }
    return ret;
}

inline static  const NSArray<NSDictionary<NSString*,NSString*>*>*
ioreg_info_to_object(const std::string &xml_string)
{
    NSError *error = nil;
    NSData *data;
    if(xml_string.at(0) == '/'){
        data = [NSData dataWithContentsOfFile:@(xml_string.data())];
    }else{
        data = [NSData dataWithBytes:xml_string.data() length:xml_string.length()];
    }
    NSXMLDocument *xml = [[NSXMLDocument alloc] initWithData:data options:NSXMLNodeOptionsNone error:&error];
    NSArray *array = to_array([[xml.rootDocument childAtIndex:0] childAtIndex:0]);
    NSMutableArray<NSMutableDictionary<NSString*,NSString*>*>* rets = nil;
    
    //NSLog(@"array: %@",array);
    for(NSUInteger i=0;i<[array count];i++){
        NSString *disk;
        id dict = [array objectAtIndex:i];
        disk = [dict valueForKey:@"BSD Name"];
        NSArray *children = [dict objectForKey:@"IORegistryEntryChildren"];
        NSString *disk_type = [dict objectForKey:@"Content"];
        if([disk_type isEqualToString:@"GUID_partition_scheme"]){
            disk_type = @"gpt";
        }else{
            disk_type = @"dos";
        }
        disk = [NSString stringWithFormat:@"/dev/%@",disk];
        for(NSUInteger k=0;k<[children count];k++){
            NSDictionary *_child = [children objectAtIndex:k];
            NSArray *_children = [_child objectForKey:@"IORegistryEntryChildren"];
            for(NSUInteger j=0;j<[_children count];j++){
                NSDictionary *child = [_children objectAtIndex:j];
                NSString* pname = [child objectForKey:@"BSD Name"];
                if(pname == nil || [pname length] == 0){
                    continue;
                }
                NSMutableDictionary*md = nil;
                pname = [NSString stringWithFormat:@"/dev/%@",pname];
                if(md == nil){
                    md = [NSMutableDictionary new];
                }
                [md setObject:disk forKey:@"DISK"];
                [md setObject:pname forKey:@"PARTITION"];
                [md setObject:disk_type forKey:@"ID_PART_LABEL_TYPE"];
                [md setObject:[child valueForKey:@"Base"] forKey:@"ID_PART_ENTRY_OFFSET"];
                if(rets == nil){
                    rets = [NSMutableArray<NSMutableDictionary<NSString*,NSString*>*> new];
                }
                [rets addObject:md];
            }
        }

    }
    return rets;
}

const std::vector< std::map<std::string,std::string> >
disk_info_from_ioreg_xml(const std::string &xml_string)
{
    const NSArray<NSDictionary<NSString*,NSString*>*>* array = ioreg_info_to_object(xml_string);
    std::vector< std::map<std::string,std::string> > rets;
    if(array == nil)
        return rets;
    for(NSUInteger i=0;i<[array count];i++){
        NSDictionary<NSString*,NSString*>* dict = [array objectAtIndex:i];
        NSArray<NSString*>* keys = [dict allKeys];
        std::map<std::string,std::string> m;
        for(NSUInteger j=0;j<[keys count];j++){
            NSString *k = [keys objectAtIndex:j];
            NSString *v = [dict valueForKey:k];
            m[k.UTF8String] = v.UTF8String;
        }
        rets.push_back(m);
    }
    
    return rets;
}


