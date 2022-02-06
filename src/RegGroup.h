/*======================================================================*\
|* Status Variable Registry
|*
|* This framework provides a hierarchical registry for statuts
|* variables. Each variable can register change callbacks.
\*======================================================================*/

#ifndef _REG_GROUP_H_
#define _REG_GROUP_H_

#include <Arduino.h>
#include <vector>
#include <iterator>
#include <cstddef>

#include "VUEF.h"
#include "Helper.h"

// Disable warning about char being deprecated. Not sure why it's needed...
#ifndef ARDUINOJSON_DEPRECATED
#define ARDUINOJSON_DEPRECATED(msg)
#endif
#include <ArduinoJson.h>

/************************************************************************\
|* Global Functions
\************************************************************************/


/************************************************************************\
|* Flags
\************************************************************************/

typedef uint16_t RFFlag;
static const RFFlag RF_IS_CONFIG =     (uint8_t)(1<<0);
static const RFFlag RF_IS_STATE =      (uint8_t)(1<<1);
static const RFFlag RF_HIDDEN =        (uint8_t)(1<<2);
static const RFFlag RF_NOT_PERSISTED = (uint8_t)(1<<3);
static const RFFlag RF_READ_ONLY =     (uint8_t)(1<<4);
static const RFFlag RF_PASSWORD =      (uint8_t)(1<<5);
static const RFFlag RF_WIZARD =        (uint8_t)(1<<6);
static const RFFlag RF_CONTROL_UI =    (uint8_t)(1<<7);

static const uint32_t RF_SHOW_PASSWORD = (uint32_t)(1<<16);

/*
const char WUI_CATEGORY[] PROGMEM = "C";
const char WUI_PARAMETER[] PROGMEM = "P";
const char WUI_LABEL[] PROGMEM = "L";
const char WUI_HELP[] PROGMEM = "H";
const char WUI_TYPE[] PROGMEM = "T";
const char WUI_VALUE[] PROGMEM = "V";
const char WUI_MAX[] PROGMEM = "S";
const char WUI_MIN[] PROGMEM = "M";
const char WUI_OPTIONS[] PROGMEM = "O";
const char WUI_FLAGS[] PROGMEM = "F";
*/

/************************************************************************\
|* RegGroup organizes Variables in a hierarchical tree
\************************************************************************/

class RegVar;
class RegGroup {
public:
  typedef void (*CallbackFunc)(RegGroup& group, void* data);
  typedef struct Callback {
      CallbackFunc callback;
      void* data;
  } Callback;

    /*------------------------------------------------------------------*\
     * Recursive iterator over all the (child) variables of this group.
    \*------------------------------------------------------------------*/
    class Iterator 
    {
        public: 
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = RegVar&;
        using pointer           = RegVar*;  // or also value_type*
        using reference         = RegVar&;  // or also value_type&

        Iterator(RegGroup& group, size_t index) : group_(group), index_(index) {
            varP_ = index < group_.size() ?  group_.get(index_) : nullptr;
        }

        reference operator*() const { return *varP_; }
        pointer operator->() { return varP_; }

        // Prefix increment
        Iterator& operator++() { next_(); return *this; }  

        // Postfix increment
        Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

        friend bool operator== (const Iterator& a, const Iterator& b) { return a.index_ == b.index_; };
        friend bool operator!= (const Iterator& a, const Iterator& b) { return a.index_ != b.index_; };     


        size_t getVarName(char* buffer, size_t size) {
            return group_.getVarName(buffer, size, index_);
        }

        protected:
        void next_() {
            ++index_;
            varP_ = index_ < group_.size() ?  group_.get(index_) : nullptr;
        }
        RegGroup& group_;
        size_t index_;
        RegVar* varP_;
    };


    /*------------------------------------------------------------------*\
     * Registry Group
    \*------------------------------------------------------------------*/
    RegGroup(const char* name, RegGroup* parent=nullptr, const char* info=nullptr, RFFlag flags=0) 
      : name_(name), parent_(parent), info_(info), flags_(flags), configCount_(0), stateCount_(0) {
        checkMain();
        if (parent_ == nullptr) { parent_ = mainGroup; }
        if (parent_ && parent_ != (RegGroup*)-1) { parent_->addChild(this); }
    }
    ~RegGroup() {
        if(parent_) {
            parent_->removeChild(this);
            parent_ = nullptr;
        }
    }
    void addChild(RegGroup* child) { children_.push_back(child); updateConfigCount_(child->configCount_); updateStateCount_(child->stateCount_);  }
    void removeChild(RegGroup* child) { children_.erase(remove(children_.begin(), children_.end(), child), children_.end()); updateConfigCount_(-child->configCount_); updateStateCount_(-child->stateCount_); }
    RegGroup* findChild(const char* name);
    void addVar(RegVar* var);
    void removeVar(RegVar* var);
    RegVar* findVar(const char* name);
    RegVar* findVarByFullName(const char* name, bool matchCase=true);
    size_t toJsonStr(char* buffer, size_t size, bool noName=false, uint32_t flags=0, RFFlag flagsMask=0);
    size_t toJson(Print* stream, bool noName=false, uint32_t flags=0, RFFlag flagsMask=0);
    size_t getWebUi(Print* stream, bool noName=false, uint32_t flags=0, RFFlag flagsMask=0);
    bool setFromJson(const JsonObject& obj);
    void setDefaults();
    inline const char* name() { return name_; }
    size_t getVarName(char* buffer, size_t size, size_t index);
    inline size_t size() { return stateCount_ + configCount_; }
    inline size_t childrenSize() { return size() - vars_.size(); }
    inline std::vector<RegGroup*>& children() { return children_; }
    inline std::vector<RegVar*>& vars() { return vars_; }
    inline const char* info() { return info_; }
    inline RFFlag flags() { return flags_; }
    inline bool isHidden() { return flags_ & RF_HIDDEN; }   
    inline bool isNotPersisted() { return flags_ & RF_NOT_PERSISTED; }   
    RegVar* get(size_t n);
    std::vector<RegVar*>::iterator getIt(size_t n);
    Iterator begin() { return Iterator(*this, 0); }
    Iterator end()   { return Iterator(*this, size()); } 

    inline bool isChanged() { return isChanged_; } 
    void addChangeCallback(CallbackFunc cb, void* data=nullptr) { changeCallbacks_.push_back({cb, data}); }
    void removeChangeCallback(CallbackFunc cb, void* data) {
        changeCallbacks_.erase(
            std::remove_if(changeCallbacks_.begin(), changeCallbacks_.end(), [&](Callback const & c) {
                return c.callback == cb && c.data == data;}), changeCallbacks_.end());
    }
    bool checkChange();
    size_t printChangeJson(Print& out, char* namePrefix, size_t npSize, size_t npEnd=~0, size_t count=0);

    static RegGroup* mainGroup;
    static void checkMain() {
        if (!mainGroup) {
            mainGroup = (RegGroup*)-1; // Dummy value. Prevent infinite recursion
            mainGroup = new RegGroup(FST("main"), (RegGroup*)-1);
        }
    }

protected:
    size_t updateConfigCount_(size_t n) {
        configCount_ += n;
        if (configCount_) { flags_ |= RF_IS_CONFIG; }
        else { flags_ &= ~RF_IS_CONFIG; }
        if (parent_ && parent_ != (RegGroup*)-1) { parent_->updateConfigCount_(n); }
        return configCount_;
    }
    size_t updateStateCount_(size_t n) {
        stateCount_ += n;
        if (stateCount_) { flags_ |= RF_IS_STATE; }
        else { flags_ &= ~RF_IS_STATE; }
        if (parent_ && parent_ != (RegGroup*)-1) { parent_->updateStateCount_(n); }
        return stateCount_;
    }

    const char* name_;
    RegGroup* parent_;
    const char* info_;
    std::vector<RegGroup*> children_;
    std::vector<RegVar*> vars_;
    std::vector<Callback> changeCallbacks_;
    bool isChanged_;
    RFFlag flags_;
    size_t configCount_;
    size_t stateCount_;
};

/************************************************************************\
|* Registry Variable Base Class
\************************************************************************/
class RegVar {
public:
  RegVar(const char* name, const char* typeHelp=nullptr, const char* info=nullptr, const char* fmt=nullptr, RegGroup* group=nullptr, RFFlag flags=0)
    : name_(name), fmt_(fmt), typeHelp_(typeHelp), info_(info), group_(group), flags_(flags), id_(count_++) {
        if (!typeHelp_) { typeHelp_ = FST("value"); }
        if (!info_) { info_ = FST(""); }
        if (!group_) { 
            RegGroup::checkMain();
            group_ = RegGroup::mainGroup; 
        }
        group_->addVar(this);
    }
    virtual ~RegVar() {
        if (group_) {
            group_->removeVar(this);
            group_ = nullptr;
        }

    }
    virtual size_t print(Print& stream) = 0;
    virtual void printChangeJson(Print& out, char* namePrefix=nullptr, size_t npSize=0, size_t npEnd=0) {};
    virtual size_t toStr(char* buffer, size_t size) = 0;
    virtual size_t toJsonStr(char* buffer, size_t size, bool noName=false, uint32_t flags=0, RFFlag flagsMask=0) = 0;
    virtual bool setFromJson(const JsonVariant& jv) { return false; } ;
    virtual size_t setFromStr(const char* valStr, const char** errorStr=nullptr) { return 0; };
    virtual size_t getWebUi(Print* stream, uint32_t flags=0, RFFlag flagsMask=0) = 0;
    virtual bool checkChange() { return false; };
    virtual void setDefault() {};
    inline const char* name() { return name_; }
    inline const char* fmt() { return fmt_; }
    inline const char* typeHelp() { return typeHelp_; }
    inline const char* info() { return info_; }
    inline bool isChanged() { return isChanged_; } 
    inline RFFlag flags() { return flags_; }
    inline bool isHidden() { return flags_ & RF_HIDDEN; }   
    inline bool isNotPersisted() { return flags_ & RF_NOT_PERSISTED; }   
    inline bool isReadOnly() { return flags_ & RF_READ_ONLY; }   
    inline bool isPassword() { return flags_ & RF_PASSWORD; }
    inline size_t id() { return id_; }   

protected:
    virtual size_t getWebUiCommon_(Print* stream) {
        size_t n = 0;
        char buffer[64];
        StrTool::toCleanName(buffer, sizeof(buffer), name_);
        n += stream->printf(FST("{\"P\":\"%s\""), buffer); // Parameter Name
        n += stream->printf(FST(",\"L\":\"%s\""), name_);  // Label
        if (typeHelp_ && typeHelp_[0]) n += stream->printf(FST(",\"T\":\"%s\""), typeHelp_);  // Type
        if (info_) {n += stream->printf(FST(",\"H\":\"%s\""), info_); } // Help
        if (flags_) {n += stream->printf(FST(",\"F\":%d"), flags_); }
        return n;
    }

    const char* name_;
    const char* fmt_;
    const char* typeHelp_;
    const char* info_;
    RegGroup* group_;
    RFFlag flags_;
    bool isChanged_;
    size_t id_;

    static size_t count_;
};

#endif // _GROUP_REG_H_

