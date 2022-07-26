#ifndef VUEF_CONF_INT_H
#define VUEF_CONF_INT_H

#if defined(VUEF_CONF_PATH)
#define __VUEF_TO_STR_AUX(x) #x
#define __VUEF_TO_STR(x) __VUEF_TO_STR_AUX(x)
#include __VUEF_TO_STR(VUEF_CONF_PATH)
#undef __VUEF_TO_STR_AUX
#undef __VUEF_TO_STR
#elif defined(VUEF_CONF_INCLUDE_SIMPLE)
#include "VuefConf.h"
#else
#include "../VuefConf.h"
#endif

#ifndef ENABLE_WIFI
#define ENABLE_WIFI 1
#endif

#ifndef ENABLE_CLI
#define ENABLE_CLI 1
#endif 

#endif // VUEF_CONF_INT_H