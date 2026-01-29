#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>

#include "json.hpp"
#include "cppcli.hpp"

#define SAMPLE_INTERVAL_MS_DEFAULT 100
#define OSRSB_VERSION "V1.0"

using json = nlohmann::json;
namespace fs = std::filesystem;

struct OSRSB_Header
{
    int frame;
    int duration;   //ms
    int interval;   //ms
    char title[24];
    char version[8];
    char reserved[20];

    operator std::string() const
    {
        std::string _base("\tframe: ");
        _base += std::to_string(frame);
        _base += std::string("  duration: ");
        _base += std::to_string(duration);
        _base += std::string("  interval: ");
        _base += std::to_string(interval);
        _base += std::string("  title: ");
        _base += std::string(title);
        _base += std::string("  version: ");
        _base += std::string(version);

        return _base;
    }
};

struct OSRSB_Body
{
    char stroke;
    char pitch;
    char roll;
    char twist;

    union _EXT_
    {
        int reserved;
        struct _ATTR_
        {
            char surge;
            char sway;
            char _BLANK1;
            char _BLANK2;
        }attr;

    }ext;
};

typedef enum _OSRSB_MOTION_TYPE_
{
    OSRSB_MOTION_STROKE,
    OSRSB_MOTION_PITCH,
    OSRSB_MOTION_ROLL,
    OSRSB_MOTION_TWIST,
    OSRSB_MOTION_SURGE,
    OSRSB_MOTION_SWAY,
    OSRSB_MOTION_UNKNOWN,
}OSRSB_MOTION_TYPE;

typedef std::map<int, int> OSRSB_ACTION_TABLE;
typedef std::map<OSRSB_MOTION_TYPE, OSRSB_ACTION_TABLE> OSRSB_MOTION_TABLE;


std::vector<fs::path> findFilesWithSameBase(const fs::path& inputPath, std::string& out_base_name) 
{
    if (!fs::exists(inputPath))
        return {}; 

    fs::path absPath = fs::absolute(inputPath).lexically_normal();
    fs::path dir = absPath.parent_path();
    std::string filename = absPath.filename().string();

    size_t dotPos = filename.find('.');
    out_base_name = (dotPos == std::string::npos) ? filename : filename.substr(0, dotPos);

    std::vector<fs::path> result;

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue; 

        std::string curFile = entry.path().filename().string();
        if (curFile == filename) continue; 

        size_t curDotPos = curFile.find('.');
        std::string curBase = (curDotPos == std::string::npos) ? curFile : curFile.substr(0, curDotPos);

        if (out_base_name == curBase)
            result.push_back(entry.path());
    }

    result.push_back(inputPath);

    return result;
}

bool is_contains_substring(const std::string& str, const std::string& substr) {
    std::string lowerStr = str;
    std::string lowerSubstr = substr;

    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    std::transform(lowerSubstr.begin(), lowerSubstr.end(), lowerSubstr.begin(), ::tolower);

    return lowerStr.find(lowerSubstr) != std::string::npos;
}


int main(int argc,char * argv[])
{
    int sample_interval_ms(SAMPLE_INTERVAL_MS_DEFAULT);

    cppcli::Option opt(argc, argv);

    opt.emptyPrintHelpThenExit(false);

    cppcli::Param v_param = opt("-v", "set action sample interval in ms");
    v_param.limitNumRange(100, 10000).setDefault(SAMPLE_INTERVAL_MS_DEFAULT);

    cppcli::Param h_param = opt("-h", "gain help doc");
    h_param.setAsHelpParam();

    opt.parse();

    if (v_param.exists())
    {
        sample_interval_ms = v_param.getInt();
        std::cout << "-v = " << v_param.getString() << std::endl;
    }

    if (argc == 1)
    {
        std::cout << "usage: OSRST.exe path/to/funscript [-v] " << std::endl;
        return 0;
    }

    std::string input_path(argv[1]);
    std::string base_name;
    auto file_list = findFilesWithSameBase(input_path, base_name);

    std::cout << "Script: " << base_name << std::endl << std::endl;
    if (file_list.size() > 0)
    {
        std::string title;
        OSRSB_MOTION_TABLE motion_table;
        int max_time = 0;
        int max_pos = 0;

        for (auto item : file_list)
        {
            try
            {
                std::cout << item << std::endl;
                std::ifstream f(item);
                json j = json::parse(f);

                OSRSB_MOTION_TYPE motion_type;
                if (is_contains_substring(item.string(), std::string("pitch")))
                    motion_type = OSRSB_MOTION_PITCH;
                else if (is_contains_substring(item.string(), std::string("roll")))
                    motion_type = OSRSB_MOTION_ROLL;
                else if (is_contains_substring(item.string(), std::string("twist")))
                    motion_type = OSRSB_MOTION_TWIST;
                else if (is_contains_substring(item.string(), std::string("surge")))
                    motion_type = OSRSB_MOTION_SURGE;
                else if (is_contains_substring(item.string(), std::string("sway")))
                    motion_type = OSRSB_MOTION_SWAY;
                else if (is_contains_substring(item.string(), base_name + std::string(".funscript")))
                    motion_type = OSRSB_MOTION_STROKE;
                else
                {
                    motion_type = OSRSB_MOTION_UNKNOWN;
                    continue;
                }

                if (j.contains("actions"))
                {
                    json actions = j["actions"];
                    std::cout << "actions.size: " << actions.size() << " ";

                    for (int cnt(0); cnt < actions.size(); cnt++)
                    {
                        json item = actions[cnt];

                        if (max_time < item["at"])
                            max_time = item["at"];

                        if (max_pos < item["pos"])
                            max_pos = item["pos"];

                        motion_table[motion_type][item["at"]] = item["pos"];
                    }
                }

                if (j.contains("metadata"))
                {
                    json metadata = j["metadata"];

                    if (metadata.contains("duration"))
                        std::cout << "duration: " << metadata["duration"] << " ";

                    if (metadata.contains("title"))
                    {
                        title = metadata["title"];
                        std::cout << "title: " << title << " ";
                    }

                    std::cout << std::endl;
                }

                std::cout << std::endl;
            }
            catch (std::exception e){
                std::cerr << e.what() << std::endl;
            }
            
        }

        OSRSB_Header sb_header;
        memset(&sb_header, 0, sizeof(sb_header));
        sb_header.interval = sample_interval_ms;
        sb_header.duration = max_time;
        sb_header.frame = int((max_time + sample_interval_ms - 1) / sample_interval_ms) + 1;
        strcpy_s(sb_header.version, OSRSB_VERSION);

        strncpy_s(sb_header.title, title.c_str(), min(title.size(), sizeof(sb_header.title) - 1));
        sb_header.title[sizeof(sb_header.title) - 1] = 0;
        std::cout << "header{\r\n\t" << std::string(sb_header) << "\r\n} " << std::endl;
        
        OSRSB_Body * sb_body = new OSRSB_Body[sb_header.frame];        
        memset(sb_body, -1, sizeof(OSRSB_Body) * sb_header.frame);
        {
            for (OSRSB_MOTION_TABLE::iterator mot_it = motion_table.begin(); mot_it != motion_table.end(); ++mot_it)
            {
                int motion_type = mot_it->first;
                OSRSB_ACTION_TABLE action_table = mot_it->second;

                for (OSRSB_ACTION_TABLE::iterator act_it = action_table.begin(); act_it != action_table.end(); ++act_it)
                {
                    int at  = act_it->first;
                    int pos = act_it->second / float(max_pos) * 100;
                    int index = (at + sample_interval_ms - 1) / sample_interval_ms;

                    switch (motion_type)
                    {
                    case OSRSB_MOTION_STROKE: sb_body[index].stroke = pos; break;
                    case OSRSB_MOTION_PITCH : sb_body[index].pitch = pos; break;
                    case OSRSB_MOTION_ROLL  : sb_body[index].roll = pos; break;
                    case OSRSB_MOTION_TWIST : sb_body[index].twist = pos; break;
                    case OSRSB_MOTION_SURGE : sb_body[index].ext.attr.surge = pos; break;
                    case OSRSB_MOTION_SWAY  : sb_body[index].ext.attr.sway = pos; break;
                    }
                }
            }
        }

        base_name.erase(std::remove_if(base_name.begin(), base_name.end(), ::isspace), base_name.end());
        std::string outName = opt.getExecPath() + "\\" + base_name + ".srbs";
        std::ofstream outFile(outName, std::ios::binary | std::ios::out);
        if (!outFile) {
            std::cerr << "Failed to create file:" << outName  << std::endl;
            return -1;
        }

        outFile.write((char *)&sb_header, sizeof(sb_header));
        outFile.write((char*)sb_body, sizeof(OSRSB_Body) * long long(sb_header.frame));

        if (!outFile) {
            std::cerr << "" << std::endl;
            return -1;
        }

        std::cout << std::endl << "All job done, binary OSR script saved to " << outName << std::endl;
        std::cout << std::endl << "Press any key to exit ..." << std::endl;
        //std::cin.get();
    }

	return 0;
}
