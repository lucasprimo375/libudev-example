#include <libudev.h>
#include <iostream>
#include <string.h>

struct udev;

uint64_t PartNumberFromSysName(const char *s);

int main(int argc, char *argv[]){
    if(argc < 2){
        std::cout << "THE DEVICE PATH MUST BE GIVEN" << std::endl;
        return 1;
    }

    std::cout << "IT HAS BEGUN FOR " << argv[1] << std::endl;

    struct udev *udev;

    udev = udev_new();
    if(udev == nullptr) {
        std::cout << "udev is null" << std::endl;
        return 1;
    }

    char *syspath = NULL;
    const char *temp_str = argv[1];

    std::size_t len = strlen(temp_str);
    syspath = new char[ len + 1];
    ((char*)(syspath))[len] = '\0';
    memcpy(syspath, temp_str, len);

    struct udev_device *device = udev_device_new_from_syspath(udev, syspath);
    if(device == nullptr) {
        std::cout << "device is null" << std::endl;
        return 1;
    }

    struct udev_enumerate *enumerate = nullptr;
    struct udev_list_entry *partitions = nullptr, *part_entry = nullptr;
    struct udev_device *part_dev = nullptr;
    struct udev_device *part_dev_parent = nullptr;
    const char *prop_temp = nullptr;

    enumerate = udev_enumerate_new(udev);
    if(enumerate == nullptr) {
        std::cout << "enumerate is null" << std::endl;
        return 1;
    }

    prop_temp = udev_device_get_property_value(device, "UDISKS_PARTITION_TABLE_SCHEME");
    std::string udev_partition_schema = "NOT FOUND";
    if (prop_temp != nullptr){
      udev_partition_schema = prop_temp;
    } else {
      prop_temp = udev_device_get_property_value(device, "ID_PART_TABLE_TYPE");
      if (prop_temp != NULL) udev_partition_schema = prop_temp;
    }
    std::cout << "UDISKS_PARTITION_TABLE_SCHEME: " << udev_partition_schema << std::endl;

    unsigned long device_id = udev_device_get_devnum(device);
    std::cout << "DEVICE_ID: " << device_id << std::endl;

    udev_enumerate_add_match_property(enumerate, "DEVTYPE", "partition");
    udev_enumerate_scan_devices(enumerate);

    partitions = udev_enumerate_get_list_entry(enumerate);

    int counter = 0;
    udev_list_entry_foreach(part_entry, partitions){
        if(counter == 0){
            std::cout << std::endl;
        }
        const char* device_name = udev_list_entry_get_name(part_entry);
        std::cout << "DEVICE NAME: " << device_name << std::endl;
        part_dev = udev_device_new_from_syspath(udev, device_name);
        if (part_dev != NULL) {
            part_dev_parent = udev_device_get_parent(part_dev);
            if (part_dev_parent != NULL) {
                std::string parent_path = udev_device_get_syspath(part_dev_parent);
                std::cout << "PARENT PATH: " << parent_path << std::endl;
                if (parent_path.compare(syspath) != 0) {
                    udev_device_unref(part_dev);
                    std::cout << "PARENT PATH IS WRONG" << std::endl;
                    continue;
                }
            } else {
                udev_device_unref(part_dev);
                std::cout << "PARENT PATH IS NULL" << std::endl;
                continue;
            }
        }

        std::cout << "YET ANOTHER(" << counter << ") PARTITION IS READ" << std::endl;

        prop_temp = udev_device_get_sysname(part_dev);
        if (NULL != prop_temp  && PartNumberFromSysName(prop_temp) != 0 ) {
          std::cout << "SYS NAME: " << prop_temp << std::endl;
          prop_temp = udev_device_get_property_value(part_dev, "ID_PART_ENTRY_TYPE");
          std::cout << "ID_PART_ENTRY_TYPE: " << prop_temp << std::endl;

          if(NULL == prop_temp) {
            prop_temp = udev_device_get_property_value(part_dev, "UDISKS_PARTITION_TYPE");
            std::cout << "UDISKS_PARTITION_TYPE: " << prop_temp << std::endl;
          }

          if("MBR" == udev_partition_schema && NULL != prop_temp) {
            std::cout << "PARTITION IS MBR" << std::endl;
          } else if ("GPT" == udev_partition_schema && NULL != prop_temp) {
            std::cout << "PARTITION IS GPT" << std::endl;
          } else  {
            std::cout << "PARTITION SCHEMA UNDEFINED" << std::endl;
          }
        }
        std::cout << std::endl;
        counter++;
    }
    std::cout << (counter > 0 ? "DONE" : "NO PARTITIONS FOUND") << std::endl;

    udev_unref(udev);
}

uint64_t PartNumberFromSysName(const char *s){
    std::string temp = s;
    std::string::reverse_iterator iter;
    uint64_t number = 0;
    uint64_t multi = 1;
    for(iter = temp.rbegin(); iter != temp.rend() && *iter <= '9' && *iter >= '0'; iter++, multi*=10) {
        number += (*iter - '0')*multi;
    }

    return number;
}
