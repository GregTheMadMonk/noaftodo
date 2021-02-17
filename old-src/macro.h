#ifndef NOAFTODO_MACRO_H
#define NOAFTODO_MACRO_H

// version string
#ifndef V_SUFFIX
#define VERSION to_string(LIST_V) + "." + to_string(CONF_V) + "." + to_string(PATCH_V)
#else
#define XMSTR(s) MSTR(s)
#define MSTR(s) #s
#define VERSION to_string(LIST_V) + "." + to_string(CONF_V) + "." + to_string(PATCH_V) + " " + XMSTR(V_SUFFIX)
#endif

// declare method as both const and not const
#define CONST_DPL(decl, body) decl body; decl const body

#endif
