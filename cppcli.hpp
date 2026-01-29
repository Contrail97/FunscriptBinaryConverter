#pragma once
#pragma once

#include <iostream>
#include <algorithm>
#include <assert.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>


// #define CPPCLI_DEBUG

#if defined(WIN32) || defined(_WIN64) || defined(__WIN32__)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace cppcli {
    static std::mutex _coutMutex;

#ifdef CPPCLI_DEBUG
    template <class... Args>
    void __cppcli_debug_print(const Args&... args)
    {
        std::unique_lock<std::mutex> lock(_coutMutex);
        std::cout << "[CPPCLI_DEBUG] ";
        auto printFunc = [](auto i) {
            std::cout << i;
        };
        std::initializer_list<int>{(printFunc(args), 0)...};
        std::cout << std::endl;
    }

#endif
}   // namespace cppcli


#ifdef CPPCLI_DEBUG
#define CPPCLI_DEBUG_PRINT(...) cppcli::__cppcli_debug_print(__VA_ARGS__)
#endif

namespace cppcli {
    class Option;
    struct ParamInfo;
    class Param;
    enum ErrorExitEnum {
        EXIT_PRINT_RULE = 0x00,
        EXIT_PRINT_RULE_HELPDOC = 0x01,
    };

    enum HelpDocEnum {
        NOT_IS_HELPDOC = 0x00,
        USE_DEFAULT_HELPDOC,
        USE_UER_DEFINED_HELPDOC,
    };

    namespace detail {
        enum ErrorEventType {
            MECESSARY_ERROR = 0x00,
            VALUETYPE_ERROR = 0x01,
            ONEOF_ERROR = 0x02,
            NUMRANGE_ERROR = 0x03,
        };

        enum ValueTypeEnum {
            STRING = 0x00,
            INT = 0x01,
            FOLAT = 0x02,
        };

        class algoUtil final {
        private:
        public:
            // command params add to map
            static void InitCommandMap(int length, char* strArr[], std::map<std::string, std::string>& stringMap);
            static bool isInt(const std::string& value);
            static bool isDouble(const std::string& value);
            static bool verifyDouble(const std::string& value);
        };

        template <class T>
        struct is_num {
            static constexpr bool value =
                std::is_same<T, int>::value || std::is_same<T, float>::value || std::is_same<T, double>::value;
        };

    }   // namespace detail
}   // namespace cppcli


/* ################################################################### */
/* ################################################################### */
/* ################################################################### */
/* ################################################################### */
/* ################################################################### */


namespace cppcli {
    struct ParamInfo {
        ParamInfo(const std::string& shortParam, const std::string helpInfo);
        ParamInfo(const std::string& shortParam, const std::string helpInfo, bool ness);
        std::string inputValue_;
        std::string key_;
        std::string helpInfo_;
        bool necessary_{ false };
        std::vector<std::string> limitOneVec_;
        std::pair<double, double> _limitNumRange;
        cppcli::detail::ValueTypeEnum valueType_{ cppcli::detail::ValueTypeEnum::STRING };
        std::string defaultValue_{ "[EMPTY]" };
        std::string errorInfo_;
        bool existsInMap_{ false };
    };



    class Param {
        friend Option;
        std::shared_ptr<cppcli::ParamInfo> paramInfo_;
        inline static cppcli::Param* helpParam;

    private:
        bool havaCommandKey(const std::string& comStr);
        std::string getError(cppcli::detail::ErrorEventType errorEventType);
        std::string buildHelpInfoLine();
        bool isVaild();
#ifdef CPPCLI_DEBUG
        std::string debugInfo() const;
#endif
    public:
        Param(const std::string& shortParam, const std::string& helpInfo);
        Param(const std::string& shortParam, const std::string& helpInfo, bool ness);

        bool exists();


        std::string getString() const;
        int getInt() const;
        double getDouble() const;


        template <class... Args>
        Param limitOneOf(Args... args);

        template <typename T, typename Y,
            typename = typename std::enable_if<cppcli::detail::is_num<T>::value&&
            cppcli::detail::is_num<Y>::value>::type>
            Param limitNumRange(T min, Y max);

        template <class T>
        Param setDefault(const T& defaultValue);


        Param setAsHelpParam();
        Param limitMustEnter(bool mustEnter = true);
        Param limitInt();
        Param limitFloat();
        Param limitDouble();
    };


}   // namespace cppcli


/* =================================================================================*/
/* =================================================================================*/
/* ===============================    Option    =====================================*/
/* =================================================================================*/
/* =================================================================================*/

namespace cppcli {

    class Option {
        bool emptyPrintHelpThenExit_{ false };
        cppcli::ErrorExitEnum _exitType = cppcli::ErrorExitEnum::EXIT_PRINT_RULE;
        inline static std::vector<cppcli::Param> _ruleVec;
        std::string _workPath;   // exe path
        std::string _execPath;   // exec command path

    public:
        inline static std::map<std::string, std::string> _commandMap;
        Option(int argc, char* argv[]);

    public:
        cppcli::Param operator()(const std::string& shortParam, const std::string& helpInfo);
        cppcli::Param add(const std::string& shortParam, const std::string& helpInfo);
        ~Option();


        void parse();
        static bool exists(const std::string& key);
        static bool exists(const cppcli::Param& rule);
        void emptyPrintHelpThenExit(bool isTrue = true);
#ifdef CPPCLI_DEBUG
        void printCommandMap();
#endif

        const std::string getWorkPath();
        const std::string getExecPath();

    private:
    private:
        void updateParamVec();
        std::string getInputValue(const cppcli::Param& rule);
        std::string buildHelpDoc();
        void printHelpDocAndExit();
        static bool isMapHaveParam(const cppcli::Param& rule);
        void pathInit();
        void errorExitFunc(const std::string errorInfo, int index, cppcli::ErrorExitEnum exitType,
            cppcli::detail::ErrorEventType eventType);
        int necessaryVerify();
        int valueTypeVerify();
        int numRangeVerify();
        int oneOfVerify();


        std::string getString(const std::string& shortParam);
        std::string getString(const cppcli::Param& rule);

        int getInt(const std::string& shortParam);
        int getInt(const cppcli::Param& rule);

        double getDouble(const std::string shortParam);
        double getDouble(const cppcli::Param& rule);
    };

}   // namespace cppcli



inline void cppcli::detail::algoUtil::InitCommandMap(int length, char* strArr[],
    std::map<std::string, std::string>& stringMap)
{

    // command params add to map
    std::string keyTmp;
    std::string valueTmp;
    int keyIndex = -1;

    for (int currentIndex = 1; currentIndex < length; currentIndex++)
    {
        std::string theStr(strArr[currentIndex]);
        if (keyIndex != -1 && theStr.size() > 0 && currentIndex == keyIndex + 1)
        {
            // if theStr is command key, set value as ""
            if (theStr.find_first_of('-') == 0 && theStr.size() > 1 && !std::isdigit(theStr.at(1)))
            {
                valueTmp = "";
            }
            else
            {
                valueTmp = theStr;
            }
            // valueTmp = theStr.find_first_of('-') == 0 && theStr.size() > 1 ? "" : theStr;

            stringMap.insert(std::make_pair(std::move(keyTmp), std::move(valueTmp)));
            keyTmp.clear();
            valueTmp.clear();

            keyIndex = -1;
        }

        if (theStr.find_first_of('-') == 0 && int(std::count(theStr.begin(), theStr.end(), '-')) < theStr.size() &&
            !std::isdigit(theStr.at(1)))
        {
            keyIndex = currentIndex;
            keyTmp = std::move(theStr);
        }

        if (currentIndex == length - 1 && keyIndex != -1)
        {

            stringMap.insert(std::make_pair(std::move(keyTmp), std::move("")));
        }
    }
}
inline bool cppcli::detail::algoUtil::isInt(const std::string& value)
{
    if (value.empty())
    {
        return false;
    }

    int startPos = value.at(0) == '-' ? 1 : 0;
    for (int i = startPos; i < value.size(); i++)
    {
        if (std::isdigit(value.at(i)) == 0)
            return false;
    }
    return true;
}

inline bool cppcli::detail::algoUtil::isDouble(const std::string& value)
{
    if (value.empty())
    {
        return false;
    }
    if (value.size() < 3)
        return false;
    std::string tmpValue = value.at(0) == '-' ? value.substr(0, value.npos) : value;
    int numCount = 0;
    for (char const& c : tmpValue)
    {
        if (std::isdigit(c) != 0)
            numCount++;
    }

    if (numCount == tmpValue.size() - 1 && tmpValue.rfind('.') > 0 && tmpValue.rfind('.') < tmpValue.size() - 1)
    {
        return true;
    }
    return false;
}

inline bool cppcli::detail::algoUtil::verifyDouble(const std::string& value)
{
    if (isInt(value) || isDouble(value))
        return true;
    return false;
}



inline cppcli::ParamInfo::ParamInfo(const std::string& shortParam, const std::string helpInfo)
    : key_(shortParam), helpInfo_(helpInfo), _limitNumRange(std::make_pair(double(-1), double(-1))) {};

inline cppcli::ParamInfo::ParamInfo(const std::string& shortParam, const std::string helpInfo, bool necessary)
    : key_(shortParam), helpInfo_(helpInfo), necessary_(necessary),
    _limitNumRange(std::make_pair(double(-1), double(-1))) {};



inline cppcli::Param::Param(const std::string& shortParam, const std::string& helpInfo)
{
    paramInfo_ = std::make_shared<cppcli::ParamInfo>(shortParam, helpInfo);
}

inline cppcli::Param::Param(const std::string& shortParam, const std::string& helpInfo, bool ness)
{
    paramInfo_ = std::make_shared<cppcli::ParamInfo>(shortParam, helpInfo, ness);
}

inline bool cppcli::Param::havaCommandKey(const std::string& comStr)
{
    return paramInfo_->inputValue_ == comStr;
}

inline bool cppcli::Param::isVaild()
{
    return nullptr == paramInfo_;
}

inline bool cppcli::Param::exists()
{
    return paramInfo_->existsInMap_;
}


inline std::string cppcli::Param::getString() const
{
    return paramInfo_->inputValue_;
}


inline int cppcli::Param::getInt() const
{
    return std::stoi(paramInfo_->inputValue_);
}

inline double cppcli::Param::getDouble() const
{
    return std::stod(paramInfo_->inputValue_);
}


template <class... Args>
inline cppcli::Param cppcli::Param::limitOneOf(Args... args)
{
    std::ostringstream oss;

    auto addToVec = [this, &oss](auto i) {
        oss << i;
        paramInfo_->limitOneVec_.push_back(std::move(oss.str()));
        oss.str("");
    };
    std::initializer_list<int>{(addToVec(args), 0)...};
    return *this;
}

template <typename T, typename Y, typename>
inline cppcli::Param cppcli::Param::limitNumRange(T min, Y max)
{
    paramInfo_->_limitNumRange = std::make_pair(double(min), double(max));
    return *this;
}

template <class T>
inline cppcli::Param cppcli::Param::setDefault(const T& defaultValue)
{
    std::ostringstream oss;
    oss << defaultValue;
    paramInfo_->defaultValue_ = oss.str();
    return *this;
}

inline cppcli::Param cppcli::Param::setAsHelpParam()
{
    if (paramInfo_->necessary_ == true)
    {
        paramInfo_->necessary_ = false;
    }
    cppcli::Param::helpParam = this;
    return *this;
}

inline cppcli::Param cppcli::Param::limitMustEnter(bool mustEnter)
{
    paramInfo_->necessary_ = true;
    return *this;
}
inline cppcli::Param cppcli::Param::limitInt()
{
    paramInfo_->valueType_ = cppcli::detail::ValueTypeEnum::INT;
    return *this;
}

inline cppcli::Param cppcli::Param::limitFloat()
{
    paramInfo_->valueType_ = cppcli::detail::ValueTypeEnum::FOLAT;
    return *this;
}
inline cppcli::Param cppcli::Param::limitDouble()
{
    return limitFloat();
}

inline std::string cppcli::Param::getError(cppcli::detail::ErrorEventType errorEventType)
{
    std::ostringstream oss;

    oss << "[";
    switch (errorEventType)
    {
    case cppcli::detail::ErrorEventType::MECESSARY_ERROR: {

        oss << paramInfo_->key_;

        break;
    }
    case cppcli::detail::ErrorEventType::VALUETYPE_ERROR: {
        if (paramInfo_->valueType_ == cppcli::detail::ValueTypeEnum::FOLAT)
            oss << std::move(" NUMBER (DOUBLE) ");
        else if (paramInfo_->valueType_ == cppcli::detail::ValueTypeEnum::INT)
            oss << std::move(" NUMBER (INT) ");
        break;
    }
    case cppcli::detail::ErrorEventType::ONEOF_ERROR: {
        for (int i = 0; i < paramInfo_->limitOneVec_.size(); i++)
        {
            if (i == (paramInfo_->limitOneVec_.size() - 1))
            {
                oss << paramInfo_->limitOneVec_.at(i);
                break;
            }
            oss << paramInfo_->limitOneVec_.at(i) << std::move(", ");
        }

        break;
    }
    case cppcli::detail::ErrorEventType::NUMRANGE_ERROR: {
        oss << paramInfo_->_limitNumRange.first << std::move("(MIN), ") << paramInfo_->_limitNumRange.second
            << std::move("(MAX)");
        break;
    }
    }
    oss << "]";
    return std::move(oss.str());
}

inline std::string cppcli::Param::buildHelpInfoLine()
{
    std::ostringstream oss;

    int commandsDis = 16;
    int helpInfoDis = 48;
    int necessaryDis = 20;
    int defaultStrDis = 20;
    int theDis = 2;

    oss << std::setw(commandsDis) << std::left << paramInfo_->key_;

    int writeTime = paramInfo_->helpInfo_.size() % helpInfoDis - theDis == 0
        ? int(paramInfo_->helpInfo_.size() / (helpInfoDis - theDis))
        : int((paramInfo_->helpInfo_.size() / (helpInfoDis - theDis))) + 1;
    std::string necessaryOutStr = paramInfo_->necessary_ ? "true" : "false";
    std::string defaultValueOutStr =
        paramInfo_->defaultValue_ == "[EMPTY]" ? paramInfo_->defaultValue_ : "=" + paramInfo_->defaultValue_;

    if (writeTime == 1)
    {
        oss << std::setw(helpInfoDis) << std::left << paramInfo_->helpInfo_;
        oss << std::setw(necessaryDis) << std::left << "MUST-ENTER[" + necessaryOutStr + "]";
        oss << std::setw(defaultStrDis) << std::left << "DEFAULT->" + paramInfo_->defaultValue_;
        oss << std::endl;
        return std::move(oss.str());
    }
    int pos = 0;
    for (int i = 0; i < writeTime; i++)
    {
        if (i == 0)
        {
            oss << std::setw(helpInfoDis) << std::setw(helpInfoDis)
                << paramInfo_->helpInfo_.substr(pos, helpInfoDis - theDis);
            oss << std::setw(necessaryDis) << std::left << "MUST-ENTER[" + necessaryOutStr + "]";
            oss << std::setw(defaultStrDis) << std::left << "DEFAULT->" + paramInfo_->defaultValue_;
            oss << std::endl;
            pos += helpInfoDis - theDis;
        }
        else
        {
            oss << std::setw(commandsDis + 4) << std::left << "";
            oss << paramInfo_->helpInfo_.substr(pos, helpInfoDis - theDis);
            oss << std::endl;
            pos += helpInfoDis - theDis;
        }
    }

    return std::move(oss.str());
}

#ifdef CPPCLI_DEBUG
inline std::string cppcli::Param::debugInfo() const
{

    std::ostringstream oss;


    oss << "command params --> " << paramInfo_->key_ << std::endl;


    oss << "[CPPCLI_DEBUG]     input value = " << paramInfo_->inputValue_ << std::endl;
    oss << "[CPPCLI_DEBUG]     necessary = " << paramInfo_->necessary_ << std::endl;
    oss << "[CPPCLI_DEBUG]     valueType = " << paramInfo_->valueType_ << std::endl;
    oss << "[CPPCLI_DEBUG]     default = " << paramInfo_->defaultValue_ << std::endl;
    oss << "[CPPCLI_DEBUG]     exist = " << paramInfo_->existsInMap_ << std::endl;

    oss << "[CPPCLI_DEBUG]     limitOneVec = (";
    for (int i = 0; i < paramInfo_->limitOneVec_.size(); i++)
    {
        if (i == paramInfo_->limitOneVec_.size() - 1)
        {
            oss << paramInfo_->limitOneVec_.at(i);
            break;
        }
        oss << paramInfo_->limitOneVec_.at(i) << ", ";
    }
    oss << "), size=" << paramInfo_->limitOneVec_.size() << std::endl;

    oss << "[CPPCLI_DEBUG]     limitNumRange = (" << paramInfo_->_limitNumRange.first << " "
        << paramInfo_->_limitNumRange.second << ")";

    return oss.str();
}
#endif



#ifdef CPPCLI_DEBUG
inline void cppcli::Option::Option::printCommandMap()
{
    CPPCLI_DEBUG_PRINT("-- commandMap, size = ", _commandMap.size());
    for (const std::pair<std::string, std::string>& pr : _commandMap)
    {
        CPPCLI_DEBUG_PRINT("    ", pr.first, "=", pr.second);
    }
    CPPCLI_DEBUG_PRINT("-- end commandMap");
}
#endif

inline void cppcli::Option::errorExitFunc(const std::string errorInfo, int index, cppcli::ErrorExitEnum exitType,
    cppcli::detail::ErrorEventType eventType)
{

    cppcli::Param rule = this->_ruleVec.at(index);

    // std::unique_lock<std::mutex> lock(cppcli::_coutMutex);
    std::ostringstream oss;
    if (eventType != cppcli::detail::ErrorEventType::MECESSARY_ERROR)
        oss << ", where command param = [" << rule.paramInfo_->key_ << "]";
    if (cppcli::Param::helpParam != nullptr)
    {
        oss << std::endl << "Use [" << cppcli::Param::helpParam->paramInfo_->key_ << "] gain help doc";
    }

    std::cout << errorInfo << rule.getError(eventType) << oss.str() << std::endl;
    if (exitType == cppcli::EXIT_PRINT_RULE_HELPDOC)
        std::cout << buildHelpDoc();

    std::exit(0);
}

inline cppcli::Option::Option(int argc, char* argv[])
{
    // init work path and exec path
    pathInit();

#ifdef CPPCLI_DEBUG
    CPPCLI_DEBUG_PRINT("---------------- argc argv start");
    auto beStr = [&]() -> const std::string {
        std::ostringstream oss;
        for (int i = 0; i < argc; i++)
            oss << argv[i] << "  ";
        return oss.str();
    };
    CPPCLI_DEBUG_PRINT("argc = ", argc, " || argv = ", beStr());
#endif

    _ruleVec.reserve(64);

    // command params save to map
    cppcli::detail::algoUtil::InitCommandMap(argc, argv, _commandMap);
#ifdef CPPCLI_DEBUG
    CPPCLI_DEBUG_PRINT("---------------- argv map start");
    printCommandMap();
#endif
}

inline cppcli::Option::~Option()
{
    // for (auto& rule : _ruleVec)
    // {
    //     if (rule != nullptr)
    //     {
    //         delete (rule);
    //     }
    // }
    // _ruleVec.clear();
}


inline cppcli::Param cppcli::Option::operator()(const std::string& shortParam, const std::string& helpInfo)
{
    if (shortParam.find("-") == shortParam.npos)
    {
        std::cout << "short-param must contains \"-\" " << std::endl;
        std::exit(-1);
    }

    _ruleVec.push_back(cppcli::Param(shortParam, helpInfo));
    return _ruleVec.back();
}

inline cppcli::Param cppcli::Option::add(const std::string& shortParam, const std::string& helpInfo)
{
    if (shortParam.find("-") == shortParam.npos)
    {
        std::cout << "short-param must contains \"-\" " << std::endl;
        std::exit(-1);
    }

    _ruleVec.push_back(cppcli::Param(shortParam, helpInfo));
    return _ruleVec.back();
}


inline void cppcli::Option::pathInit()
{

    char workBuf[1024];
#if defined(WIN32) || defined(_WIN64) || defined(__WIN32__)
    GetModuleFileNameA(NULL, workBuf, sizeof(workBuf));
#else
    auto none2 = readlink("/proc/self/exe", workBuf, sizeof(workBuf));
#endif

    _execPath = std::filesystem::current_path().string();
    _workPath = std::filesystem::path(workBuf).parent_path().string();

#ifdef CPPCLI_DEBUG
    CPPCLI_DEBUG_PRINT("execPath = ", _execPath, ", workPath = ", _workPath);
#endif
}

inline std::string cppcli::Option::getInputValue(const cppcli::Param& rule)
{

    std::string inputValue;
    if (_commandMap.find(rule.paramInfo_->key_) != _commandMap.end())
    {
        inputValue = _commandMap[rule.paramInfo_->key_];
    }

    return inputValue;
}

inline void cppcli::Option::updateParamVec()
{
    std::string inputValue;

    for (cppcli::Param rule : _ruleVec)
    {
        if (!isMapHaveParam(rule))
            continue;

        rule.paramInfo_->existsInMap_ = true;
        inputValue = getInputValue(rule);

        if (!inputValue.empty())
        {
            rule.paramInfo_->inputValue_ = inputValue;

            continue;
        }
        if (inputValue.empty() && rule.paramInfo_->defaultValue_ != "[EMPTY]")
        {
            rule.paramInfo_->inputValue_ = rule.paramInfo_->defaultValue_;
        }
    }
}

inline bool cppcli::Option::isMapHaveParam(const cppcli::Param& rule)
{
    return _commandMap.find(rule.paramInfo_->key_) != _commandMap.end();
}

inline bool cppcli::Option::exists(const cppcli::Param& rule)
{
#ifdef CPPCLI_DEBUG
    CPPCLI_DEBUG_PRINT("---------------- exist rule");
    CPPCLI_DEBUG_PRINT(rule.debugInfo());
#endif
    return isMapHaveParam(rule);
}

inline void cppcli::Option::emptyPrintHelpThenExit(bool isTrue)
{
    emptyPrintHelpThenExit_ = isTrue;
}

inline bool cppcli::Option::exists(const std::string& shortParam)
{

    for (cppcli::Param& param : _ruleVec)
    {
        if (param.paramInfo_->key_ == shortParam)
        {
#ifdef CPPCLI_DEBUG
            CPPCLI_DEBUG_PRINT("---------------- exist rule");
            CPPCLI_DEBUG_PRINT(param.debugInfo());
#endif
            return isMapHaveParam(param);
        }
    }
    return false;
}

inline std::string cppcli::Option::buildHelpDoc()
{
    std::ostringstream oss;
    oss << "workpath: " << _workPath << std::endl;
    oss << "options:" << std::endl;
    for (cppcli::Param& rule : _ruleVec)
    {
        oss << rule.buildHelpInfoLine();
    }
    return oss.str();
}

inline void cppcli::Option::printHelpDocAndExit()
{
#ifdef CPPCLI_DEBUG
    if (cppcli::Param::helpParam == nullptr)
    {
        CPPCLI_DEBUG_PRINT("warning: you don't set help param\n");
    }
#endif

    // if (!isMapHaveParam(*cppcli::Param::helpParam))
    // {
    //     return;
    // }

    std::cout << buildHelpDoc();
    std::exit(0);
}


inline int cppcli::Option::necessaryVerify()
{
    // cppcli::Param rule = nullptr;
    for (int index = 0; index < _ruleVec.size(); index++)
    {
        auto rule = _ruleVec.at(index);
        if (rule.paramInfo_->necessary_ && !isMapHaveParam(rule))
        {
#ifdef CPPCLI_DEBUG
            CPPCLI_DEBUG_PRINT("failed in necessaryVerify, fail rule in following");
            CPPCLI_DEBUG_PRINT(rule.debugInfo(), "\n");
#endif
            return index;
        }
    }
    return -1;
};

inline int cppcli::Option::valueTypeVerify()
{

    for (int index = 0; index < _ruleVec.size(); index++)
    {
        auto rule = _ruleVec.at(index);
        if (rule.paramInfo_->valueType_ == cppcli::detail::ValueTypeEnum::STRING || !isMapHaveParam(rule))
        {
            continue;
        }

        if (rule.paramInfo_->inputValue_.empty())
        {
            return index;
        }

        if (rule.paramInfo_->valueType_ == cppcli::detail::ValueTypeEnum::INT &&
            !cppcli::detail::algoUtil::isInt(rule.paramInfo_->inputValue_))
        {
#ifdef CPPCLI_DEBUG
            CPPCLI_DEBUG_PRINT("failed in valueTypeVerify, fail rule in following");
            CPPCLI_DEBUG_PRINT(rule.debugInfo(), "\n");
#endif


            return index;
        }

        if (rule.paramInfo_->valueType_ == cppcli::detail::ValueTypeEnum::FOLAT &&
            !cppcli::detail::algoUtil::verifyDouble(rule.paramInfo_->inputValue_))
        {
#ifdef CPPCLI_DEBUG
            CPPCLI_DEBUG_PRINT("failed in valueTypeVerify, fail rule in following");
            CPPCLI_DEBUG_PRINT(rule.debugInfo(), "\n");
#endif
            return index;
        }
    }
    return -1;
}

inline int cppcli::Option::numRangeVerify()
{
    // cppcli::ParamInfo* rule = nullptr;
    for (int index = 0; index < _ruleVec.size(); index++)
    {
        auto rule = _ruleVec.at(index);
        if (rule.paramInfo_->valueType_ == cppcli::detail::ValueTypeEnum::STRING || !isMapHaveParam(rule))
        {
            continue;
        }

        // no set it
        if (rule.paramInfo_->_limitNumRange.first == -1 && rule.paramInfo_->_limitNumRange.second == -1)
        {
            continue;
        }

        if (rule.paramInfo_->inputValue_.empty() ||
            !cppcli::detail::algoUtil::verifyDouble(rule.paramInfo_->inputValue_))
        {
#ifdef CPPCLI_DEBUG
            CPPCLI_DEBUG_PRINT("failed in numRangeVerify, fail rule in following");
            CPPCLI_DEBUG_PRINT(rule.debugInfo(), "\n");
#endif
            return index;
        }

        if (std::stod(rule.paramInfo_->inputValue_) < rule.paramInfo_->_limitNumRange.first ||
            std::stod(rule.paramInfo_->inputValue_) > rule.paramInfo_->_limitNumRange.second)
        {
#ifdef CPPCLI_DEBUG
            CPPCLI_DEBUG_PRINT("failed in numRangeVerify, fail rule in following");
            CPPCLI_DEBUG_PRINT(rule.debugInfo(), "\n");
#endif
            return index;
        }
    }
    return -1;
}

inline int cppcli::Option::oneOfVerify()
{

    for (int index = 0; index < _ruleVec.size(); index++)
    {
        cppcli::Param& rule = _ruleVec.at(index);
        if (rule.paramInfo_->limitOneVec_.size() == 0 || !isMapHaveParam(rule))
        {
            continue;
        }

        if (std::find(rule.paramInfo_->limitOneVec_.begin(), rule.paramInfo_->limitOneVec_.end(),
            rule.paramInfo_->inputValue_) == rule.paramInfo_->limitOneVec_.end())
        {

#ifdef CPPCLI_DEBUG
            CPPCLI_DEBUG_PRINT("failed in oneOfVerify, fail rule in following");
            CPPCLI_DEBUG_PRINT(rule.debugInfo(), "\n");
#endif

            return index;
        }
    }
    return -1;
}

inline const std::string cppcli::Option::getWorkPath()
{
    return _workPath;
}
inline const std::string cppcli::Option::getExecPath()
{
    return _execPath;
}

inline std::string cppcli::Option::getString(const std::string& shortParam)
{
    for (cppcli::Param& rule : _ruleVec)
    {
        if (rule.havaCommandKey(shortParam))
        {
            return rule.getString();
        }
    }
    std::cout << "error: don't set where short-param = " << shortParam << std::endl;
    std::exit(-1);
}


inline int cppcli::Option::getInt(const std::string& shortParam)
{
    for (cppcli::Param& rule : _ruleVec)
    {
        if (rule.havaCommandKey(shortParam))
        {
            return rule.getInt();
        }
    }
    std::cout << "error: don't set where short-param = " << shortParam << std::endl;
    std::exit(-1);
}

inline double cppcli::Option::getDouble(const std::string shortParam)
{
    for (cppcli::Param& rule : _ruleVec)
    {
        if (rule.havaCommandKey(shortParam))
        {
            return rule.getDouble();
        }
    }
    std::cout << "error: don't set where short-param = " << shortParam << std::endl;
    std::exit(-1);
}

inline std::string cppcli::Option::getString(const cppcli::Param& rule)
{
    return rule.getString();
}

inline int cppcli::Option::getInt(const cppcli::Param& rule)
{
    return rule.getInt();
}

inline double cppcli::Option::getDouble(const cppcli::Param& rule)
{
    return rule.getDouble();
}


inline void cppcli::Option::parse()
{
    // rules save Unique correspondence input value
    updateParamVec();

#ifdef CPPCLI_DEBUG
    CPPCLI_DEBUG_PRINT("---------------- rules vector start");
    for (int i = 0; i < _ruleVec.size(); i++)
    {
        CPPCLI_DEBUG_PRINT("vec index = ", i, "  ", _ruleVec[i].debugInfo());
    }

#endif

    if (emptyPrintHelpThenExit_ && cppcli::Option::_commandMap.empty())
    {
        printHelpDocAndExit();
    }

    if (cppcli::Param::helpParam != nullptr && isMapHaveParam(*cppcli::Param::helpParam))
    {
        printHelpDocAndExit();
    }


    int necessaryResult = necessaryVerify();
    int valueTypeResult = valueTypeVerify();
    int oneOfResult = oneOfVerify();
    int numRangeResult = numRangeVerify();

#ifdef CPPCLI_DEBUG
    CPPCLI_DEBUG_PRINT("---------------- verify result");
    CPPCLI_DEBUG_PRINT("necessaryResult=", necessaryResult, ", valueTypeResult=", valueTypeResult,
        ",oneOfResult=", oneOfResult, ", numRangeResult", numRangeResult);

#endif

    if (necessaryResult > -1)
    {
        errorExitFunc("Must enter this param: ", necessaryResult, _exitType,
            cppcli::detail::ErrorEventType::MECESSARY_ERROR);
    }

    if (valueTypeResult > -1)
    {
        errorExitFunc("Please enter the correct type: ", valueTypeResult, _exitType,
            cppcli::detail::ErrorEventType::VALUETYPE_ERROR);
    }

    if (oneOfResult > -1)
    {
        errorExitFunc("Must be one of these values: ", oneOfResult, _exitType,
            cppcli::detail::ErrorEventType::ONEOF_ERROR);
    }

    if (numRangeResult > -1)
    {
        errorExitFunc("Must be within this range: ", numRangeResult, _exitType,
            cppcli::detail::ErrorEventType::NUMRANGE_ERROR);
    }

#ifdef CPPCLI_DEBUG
    CPPCLI_DEBUG_PRINT("---------------- parse result");
    CPPCLI_DEBUG_PRINT(">>>>>>>>>   PASS   <<<<<<<<<<");
#endif
}
