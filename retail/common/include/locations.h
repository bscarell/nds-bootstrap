#ifndef LOCATIONS_H
#define LOCATIONS_H

#define EXCEPTION_STACK_LOCATION 0x23EFFFC

#define ROM_FILE_LOCATION 0x2715000
#define SAV_FILE_LOCATION 0x2715020

#define LOAD_CRT0_LOCATION 0x06860000 // LCDC_BANK_C

#define CARDENGINE_ARM7_LOCATION_BUFFER 0x023E0000

#define CARDENGINE_ARM7_LOCATION 0x023E0000
#define CARDENGINE_ARM9_LOCATION 0x023DC000

#define CARDENGINE_SHARED_ADDRESS 0x027FFB0C

//#define TEMP_MEM 0x02FFE000 // __DSiHeader

#define NDS_HEADER         0x027FFE00
#define NDS_HEADER_SDK5    0x02FFFE00 // __NDSHeader
#define NDS_HEADER_POKEMON 0x027FF000

#define ARM9_START_ADDRESS_LOCATION      (NDS_HEADER + 0x1F4) //0x027FFFF4
#define ARM9_START_ADDRESS_SDK5_LOCATION (NDS_HEADER_SDK5 + 0x1F4) //0x02FFFFF4

#define ROM_LOCATION_S2   0x09000000
#define ROM_LOCATION      0x0C804000
#define ROM_SDK5_LOCATION 0x0D000000

#endif // LOCATIONS_H
