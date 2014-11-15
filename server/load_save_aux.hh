#ifndef WC_SERVER_LOAD_SAVED_H
#define WC_SERVER_LOAD_SAVED_H

#include "unit.h"   /* Unit_Type_id, enum unit_activity */

extern const char hex_chars[];
extern const char* old_impr_types[];
extern const int old_impr_types_size;
extern const char* old_civ1_unit_types[];
extern const int old_civ1_unit_types_size;
extern const char* old_default_unit_types[];
extern const int old_default_unit_types_size;
extern const char* old_default_techs[];
extern const int old_default_techs_size;
extern const char* old_civ2_governments[];
extern const int old_civ2_governments_size;
extern const char* old_default_governments[];

/***************************************************************
This returns an ascii hex value of the given half-byte of the binary
integer. See ascii_hex2bin().
  example: bin2ascii_hex(0xa00, 2) == 'a'
***************************************************************/
#define bin2ascii_hex(value, halfbyte_wanted) \
  hex_chars[((value) >> ((halfbyte_wanted) * 4)) & 0xf]

char activity2char(enum unit_activity activity);

#endif  /* WC_SERVER_LOAD_SAVED_H */
