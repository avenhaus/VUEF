/*======================================================================*\
|* Configuration Variable Registry
|*
|* This framework provides a hierarchical registry for configuration 
|* variables. Each variable can register defaults, formatting, references,
|* getter/setter callbacks etc.
|*
|* The variables can be persisted in EEPROM (as JSON). The registry
|* can be used to control or read variables from CLI, Web Interfaces
|* or Display GUI interfaces.
\*======================================================================*/


#ifndef _CONFIG_REG_H_
#define _CONFIG_REG_H_

#include <Arduino.h>
#include <vector>
#include <iterator>
#include <cstddef>

#include "RegGroup.h"
#include "Helper.h"

// Disable warning about char being deprecated. Not sure why it's needed...
#define ARDUINOJSON_DEPRECATED(msg)
#include <ArduinoJson.h>

#define CONFIG_MAGIC 0x19710914

// Size of buffer for JSON written to EEPROM
#ifndef CONFIG_BUFFER_SIZE
#define CONFIG_BUFFER_SIZE 1024
#endif


/************************************************************************\
|* Global Functions
\************************************************************************/
void saveConfig();
bool loadConfig();
void defaultConfig();
bool parseConfigJson(char* jsonStr);

extern bool isConfigOk;


/************************************************************************\
|* EEPROM Config Header
\************************************************************************/

typedef struct ConfigHeader {
    uint32_t magic;
    size_t size;
    uint32_t crc;
} ConfigHeader;



/************************************************************************\
|* Config Variable Template Class for individual values
\************************************************************************/
template <class T> class ConfigVarT : public RegVar {
public:
    ConfigVarT(const char* name, const T deflt=0, const char* typeHelp=nullptr, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, T* ptr=nullptr, bool (*setCb)(T val, void* cbData)=nullptr, T (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : RegVar(name, typeHelp, info, fmt, group, flags | RF_IS_CONFIG), default_(deflt), value_(deflt), ptr_(ptr), setCb_(setCb), getCb_(getCb), cbData_(cbData) {
        if (!fmt) { fmt_ = FST("%d"); }
    }
    virtual ~ConfigVarT() {}

    virtual inline T* getValueRef() { return &value_; }

    virtual T get() {
        if (ptr_) { value_ = *ptr_; }
        if (getCb_) { value_ = getCb_(cbData_); }
        return value_;
    }

    virtual bool set(T val) {
        value_ = val;
        if (ptr_) { *ptr_ = value_; }
        if (setCb_) { return setCb_(value_, cbData_); }
        return false;
    }

    virtual size_t print(Print& stream) {
        return stream.printf(fmt_, get());
    }

    virtual size_t toStr(char* buffer, size_t size) {
        size_t n = 0;
        n += snprintf(buffer+n, size-n-1, fmt_, get());
        buffer[n] = '\0';
        return n;        
    }

    virtual size_t toJsonStr(char* buffer, size_t size, bool noName=false, uint32_t flags=0, RFFlag flagsMask=0) {
        if ((flags_ ^ flags) & flagsMask) { return 0; }
        size_t n = 0;
        if (!noName) { n += StrTool::toJsonName(buffer+n, size-n, name_); }
        return n + toStr(buffer+n, size-n);
    }

    virtual bool setFromJson(const JsonVariant& jv) {
        if (!jv.is<JsonInteger>()) {
            DEBUG_printf(FST("JSON config for \"%s\" is not an integer\n"), name_);
            return true;
        }
        set(jv);
        return false;
    }

    virtual size_t setFromStr(const char* valStr, const char** errorStr=nullptr) {
        int32_t value = 0;
        size_t n = StrTool::readInteger(valStr, &value, errorStr);
        if (n) { set(value); }
        return n;
    }

    virtual void setDefault() {
        set(default_);
    }

    virtual size_t getWebUi(Print* stream, uint32_t flags=0, RFFlag flagsMask=0) {
        if ((flags_ ^ flags) & flagsMask) { return 0; }
        size_t n = getWebUiCommon_(stream);
        get();
        n += stream->print(FST(",\"V\":"));
        n += stream->print(value_);
        stream->write('}'); n++;
        return n;
    }

protected:
    T default_;
    T value_;
    T* ptr_;
    bool (*setCb_)(T val, void* cbData);
    T (*getCb_)(void* cbData);
    void* cbData_;
};

/*----------------------------------------------------------------------*\
 * Boolean
\*----------------------------------------------------------------------*/
class ConfigBool : public ConfigVarT<bool> {
public:
    ConfigBool(const char* name, bool deflt=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, bool* ptr=nullptr, bool (*setCb)(bool val, void* cbData)=nullptr, bool (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigVarT(name, deflt, FST("bool"), info, fmt, group, ptr, setCb, getCb, cbData, flags) {}

    virtual size_t setFromStr(const char* valStr, const char** errorStr=nullptr) {
        bool value = false;
        size_t n = StrTool::readBool(valStr, &value, errorStr);
        if (n) { set(value); }
        return n;
    }
};

/*----------------------------------------------------------------------*\
 * Int32
\*----------------------------------------------------------------------*/
class ConfigInt32 : public ConfigVarT<int32_t> {
public:
    ConfigInt32(const char* name, int32_t deflt=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, int32_t* ptr=nullptr, bool (*setCb)(int32_t val, void* cbData)=nullptr, int32_t (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigVarT(name, deflt, FST("int32"), info, fmt, group, ptr, setCb, getCb, cbData, flags) {}
};

/*----------------------------------------------------------------------*\
 * Unsigned Int32
\*----------------------------------------------------------------------*/
class ConfigUInt32 : public ConfigVarT<uint32_t> {
public:
    ConfigUInt32(const char* name, uint32_t deflt=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, uint32_t* ptr=nullptr, bool (*setCb)(uint32_t val, void* cbData)=nullptr, uint32_t (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigVarT(name, deflt, FST("uint32"), info, fmt, group, ptr, setCb, getCb, cbData, flags) {}
};

/*----------------------------------------------------------------------*\
 * Int16
\*----------------------------------------------------------------------*/
class ConfigInt16 : public ConfigVarT<int16_t> {
public:
    ConfigInt16(const char* name, int16_t deflt=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, int16_t* ptr=nullptr, bool (*setCb)(int16_t val, void* cbData)=nullptr, int16_t (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigVarT(name, deflt, FST("int16"), info, fmt, group, ptr, setCb, getCb, cbData, flags) {}
};

/*----------------------------------------------------------------------*\
 * Unsigned Int16
\*----------------------------------------------------------------------*/
class ConfigUInt16 : public ConfigVarT<uint16_t> {
public:
    ConfigUInt16(const char* name, uint16_t deflt=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, uint16_t* ptr=nullptr, bool (*setCb)(uint16_t val, void* cbData)=nullptr, uint16_t (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigVarT(name, deflt, FST("uint16"), info, fmt, group, ptr, setCb, getCb, cbData, flags) {}
};

/*----------------------------------------------------------------------*\
 * Int8
\*----------------------------------------------------------------------*/
class ConfigInt8 : public ConfigVarT<int8_t> {
public:
    ConfigInt8(const char* name, int8_t deflt=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, int8_t* ptr=nullptr, bool (*setCb)(int8_t val, void* cbData)=nullptr, int8_t (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigVarT(name, deflt, FST("int8"), info, fmt, group, ptr, setCb, getCb, cbData, flags) {}
};

/*----------------------------------------------------------------------*\
 * Unsigned Int8
\*----------------------------------------------------------------------*/
class ConfigUInt8 : public ConfigVarT<uint8_t> {
public:
    ConfigUInt8(const char* name, uint8_t deflt=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, uint8_t* ptr=nullptr, bool (*setCb)(uint8_t val, void* cbData)=nullptr, uint8_t (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigVarT(name, deflt, FST("uint8"), info, fmt, group, ptr, setCb, getCb, cbData, flags) {}
};


/*----------------------------------------------------------------------*\
 * Enum
\*----------------------------------------------------------------------*/
class ConfigEnum : public ConfigVarT<int> {
public:
    typedef struct Option { const char* text; int value; } Option;
    ConfigEnum(const char* name, const Option* map, size_t mapSize, int deflt=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, int* ptr=nullptr, bool (*setCb)(int val, void* cbData)=nullptr, int (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigVarT(name, deflt, FST("enum"), info, fmt, group, ptr, setCb, getCb, cbData, flags), map_(map), mapSize_(mapSize) {}

    virtual bool set(int val) {
        if (getIndex_(val) != ~0) { return ConfigVarT::set(val); }
        DEBUG_printf(FST("ConfigEnum %s invalid value: %d\n"), name_, val);
        return true;
    }

    virtual size_t getWebUi(Print* stream, uint32_t flags=0, RFFlag flagsMask=0) {
        if ((flags_ ^ flags) & flagsMask) { return 0; }
        size_t n = getWebUiCommon_(stream);
        get();
        n += stream->print(FST(",\"V\":"));
        n += stream->print(value_);
        n += stream->print(FST(",\"O\":["));
        for (size_t i=0; i<mapSize_; i++) {
            if (i) { stream->write(','); n++; }
            n += stream->printf(FST("{\"%s\":"), map_[i].text);
            n += stream->print(map_[i].value);
            stream->write('}'); n++;
        }
        stream->write(']'); n++;
        stream->write('}'); n++;
        return n;
    }

    virtual const char* getText() {
        get();
        if (getIndex_(value_) != ~0) { return map_[index_].text; }
        DEBUG_printf(FST("ConfigEnum %s invalid value: %d\n"), name_, value_);
        return FST("???");
    }

protected:
    size_t getIndex_(int val) {
        for (size_t i=0; i<mapSize_; i++) {
            if (val == map_[i].value) { 
                index_ = i;
                return index_;
            }
        }
        return ~0;
    }

    size_t index_;
    const Option* map_;
    const size_t mapSize_;
};


/************************************************************************\
|* Config Variable Template Class for arrays
\************************************************************************/
template <class T> class ConfigArrayT : public RegVar {
public:
    ConfigArrayT(const char* name, size_t size, const T* deflt=nullptr, const char* typeHelp=nullptr, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, T* ptr=nullptr, bool (*setCb)(T* val, void* cbData)=nullptr, void (*getCb)(T* val,void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : RegVar(name, typeHelp, info, fmt, group, flags | RF_IS_CONFIG), size_(size), default_(deflt), ptr_(ptr), setCb_(setCb), getCb_(getCb), cbData_(cbData) {
        if (!fmt) { fmt_ = FST("%d"); }
        value_ = new T[size_];
        if (default_) { for (size_t n=0; n<size_; n++) { value_[n] = default_[n]; } }
        else { for (size_t n=0; n<size_; n++) { value_[n] = 0; } }        
    }
    virtual ~ConfigArrayT() { delete [] value_; }

   virtual inline T* getValueRef() { return value_; }

    virtual const T* get() {
        if (ptr_) { for (size_t n=0; n<size_; n++) { value_[n] = ptr_[n]; } }
        if (getCb_) { getCb_(value_, cbData_); }
        return value_;
    }

    virtual bool set(const T* val) {
        for (size_t n=0; n<size_; n++) { value_[n] = val[n]; }
        if (ptr_) { for (size_t n=0; n<size_; n++) { ptr_[n] = val[n]; } }
        if (setCb_) { return setCb_(value_, cbData_); }
        return false;
    }

    virtual size_t print(Print& stream) {
        size_t n = 0;
        stream.write('['); n++; 
        get();
        for (size_t i=0; i<size_; i++) {
            if (i) { stream.write(','); n++; }
            n += stream.printf(fmt_, value_[i]);
        }
        stream.write(']'); n++; 
        return n;
    }

    virtual size_t toStr(char* buffer, size_t size) {
        size_t n = 0;
        if (n < size-1) { buffer[n++] = '['; }
        get();
        for (size_t i=0; i<size_; i++) {
            if (i && (n < size-1)) { buffer[n++] = ','; }
            n += snprintf(buffer+n, size-n-1, fmt_, value_[i]);
        }
        if (n < size-1) { buffer[n++] = ']'; }
        buffer[n] = '\0';
        return n;
    }

    virtual size_t toJsonStr(char* buffer, size_t size, bool noName=false, uint32_t flags=0, RFFlag flagsMask=0) {
        if ((flags_ ^ flags) & flagsMask) { return 0; }
        size_t n = 0;
        if (!noName) { n += StrTool::toJsonName(buffer+n, size-n, name_); }
        return n + toStr(buffer+n, size-n);
    }

    virtual bool setFromJson(const JsonVariant& jv) {
        if (!jv.is<JsonArray>()) {
            DEBUG_printf(FST("JSON config for \"%s\" is not an array\n"), name_);
            return true;
        }
        JsonArray arr = jv;
        if ( arr.size() > size_ ) {
            DEBUG_printf(FST("JSON config array for \"%s\" is too large:%d space:%d\n"), name_, arr.size(), size_);
            return true;
        }
        size_t n = 0;
        for (JsonVariant v : arr) {
            if (!v.is<JsonInteger>()) {
                DEBUG_printf(FST("JSON config array element for \"%s\" is not an integer\n"), name_);
                return true;
            }
            value_[n++] = v;
       }
       set(value_);
       return false;
    };

    virtual size_t setFromStr(const char* valStr, const char** errorStr=nullptr) {
        size_t n = 0;
        const char* eStr=nullptr;
        if (valStr[n++] != '[') {
            *errorStr = FST("array does not start with [");
            return 0;
        }
        for (size_t i=0; i<size_; i++) {
            if (i) {
                if(valStr[n] == ']') {
                    *errorStr = FST("not enough array values");
                    return 0;
                }
                if(valStr[n++] != ',') {
                    *errorStr = FST("invalid array");
                    return 0;
                }
            }
            while (valStr[n] == ' ') { n++; }
            int32_t value = 0;
            n += StrTool::readInteger(valStr+n, &value, &eStr, true);
            if (eStr) {
                *errorStr = eStr;
                return 0;
            }
            value_[i] = value;
            while (valStr[n] == ' ') { n++; }
        }
        if (valStr[n++] != ']') {
            *errorStr = FST("array does not end with ]");
            return 0;
        }
        if (n) { set(value_); }
        return n;
    }

    virtual void setDefault() {
        if (default_) {
            for (size_t n=0; n<size_; n++) {
                value_[n] = default_[n];
            }
        } else {
            for (size_t n=0; n<size_; n++) { value_[n] = 0; }
        }
        set(value_);
    }

    inline size_t size() { return size_; }
    virtual size_t getWebUi(Print* stream, uint32_t flags=0, RFFlag flagsMask=0) {
        if ((flags_ ^ flags) & flagsMask) { return 0; }
        char buffer[32];
        size_t n = getWebUiCommon_(stream);
        n += stream->print(FST(",\"V\":["));  // Value
        get();
        for (size_t i=0; i<size_; i++) {
            if (i) { stream->write(','); n++; }
             size_t m = snprintf(buffer, sizeof(buffer)-1, fmt_, value_[i]);
             buffer[m] = '\0';
             stream->write(buffer, m);
             n += m;
        }
        stream->write(']'); n++;
        stream->write('}'); n++;
        return n;
    }

protected:
    size_t size_;
    const T* default_;
    T* value_;
    T* ptr_;
    bool (*setCb_)(T* val, void* cbData);
    void (*getCb_)(T* val, void* cbData);
    void* cbData_;
};

/*----------------------------------------------------------------------*\
 * String
\*----------------------------------------------------------------------*/
class ConfigStr : public ConfigArrayT<char> {
public:
    ConfigStr(const char* name, size_t size, const char* deflt=nullptr, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, char* ptr=nullptr, bool (*setCb)(char* val, void* cbData)=nullptr, void (*getCb)(char* val, void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigArrayT(name, size, deflt, FST("str"), info, fmt, group, ptr, setCb, getCb, cbData, flags) {
            if (!fmt) { fmt_ = FST("%s"); }
            if (!default_) { value_[0] = '\0'; }
            value_[size_-1] = '\0';
        }

    virtual size_t print(Print& stream) {
        get();
        return stream.printf(fmt_, value_);
    }
    virtual size_t toStr(char* buffer, size_t size) {
        size_t n = 0;
        get();
        n += snprintf(buffer+n, size-n-1, fmt_, value_);
        buffer[n] = '\0';
        return n;
    }
    virtual size_t toJsonStr(char* buffer, size_t size, bool noName=false, uint32_t flags=0, RFFlag flagsMask=0) {
        if ((flags_ ^ flags) & flagsMask) { return 0; }
        size_t n = 0;
        if (!noName) { n += StrTool::toJsonName(buffer+n, size-n, name_); }
        if (n < size-2) { buffer[n++] = '"'; }
        if (isPassword() && !(flags & RF_SHOW_PASSWORD)) {
            for (size_t i=0; i < 8; i++) { if (n < size-2) buffer[n++] = '*'; }
        } else {
            n += toStr(buffer+n, size-n - 1);
        }
        if (n < size-2) { buffer[n++] = '"'; }
        buffer[n]= '\0';
        return n;
    }

    virtual bool setFromJson(const JsonVariant& jv) {
        if (!jv.is<const char*>()) {
            DEBUG_printf(FST("JSON config for \"%s\" is not string\n"), name_);
            return true;
        }
        set((const char*)jv);
        return false;
    }

    virtual size_t setFromStr(const char* valStr, const char** errorStr=nullptr) {
        size_t n = 0;
        while (valStr[n] && n < size_-1) { value_[n] = valStr[n]; n++; }
        value_[n] = '\0';
        set(value_);
        return n;
    }

    virtual size_t getWebUi(Print* stream, uint32_t flags=0, RFFlag flagsMask=0) {
        if ((flags_ ^ flags) & flagsMask) { return 0; }
        size_t n = getWebUiCommon_(stream);
        get();
        if (isPassword() && !(flags & RF_SHOW_PASSWORD)) {
            n += stream->print(FST(",\"V\":\"********\""));
        } else { n += stream->printf(FST(",\"V\":\"%s\""), value_); } 
        n += stream->printf(FST(",\"S\":%d"), size_-1);  // Max
        n += stream->printf(FST(",\"M\":%d"), 0);  // Min
        stream->write('}'); n++;        
        return n;
    }
};


/*----------------------------------------------------------------------*\
 * Int32 Array
\*----------------------------------------------------------------------*/
class ConfigInt32Array : public ConfigArrayT<int32_t> {
public:
    ConfigInt32Array(const char* name, size_t size, const int32_t* deflt=nullptr, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, int32_t* ptr=nullptr, bool (*setCb)(int32_t* val, void* cbData)=nullptr, void (*getCb)(int32_t* val, void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigArrayT(name, size, deflt, nullptr, info, fmt, group, ptr, setCb, getCb, cbData, flags) {
            sprintf(ths, FST("int32[%d]"), size);
            typeHelp_ = ths;
        }
protected:
    char ths[16];
};

/*----------------------------------------------------------------------*\
 * Unsigned Int32 Array
\*----------------------------------------------------------------------*/
class ConfigUInt32Array : public ConfigArrayT<uint32_t> {
public:
    ConfigUInt32Array(const char* name, size_t size, const uint32_t* deflt=nullptr, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, uint32_t* ptr=nullptr, bool (*setCb)(uint32_t* val, void* cbData)=nullptr, void (*getCb)(uint32_t* val, void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigArrayT(name, size, deflt, nullptr, info, fmt, group, ptr, setCb, getCb, cbData, flags) {
            sprintf(ths, FST("uint32[%d]"), size);
            typeHelp_ = ths;
        }
protected:
    char ths[16];
};

/*----------------------------------------------------------------------*\
 * Int16 Array
\*----------------------------------------------------------------------*/
class ConfigInt16Array : public ConfigArrayT<int16_t> {
public:
    ConfigInt16Array(const char* name, size_t size, const int16_t* deflt=nullptr, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, int16_t* ptr=nullptr, bool (*setCb)(int16_t* val, void* cbData)=nullptr, void (*getCb)(int16_t* val, void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigArrayT(name, size, deflt, nullptr, info, fmt, group, ptr, setCb, getCb, cbData, flags) {
            sprintf(ths, FST("int16[%d]"), size);
            typeHelp_ = ths;
        }
protected:
    char ths[16];
};

/*----------------------------------------------------------------------*\
 * Unsigned Int16 Array
\*----------------------------------------------------------------------*/
class ConfigUInt16Array : public ConfigArrayT<uint16_t> {
public:
    ConfigUInt16Array(const char* name, size_t size, const uint16_t* deflt=nullptr, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, uint16_t* ptr=nullptr, bool (*setCb)(uint16_t* val, void* cbData)=nullptr, void (*getCb)(uint16_t* val, void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigArrayT(name, size, deflt, nullptr, info, fmt, group, ptr, setCb, getCb, cbData, flags) {
            sprintf(ths, FST("uint16[%d]"), size);
            typeHelp_ = ths;
        }
protected:
    char ths[16];
};

/*----------------------------------------------------------------------*\
 * Int8 Array
\*----------------------------------------------------------------------*/
class ConfigInt8Array : public ConfigArrayT<int8_t> {
public:
    ConfigInt8Array(const char* name, size_t size, const int8_t* deflt=nullptr, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, int8_t* ptr=nullptr, bool (*setCb)(int8_t* val, void* cbData)=nullptr, void (*getCb)(int8_t* val, void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigArrayT(name, size, deflt, nullptr, info, fmt, group, ptr, setCb, getCb, cbData, flags) {
            sprintf(ths, FST("int8[%d]"), size);
            typeHelp_ = ths;
        }
protected:
    char ths[16];
};

/*----------------------------------------------------------------------*\
 * Unsigned Int8 Array
\*----------------------------------------------------------------------*/
class ConfigUInt8Array : public ConfigArrayT<uint8_t> {
public:
    ConfigUInt8Array(const char* name, size_t size, const uint8_t* deflt=nullptr, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, uint8_t* ptr=nullptr, bool (*setCb)(uint8_t* val, void* cbData)=nullptr, void (*getCb)(uint8_t* val, void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigArrayT(name, size, deflt, nullptr, fmt, info, group, ptr, setCb, getCb, cbData, flags) {
            sprintf(ths, FST("uint8[%d]"), size);
            typeHelp_ = ths;
        }
protected:
    char ths[16];
};

/*----------------------------------------------------------------------*\
 * IP Address
\*----------------------------------------------------------------------*/
class ConfigIpAddr : public ConfigArrayT<uint8_t> {
public:
    ConfigIpAddr(const char* name, const uint8_t* deflt=nullptr, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, uint8_t* ptr=nullptr, bool (*setCb)(uint8_t* val, void* cbData)=nullptr, void (*getCb)(uint8_t* val, void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigArrayT(name, 4, deflt, FST("IP_ADR"), info, fmt, group, ptr, setCb, getCb, cbData, flags) {
            if (!fmt) { fmt_ = FST("%d.%d.%d.%d"); }
        }

    /* ConfigIpAddr(const char* name, uint32_t deflt=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, uint8_t* ptr=nullptr, bool (*setCb)(uint8_t* val, void* cbData)=nullptr, void (*getCb)(uint8_t* val, void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : ConfigArrayT(name, 4, (uint8_t*)&deflt, FST("IP_ADR"), info, fmt, group, ptr, setCb, getCb, cbData, flags) {
            if (!fmt) { fmt_ = FST("%d.%d.%d.%d"); }
        } */

    virtual size_t print(Print& stream) {
        get();
        return stream.printf(fmt_, value_[0], value_[1], value_[2], value_[3]);
    }

    virtual size_t toStr(char* buffer, size_t size) {
        size_t n = 0;
        get();
        n += snprintf(buffer+n, size-n-1, fmt_, value_[0], value_[1], value_[2], value_[3]);
        buffer[n] = '\0';
        return n;
    }
    virtual size_t toJsonStr(char* buffer, size_t size, bool noName=false, uint32_t flags=0, RFFlag flagsMask=0) {
        if ((flags_ ^ flags) & flagsMask) { return 0; }
        size_t n = 0;
        if (!noName) { n += StrTool::toJsonName(buffer+n, size-n, name_); }
        if (n < size-2) { buffer[n++]= '"'; }
        n += toStr(buffer+n, size-n - 1);
        if (n < size-2) { buffer[n++]= '"'; }
        buffer[n]= '\0';
        return n;
    }
    virtual size_t setFromStr(const char* valStr, const char** errorStr=nullptr) {
        uint8_t ip[4] = {0};
        size_t n = StrTool::readIpAddr(valStr, ip, errorStr);
        if (n) { set(ip); }
        return n;
    }
    virtual bool setFromJson(const JsonVariant& jv) {
        if (jv.is<JsonArray>()) { return ConfigArrayT::setFromJson(jv); }
        if (!jv.is<const char*>()) {
            DEBUG_printf(FST("JSON config for \"%s\" is not a string or array\n"), name_);
            return true;
        }
        if (!setFromStr((const char*)jv)) {
            DEBUG_printf(FST("JSON config for \"%s\" is not a valid IP address\n"), name_);
            return true;
        }
        return false;
    }
    virtual size_t getWebUi(Print* stream, uint32_t flags=0, RFFlag flagsMask=0) {
        if ((flags_ ^ flags) & flagsMask) { return 0; }
        size_t n = getWebUiCommon_(stream);
        get();
        n += stream->printf(FST(",\"V\":\"%d.%d.%d.%d\""), value_[0], value_[1], value_[2], value_[3]);  // Value
        stream->write('}'); n++;        
        return n;
    }

    inline uint32_t getUInt32() { return *((uint32_t*)value_); }
    inline bool isSet() { return getUInt32() != 0; }
};


#endif // _CONFIG_REG_H_

