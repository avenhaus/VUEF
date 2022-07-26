/*======================================================================*\
|* Command Registry
|*
|* This framework provides a hierarchical registry for commands. 
\*======================================================================*/

#ifndef _COMMAND_H_
#define _COMMAND_H_

#include <Arduino.h>
#include <vector>
#include <iterator>
#include <cstddef>

#include "VUEF.h"

extern const char CT_TEXT_PLAIN[] PROGMEM;
extern const char CT_APP_JSON[] PROGMEM;

#if ENABLE_CLI
static const uint8_t CMDF_HIDDEN = (uint8_t)(1<<0); 
static const uint8_t CMDF_NEEDS_AUTH = (uint8_t)(7<<0); 

/************************************************************************\
|* Global Functions
\************************************************************************/
void findCommand();


/************************************************************************\
|* Command Registry organizes commands in a hierarchical tree
\************************************************************************/

class Command;
class CommandRegistry {
public:

    /*------------------------------------------------------------------*\
     * Recursive iterator over all the (child) commands of this registry.
    \*------------------------------------------------------------------*/
    class Iterator 
    {
        public: 
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = Command&;
        using pointer           = Command*;  // or also value_type*
        using reference         = Command&;  // or also value_type&

        Iterator(CommandRegistry& reg, size_t index) : reg_(reg), index_(index) {
            cmdP_ = index < reg_.size() ?  reg_.get(index_) : nullptr;
        }

        reference operator*() const { return *cmdP_; }
        pointer operator->() { return cmdP_; }

        // Prefix increment
        Iterator& operator++() { next_(); return *this; }  

        // Postfix increment
        Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

        friend bool operator== (const Iterator& a, const Iterator& b) { return a.index_ == b.index_; };
        friend bool operator!= (const Iterator& a, const Iterator& b) { return a.index_ != b.index_; };     


        size_t getCmdName(char* buffer, size_t size) {
            return reg_.getCmdName(buffer, size, index_);
        }

        protected:
        void next_() {
            ++index_;
            cmdP_ = index_ < reg_.size() ?  reg_.get(index_) : nullptr;
        }
        CommandRegistry& reg_;
        size_t index_;
        Command* cmdP_;
    };


    /*------------------------------------------------------------------*\
     * Command Registry
    \*------------------------------------------------------------------*/
    CommandRegistry(const char* name, CommandRegistry* parent=nullptr, const char* info=nullptr, uint8_t flags=0) 
      : name_(name), parent_(parent), info_(info), flags_(flags), cmdCount_(0) {
        checkMain();
        if (parent_ == nullptr) { parent_ = mainCmdReg; }
        if (parent_ && parent_ != (CommandRegistry*)-1) { parent_->addChild(this); }
    }
    ~CommandRegistry() {
        if(parent_) {
            parent_->removeChild(this);
            parent_ = nullptr;
        }
    }
    void addChild(CommandRegistry* child) { children_.push_back(child); updateCmdCount_(child->size()); }
    void removeChild(CommandRegistry* child) { remove(children_.begin(), children_.end(), child);  updateCmdCount_(-child->size()); }
    CommandRegistry* findChild(const char* name);
    void addCmd(Command* cmd) { cmds_.push_back(cmd); updateCmdCount_(1); }
    void removeCmd(Command* cmd) { size_t tmp=cmds_.size(); remove(cmds_.begin(), cmds_.end(), cmd); updateCmdCount_(cmds_.size() - tmp); }
    Command* findCmd(const char* name);
    Command* findCmdByFullName(const char* name, bool matchCase=true);
    inline const char* name() { return name_; }
    size_t getCmdName(char* buffer, size_t size, size_t index);
    inline size_t size() { return cmdCount_; }
    inline size_t childrenSize() { return cmdCount_ - cmds_.size(); }
    inline std::vector<CommandRegistry*>& children() { return children_; }
    inline std::vector<Command*>& cmds() { return cmds_; }
    inline const char* info() { return info_; }
    inline uint8_t flags() { return flags_; }
    inline bool isHidden() { return flags_ & CMDF_HIDDEN; }   
    Command* get(size_t n);
    std::vector<Command*>::iterator getIt(size_t n);
    Iterator begin() { return Iterator(*this, 0); }
    Iterator end()   { return Iterator(*this, size()); } 

    static CommandRegistry* mainCmdReg;
    static void checkMain() {
        if (!mainCmdReg) {
            mainCmdReg = (CommandRegistry*)-1; // Dummy value. Prevent infinite recursion
            mainCmdReg = new CommandRegistry(FST("main"), (CommandRegistry*)-1);
        }
    }

protected:
    size_t updateCmdCount_(size_t n) {
        cmdCount_ += n;
        if (parent_ && parent_ != (CommandRegistry*)-1) { parent_->updateCmdCount_(n); }
        return cmdCount_;
    }

    const char* name_;
    CommandRegistry* parent_;
    const char* info_;
    std::vector<CommandRegistry*> children_;
    std::vector<Command*> cmds_;
    uint8_t flags_;
    size_t cmdCount_;
};

/************************************************************************\
|* Command Class
\************************************************************************/
class Command {
public:
  typedef ErrorCode (*CmdFct)(const char* args, Print* s);

  Command(const char* name, CmdFct fct, const char* info=nullptr, CommandRegistry* reg=nullptr, const char* inputHelp=nullptr, const char* contentType=nullptr, uint8_t flags=0)
    : name_(name), fct_(fct), info_(info), reg_(reg), inputHelp_(inputHelp), contentType_(contentType), flags_(flags) {
        if (!info_) { info_ = EMPTY_STRING; }
        if (!inputHelp_) { inputHelp_ = EMPTY_STRING; }
        if (!contentType_) { contentType_ = CT_TEXT_PLAIN; }
        if (!reg_) { 
            CommandRegistry::checkMain();
            reg_ = CommandRegistry::mainCmdReg; 
        }
        reg_->addCmd(this);
    }
    virtual ~Command() {
        if (reg_) {
            reg_->removeCmd(this);
            reg_ = nullptr;
        }

    }
    inline const char* name() { return name_; }
    inline const char* info() { return info_; }
    inline const char* inputHelp() { return inputHelp_; }
    inline const char* contentType() { return contentType_; }
    inline uint8_t flags() { return flags_; }
    inline bool isHidden() { return flags_ & CMDF_HIDDEN; }   
    inline ErrorCode execute(const char* args, Print* s) { return (*fct_)(args, s); }

protected:
    const char* name_;
    CmdFct fct_;
    const char* info_;
    CommandRegistry* reg_;
    const char* inputHelp_;
    const char* contentType_;
    uint8_t flags_;
};

#endif // ENABLE_CLI

#endif // _COMMAND_H_

