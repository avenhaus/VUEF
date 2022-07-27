/*======================================================================*\
|* Status Variable Registry
|*
|* This framework provides a hierarchical registry for statuts
|* variables. Each variable can register change callbacks.
\*======================================================================*/

#ifndef _STATE_REG_H_
#define _STATE_REG_H_

#if ENABLE_STATE_REG
#include <Arduino.h>
#include <vector>
#include <iterator>
#include <cstddef>

#include "VUEF.h"
#include "RegGroup.h"
#include "Helper.h"

// Disable warning about char being deprecated. Not sure why it's needed...
#ifndef ARDUINOJSON_DEPRECATED
#define ARDUINOJSON_DEPRECATED(msg)
#endif
#include <ArduinoJson.h>


#ifndef STATE_REG_CHECK_MS
#define STATE_REG_CHECK_MS 100
#endif


/************************************************************************\
|* Global Functions
\************************************************************************/

bool stateRegRun(uint32_t now=0);



/************************************************************************\
|* State Variable Template Class for individual values
\************************************************************************/
template <class T> class StateVarT : public RegVar {
public:
    typedef void (*CallbackFunc)(RegVar& var, T value, void* data);
    typedef struct Callback {
        CallbackFunc callback;
        void* data;
    } Callback;
    StateVarT(const char* name, const T value=0, const char* typeHelp=nullptr, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, T* ptr=nullptr, T (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : RegVar(name, typeHelp, info, fmt, group, flags | RF_IS_STATE), value_(value), oldValue_(value), ptr_(ptr), getCb_(getCb), cbData_(cbData), callbacks_(nullptr) {
        if (!fmt) { fmt_ = FST("%d");  }
    }
    virtual ~StateVarT() {
        if (callbacks_) {
            delete callbacks_; callbacks_ = nullptr;
        }
    }

    virtual inline T* getValueRef() { return &value_; }

    virtual T get() {
        if (ptr_) { value_ = *ptr_; }
        if (getCb_) { value_ = getCb_(cbData_); }
        return value_;
    }

    inline T set(T val) { value_ = val; return value_; }
    inline T value() { return value_; }

    virtual size_t print(Print& stream) {
        return stream.printf(fmt_, get());
    }

    virtual void printChangeJson(Print& out, char* namePrefix=nullptr, size_t npSize=0, size_t npEnd=0) {
        size_t n = 0;
        while (name_[n] && npEnd < npSize -2) { namePrefix[npEnd++] = name_[n++]; }
        namePrefix[npEnd] = '\0';
        char buffer[128];
        n = StrTool::toJsonName(buffer, sizeof(buffer), namePrefix);
        out.write(buffer, n); 
        out.print(oldValue_);
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

    virtual size_t getWebUi(Print* stream, uint32_t flags=0, RFFlag flagsMask=0) {
        if ((flags_ ^ flags) & flagsMask) { return 0; }
        size_t n = getWebUiCommon_(stream);
        get();
        n += stream->print(FST(",\"V\":"));
        n += stream->print(value_);
        stream->write('}'); n++;
        return n;
    }

    virtual void addCallback(CallbackFunc cb, void* data=nullptr) {
        if (!callbacks_) {
            callbacks_ = new std::vector<Callback>;
        }
        callbacks_->push_back({cb, data});
    }

    virtual void removeCallback(CallbackFunc cb, void* data) {
        if (!callbacks_) { return; }
        callbacks_->erase(
            std::remove_if(callbacks_->begin(), callbacks_->end(), [&](Callback const & c) {
            return c.callback == cb && c.data == data;
            }),
        callbacks_->end());
    }

    virtual bool checkChange() {
        get();
        if (oldValue_ == value_) {
            isChanged_ = false;
            return false;
        }
        oldValue_ = value_;
        isChanged_ = true;
        if (callbacks_) {
            for(auto cb: *callbacks_) { cb.callback(*this, value_, cb.data); }
        }
        return true;
    }

protected:
    T value_;
    T oldValue_;
    T* ptr_;
    T (*getCb_)(void* cbData);
    void* cbData_;
    std::vector<Callback>* callbacks_;
};

/*----------------------------------------------------------------------*\
 * Boolean
\*----------------------------------------------------------------------*/
class StateBool : public StateVarT<bool> {
public:
    StateBool(const char* name, bool value=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, bool* ptr=nullptr, bool (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : StateVarT(name, value, FST("bool"), info, fmt, group, ptr, getCb, cbData, flags) {}
};

/*----------------------------------------------------------------------*\
 * Double
\*----------------------------------------------------------------------*/
class StateDouble : public StateVarT<double> {
public:
    StateDouble(const char* name, double value=0.0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, double* ptr=nullptr, double (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : StateVarT(name, value, FST("double"), info, fmt, group, ptr, getCb, cbData, flags) {
            if (!fmt) { fmt_ = FST("%f"); }
    }
};


/*----------------------------------------------------------------------*\
 * Float
\*----------------------------------------------------------------------*/
class StateFloat : public StateVarT<float> {
public:
    StateFloat(const char* name, float value=0.0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, float* ptr=nullptr, float (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : StateVarT(name, value, FST("float"), info, fmt, group, ptr, getCb, cbData, flags) {
            if (!fmt) { fmt_ = FST("%f"); }
    }
};


/*----------------------------------------------------------------------*\
 * Int32
\*----------------------------------------------------------------------*/
class StateInt32 : public StateVarT<int32_t> {
public:
    StateInt32(const char* name, int32_t value=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, int32_t* ptr=nullptr, int32_t (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : StateVarT(name, value, FST("int32"), info, fmt, group, ptr, getCb, cbData, flags) {}
};

/*----------------------------------------------------------------------*\
 * Unsigned Int32
\*----------------------------------------------------------------------*/
class StateUInt32 : public StateVarT<uint32_t> {
public:
    StateUInt32(const char* name, uint32_t value=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, uint32_t* ptr=nullptr, uint32_t (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : StateVarT(name, value, FST("uint32"), info, fmt, group, ptr, getCb, cbData, flags) {}
};

/*----------------------------------------------------------------------*\
 * Int16
\*----------------------------------------------------------------------*/
class StateInt16 : public StateVarT<int16_t> {
public:
    StateInt16(const char* name, int16_t value=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, int16_t* ptr=nullptr, int16_t (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : StateVarT(name, value, FST("int16"), info, fmt, group, ptr, getCb, cbData, flags) {}
};

/*----------------------------------------------------------------------*\
 * Unsigned Int16
\*----------------------------------------------------------------------*/
class StateUInt16 : public StateVarT<uint16_t> {
public:
    StateUInt16(const char* name, uint16_t value=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, uint16_t* ptr=nullptr, uint16_t (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : StateVarT(name, value, FST("uint16"), info, fmt, group, ptr, getCb, cbData, flags) {}
};

/*----------------------------------------------------------------------*\
 * Int8
\*----------------------------------------------------------------------*/
class StateInt8 : public StateVarT<int8_t> {
public:
    StateInt8(const char* name, int8_t value=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, int8_t* ptr=nullptr, int8_t (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : StateVarT(name, value, FST("int8"), info, fmt, group, ptr, getCb, cbData, flags) {}
};

/*----------------------------------------------------------------------*\
 * Unsigned Int8
\*----------------------------------------------------------------------*/
class StateUInt8 : public StateVarT<uint8_t> {
public:
    StateUInt8(const char* name, uint8_t value=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, uint8_t* ptr=nullptr, uint8_t (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : StateVarT(name, value, FST("uint8"), info, fmt, group, ptr, getCb, cbData, flags) {}
};


/*----------------------------------------------------------------------*\
 * Enum
\*----------------------------------------------------------------------*/
class StateEnum : public StateVarT<int> {
public:
    typedef struct Option { const char* text; int value; } Option;
    StateEnum(const char* name, const Option* map, size_t mapSize, int value=0, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, int* ptr=nullptr, int (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : StateVarT(name, value, FST("enum"), info, fmt, group, ptr, getCb, cbData, flags), map_(map), mapSize_(mapSize) {}

    virtual bool set(int val) {
        if (getIndex_(val) != ~0) { return StateVarT::set(val); }
        DEBUG_printf(FST("StateEnum %s invalid value: %d\n"), name_, val);
        return true;
    }

    virtual size_t getWebUi(Print* stream, uint32_t flags=0, RFFlag flagsMask=0) {
        if ((flags_ ^ flags) & flagsMask) { return 0; }
        size_t n = getWebUiCommon_(stream);
        get();
        n += stream->print(FST(",\"V\":"));
        n += stream->print(value_);
        n += stream->print(FST(",\"E\":\""));
        if (getIndex_(value_) != ~0) { n += stream->print(map_[index_].text); }
        else {
            DEBUG_printf(FST("StateEnum %s invalid value: %d\n"), name_, value_);
            n += stream->print(FST("???"));
        }
        stream->write('"'); n++;
        stream->write('}'); n++;
        return n;
    }

    virtual const char* getText() {
        get();
        if (getIndex_(value_) != ~0) { return map_[index_].text; }
        DEBUG_printf(FST("StateEnum %s invalid value: %d\n"), name_, value_);
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

/*----------------------------------------------------------------------*\
 * String
\*----------------------------------------------------------------------*/
class StateStr : public StateVarT<char*> {
public:
    StateStr(const char* name, const char* value=nullptr, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, char** ptr=nullptr, char* (*getCb)(void* cbData)=nullptr, void* cbData=nullptr, RFFlag flags=0)
        : StateVarT(name, (char*)value, FST("str"), info, fmt, group, ptr, getCb, cbData, flags) {
            if (!fmt) { fmt_ = FST("%s"); }
            //get();
            crc_ = StrTool::calculateStrCrc(value_);
        }

    virtual size_t print(Print& stream) {
        get();
        return stream.printf(fmt_, value_);
    }

    virtual void printChangeJson(Print& out, char* namePrefix=nullptr, size_t npSize=0, size_t npEnd=0) {
        size_t n = 0;
        while (name_[n] && npEnd < npSize -2) { namePrefix[npEnd++] = name_[n++]; }
        namePrefix[npEnd] = '\0';
        char buffer[128];
        n = StrTool::toJsonName(buffer, sizeof(buffer), namePrefix);
        out.write(buffer, n); 
        out.write('"');
        out.print(oldValue_);
        out.write('"');
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
        n += toStr(buffer+n, size-n - 1);
        if (n < size-2) { buffer[n++] = '"'; }
        buffer[n]= '\0';
        return n;
    }

    virtual size_t getWebUi(Print* stream, uint32_t flags=0, RFFlag flagsMask=0) {
        if ((flags_ ^ flags) & flagsMask) { return 0; }
        size_t n = getWebUiCommon_(stream);
        get();
        n += stream->printf(FST(",\"V\":\"%s\""), value_);
        // n += stream->printf(FST(",\"S\":%d"), size_-1);  // Max
        stream->write('}'); n++;        
        return n;
    }

    virtual bool checkChange() {
        get();
        uint32_t crc = StrTool::calculateStrCrc(value_);
        if (crc == crc_ && oldValue_ == value_) {
            isChanged_ = false;
            return false;
        }
        oldValue_ = value_;
        crc_ = crc;
        isChanged_ = true;
        if (callbacks_) {
            for(auto cb: *callbacks_) { cb.callback(*this, value_, cb.data); }
        }
        return true;
    }

    inline const char* set(const char* val) { value_ = (char*) val; return value_; }


protected: 
    uint32_t crc_;    
};

#endif // ENABLE_STATE_REG
#endif // _STATE_REG_H_

