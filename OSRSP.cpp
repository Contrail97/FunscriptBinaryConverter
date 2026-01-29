#include <string>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

using String = std::string;

template<typename  T>
static inline String to_string(T var) {
    return std::to_string(var);
}

static inline String generate_tcode(const char * axis, char pos) {
    
    static char tcode[8];

    if (pos > 99) pos = 99;
    if (pos < 0) pos = 0;

    memset(tcode, 0, sizeof(tcode));
    sprintf(tcode, "%s%04d ", axis, pos * 100);

    return String(tcode);
}

static inline unsigned long get_curr_time_ms() {
    return GetTickCount();
};


struct OSRSB_Header
{
    int frame;
    int duration;   //ms
    int interval;   //ms
    char title[24];
    char version[8];
    char reserved[20];

    operator String() const
    {
        String _base("\tframe: ");
        _base += to_string(frame);
        _base += String("  duration: ");
        _base += to_string(duration);
        _base += String("  interval: ");
        _base += to_string(interval);
        _base += String("  title: ");
        _base += String(title);
        _base += String("  version: ");
        _base += String(version);

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


class OSR_SCRIPT
{
    typedef enum _SCRIPT_PLAY_STATE_
    {
        SCRIPT_STOPPED,
        SCRIPT_PAUSED,
        SCRIPT_PLAYING,
    }SCRIPT_PLAY_STATE;

private:

    FILE* _file;
    String _path;
    OSRSB_Header _header;
    OSRSB_Body * _buffer;
    
    int _file_pos;
    int _file_size;
    int _frame_pos;
    int _last_frame_pos;
    int _start_frame_pos;
    unsigned long _start_time;

    int _buffer_length;
    int _buffer_start_frame_pos;

    int _interval;
    bool _validation;
    SCRIPT_PLAY_STATE _state;


    bool _parse_script_bin() {

        _file = fopen(_path.c_str(), "rb"); 
        if (_file == NULL) {
            perror((String("Error opening file: ") + _path).c_str());
            return false;
        }

        size_t bytesRead = fread(&_header, 1, sizeof(OSRSB_Header), _file);
        if (bytesRead != sizeof(OSRSB_Header)) {
            perror((String("Error parsing file: ") + _path).c_str());
            return false;
        }

        _file_pos = ftell(_file);
        fseek(_file, 0, SEEK_END); 
        _file_size = ftell(_file);
        fseek(_file, _file_pos, SEEK_SET);

        _interval = _header.interval;

        return _header.frame * sizeof(OSRSB_Body) + sizeof(OSRSB_Header) == _file_size;
    };

    void _load_from_script_bin() {

        long file_pos = _buffer_start_frame_pos * sizeof(OSRSB_Body) + sizeof(OSRSB_Header);
        fseek(_file, file_pos, SEEK_SET);
        size_t bytesRead = fread(_buffer, 1, _buffer_length * sizeof(OSRSB_Body), _file);
        if (bytesRead == 0) {
            perror((String("Error reading file: ") + _path + String(" at pos: ") + to_string(file_pos)).c_str());
            memset(_buffer, -1, _buffer_length * sizeof(OSRSB_Body));
        }
    };

    OSRSB_Body _get_current_motion() {

        OSRSB_Body act;

        int buffer_pos = _frame_pos - _buffer_start_frame_pos;

        if (buffer_pos < _buffer_length)
            act = _buffer[buffer_pos];
        else
        {
            _buffer_start_frame_pos = _frame_pos;
            _load_from_script_bin();
            act = _buffer[0];
        }

        return act;
    };

    String _transfer_into_tcode(OSRSB_Body& act) {

        String tcode;

        if (act.stroke != -1)
            tcode += generate_tcode("L0", act.stroke);

        if (act.pitch != -1)
            tcode += generate_tcode("R2", act.pitch);

        if (act.roll != -1)
            tcode += generate_tcode("R1", act.roll);

        if (act.twist != -1)
            tcode += generate_tcode("R0", act.twist);
        
        return tcode;
    };

public:

    OSR_SCRIPT(String path, int buffer_length = 128) {

        _path = path;
        _file_pos = 0;
        _frame_pos = 0;
        _buffer = nullptr;
        _start_frame_pos = 0;
        _last_frame_pos = -1;
        _state = SCRIPT_STOPPED;
        _buffer_length = buffer_length;
        _buffer_start_frame_pos = 0;

        _validation = _parse_script_bin();

    }

    ~OSR_SCRIPT() {

        if(_buffer)
        {
            delete _buffer;
            _buffer = nullptr;
        }
        
        fclose(_file);
    }

    void rewind(){
        set_pos(0);
    }

    void set_pos(int pos) {
        _frame_pos = pos;
        _start_frame_pos = _frame_pos;
        _load_from_script_bin();
    };

    int get_pos() {
        return _frame_pos;
    };

    void play(){

        if(_validation && !_buffer)
        {
            _buffer = new OSRSB_Body[buffer_length];
            _load_from_script_bin();
        }

        _state = SCRIPT_PLAYING;
        _start_frame_pos = _frame_pos;
        _start_time = get_curr_time_ms();
    }

    void pause() {
        _state = SCRIPT_PAUSED;
    }

    void stop() {
        rewind();
        _start_time = 0;
        _state = SCRIPT_STOPPED;
    }

    bool vaildate() {
        return _validation;
    }

    SCRIPT_PLAY_STATE roll(String& out_tcode){

        if(!_validation)
        {
            out_tcode = String("Invaild Script.");
            return SCRIPT_STOPPED;
        }

        if (_state == SCRIPT_PLAYING)
        {
            _frame_pos = (get_curr_time_ms() - _start_time) / _interval + _start_frame_pos;

            if (_frame_pos <= _header.frame)
            {
                if (_last_frame_pos != _frame_pos)
                {
                    _last_frame_pos = _frame_pos;
                    OSRSB_Body act = _get_current_motion();
                    out_tcode = _transfer_into_tcode(act);
                }
                else
                    out_tcode = String("");
            }
            else stop();
        }

        return _state;
    };

    void set_interval(int v) {
        if (v > 0 && v < 100000)
        {
            _interval = v;
            play();
        }
    }

    int get_interval() {
        return _interval;
    }

    String inline get_file_path() const {
        return _path;
    }
};


int main(int argc, char* argv[])
{
    //std::string input_path(argv[1]);
    std::string input_path("D:\\workspace\\backup\\(BlobCG)Anis.srbs");

    std::cout << "Loading script from: " << input_path << std::endl;

    OSR_SCRIPT osrs(input_path);
    
    if(!osrs.vaildate())
        std::cout << "Script file: " << osrs.get_file_path() << " is not available." << std::endl;
    else
    {
        String tcode;

        std::cout <<"Normal play" << std::endl;
        osrs.play();
        for(int cnt(0); cnt < 100 && osrs.roll(tcode); cnt ++)
        {
            std::cout << "[" << osrs.get_pos() << "|" << osrs.get_pos() * osrs.get_interval() << "] Tcode:" << tcode << std::endl;
            Sleep(osrs.get_interval());
        }

        std::cout << "Normal play 20ms" << std::endl;
        osrs.set_interval(20);
        for (int cnt(0); cnt < 100 && osrs.roll(tcode); cnt++)
        {
            std::cout << "[" << osrs.get_pos() << "|" << osrs.get_pos() * osrs.get_interval() << "] Tcode:" << tcode << std::endl;
            Sleep(osrs.get_interval());
        }

        std::cout << "Pause" << std::endl;
        osrs.pause();
        for (int cnt(0); cnt < 100 && osrs.roll(tcode); cnt++)
        {
            std::cout << "[" << osrs.get_pos() << "|" << osrs.get_pos() * osrs.get_interval() << "] Tcode:" << tcode << std::endl;
            Sleep(osrs.get_interval());
        }

        std::cout << "Normal play 1000ms" << std::endl;
        osrs.set_interval(1000);
        osrs.play();
        for (int cnt(0); cnt < 10 && osrs.roll(tcode); cnt++)
        {
            std::cout << "[" << osrs.get_pos() << "|" << osrs.get_pos() * osrs.get_interval() << "] Tcode:" << tcode << std::endl;
            Sleep(osrs.get_interval());
        }

        std::cout << "Normal play 100ms" << std::endl;
        osrs.set_pos(0);
        osrs.set_interval(100);
        osrs.play();
        while(osrs.roll(tcode))
        {
            std::cout << "[" << osrs.get_pos() << "|" << osrs.get_pos() * osrs.get_interval() << "] Tcode:" << tcode << std::endl;
            Sleep(osrs.get_interval());
        }

        osrs.rewind();
    }

    return 0;
}
