/*======================================================================*\
|* Status Variable Registry
|*
|* This framework provides a hierarchical registry for statuts
|* variables. Each variable can register change callbacks.
\*======================================================================*/

#include <Arduino.h>
#include "VUEF.h"
#include "StateReg.h"
#include "Helper.h"
#include "Command.h"

#if ENABLE_STATE_REG


/************************************************************************\
|* Config Commands
\************************************************************************/

#if ENABLE_CLI
CommandRegistry cmdRegState(FST("state"));
const char NO_STATE_VARS[] PROGMEM = "No state variables defined";

Command cmdGetState(FST("get"), 
[] (const char* args, Print* stream) {
    if (!RegGroup::mainGroup) { stream->println(NO_STATE_VARS); return EC_ERROR; }
    RegGroup::mainGroup->toJson(stream, true, RF_IS_STATE, RF_IS_STATE);
    stream->println();
    return EC_OK;
},
FST("Get state as JSON"), &cmdRegState,
nullptr, CT_APP_JSON
);

Command cmdStateVar(FST("var"), 
[] (const char* args, Print* stream) {
    if (!RegGroup::mainGroup) { stream->println(NO_STATE_VARS); return EC_ERROR; }
    RegVar* var = RegGroup::mainGroup->findVarByFullName(args, false);
    if (var) {
        if (stream) { var->print(*stream); }
    } else {
         if (stream) { stream->print(FST("unknown variable")); }
         return EC_NOT_FOUND;
    }
    if (stream) { stream->println(); }
    return EC_OK;
},
FST("Get state variable"), &cmdRegState, FST("<name>"), CT_APP_JSON
);

Command cmdGetStateUi(FST("ui"), 
[] (const char* args, Print* stream) {
    if (!RegGroup::mainGroup) { stream->println(NO_STATE_VARS); return EC_ERROR; }
    RegGroup::mainGroup->getWebUi(stream, true, RF_IS_STATE, RF_IS_STATE);
    stream->println();
    return EC_OK;
},
FST("Get state UI as JSON"), &cmdRegState,
nullptr, CT_APP_JSON
);

#endif // ENABLE_CLI


/************************************************************************\
|* Global Functions
\************************************************************************/

bool stateRegRun(uint32_t now) {
    static uint32_t stateRegNextTs = 0;
    if (!now) { now = millis(); }
    if (now < stateRegNextTs || !RegGroup::mainGroup) { return false; }
    stateRegNextTs = now + STATE_REG_CHECK_MS;
    if (!RegGroup::mainGroup->checkChange()) {
        return false;
    }
    return true;
}

/************************************************************************\
|* State Group
\************************************************************************/

bool RegGroup::checkChange() {
    isChanged_ = false;
    for(auto v: vars_) { isChanged_ |= v->checkChange(); }
    for(auto g: children_) { isChanged_ |= ((RegGroup*) g)->checkChange(); }
    if (isChanged_) {
        for(auto cb: changeCallbacks_) { cb.callback(*this, cb.data); }
    }
    return isChanged_;
}

size_t RegGroup::printChangeJson(Print& out, char* namePrefix, size_t npSize, size_t npEnd/*=~0*/, size_t count/*=0*/) {
    if (!isChanged_) { return 0; }
    size_t n = 0;
    bool isOuter = false;
    if (npEnd == ~0) { 
        npEnd = 0; 
        isOuter = true;
        out.write('{'); 
    } 
    else {
        while (name_[n] && npEnd < npSize -2) { namePrefix[npEnd++] = name_[n++]; }
        if (npEnd < npSize -2) { namePrefix[npEnd++] = '.'; }
    }
    namePrefix[npEnd] = '\0';
    for(auto v: vars_) { 
        if(v->isChanged()) { 
            if (count) { out.write(','); }
            v->printChangeJson(out, namePrefix, npSize, npEnd);
            count++;
        }
    }
    for(auto g: children_) { 
        count += ((RegGroup*)g)->printChangeJson(out, namePrefix, npSize, npEnd, count); 
    }
    if (isOuter) { out.write('}'); }
    return count;
}
#endif // ENABLE_STATE_REG
