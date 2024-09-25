// SPDX-LicensE-Identifier: MIT

#include "xbox.h"

extern uint8_t current_encoder_address;
extern uint32_t current_output_mode_coding;
extern display_information_t xbox_display_info;

// clang-format off
const uint8_t XCALIBUR_OFFSETS[66] =
    {0x00, 0x01, 0x0f, 0x1b, 0x50, 0x51, 0x52, 0x54, 0x55, 0x56, 0x58, 0x59, 0x5a, 0x5b, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x80, 0x81, 0x45, 0x46, 0x47, 0x48, 0x42, 0x43, 0x44, 0x1f, 0x20,
     0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41};

// Data written to xcalibur is in dwords. There is a lot of repeated values so instead of storing a huge dword array with lots of repeated values, we first create an array of all the unique values, then we create
// a char sized lookup table that maps the unique values to the correct index in the dword array. This way we can save a lot of space.
// ie 0xA7 = 0xf8d5ee04
const uint32_t XCALIBUR_VALUES[] = 
    {
             //0x00      //0x01      //0x02      //0x03      //0x04      //0x05      //0x06      //0x07      //0x08      //0x09      //0x0A      //0x0B      //0x0C      //0x0D      //0x0E      //0x0F
    /*0x00*/ 0x04010101, 0x00000000, 0x03408010, 0x00000001, 0x030c820d, 0x1e00a027, 0x28000049, 0x030c020d, 0x1e109826, 0x28000060, 0x00808010, 0x000000f0, 0x0000008c, 0x00001001, 0x01410010, 0x00808020,
    /*0x10*/ 0x00000005, 0x0000000f, 0x00000004, 0x08e1e5f8, 0xe1083a3a, 0x0000f8e5, 0x1002fdfa, 0x0000231b, 0x000afffe, 0x0037ffe9, 0x000000e4, 0x0003fffb, 0x000ffffc, 0x007effdb, 0xffe100c0, 0xffff0008,
    /*0x20*/ 0x0009fffd, 0x00beffe1, 0xffdb0080, 0xfffc000e, 0xfffc0003, 0x00e20000, 0xffe80039, 0xfffe000b, 0x04020202, 0x03488010, 0x035a820d, 0x2d00004d, 0x035a020d, 0x2d000060, 0x00000096, 0x00000006,
    /*0x30*/ 0x10eae1f1, 0xea103434, 0x0000f1e1, 0x04010103, 0x03400010, 0x09dee3f7, 0xde093f3f, 0x0000f7e3, 0x000cfffd, 0x003dffe6, 0x000100f5, 0x0004fffa, 0x0089ffd8, 0xffde00cf, 0xfffe000a, 0x000afffd,
    /*0x40*/ 0x00ceffde, 0xffd8008a, 0xfffc000f, 0xfffa0004, 0x00f50000, 0xffe6003e, 0xfffd000c, 0x04020204, 0x03480010, 0x11e8def0, 0xe8113939, 0x0000f0de, 0x0401010b, 0x03400032, 0xf6d3f004, 0xd3f64343,
    /*0x50*/ 0x000004f0, 0x003bffe7, 0x000000f1, 0x0086ffd9, 0xffdf00cb, 0xfffe0009, 0x0009fffe, 0x00caffdf, 0xffd90087, 0xfffb0003, 0x00f00000, 0xffe7003c, 0x0402020c, 0x03480032, 0xffd8e900, 0xd8ff4040,
    /*0x60*/ 0x000000e9, 0x0801010d, 0x00408010, 0x000efffc, 0x007fffdb, 0xffe100bf, 0x0032ffea, 0x000000cd, 0x0003fffc, 0x0072ffdf, 0xffe500ac, 0xffff0007, 0x0802020e, 0x00488010, 0x08010119, 0x00400032,
    /*0x70*/ 0x0802021a, 0x00480032, 0x20010101, 0x0032ffeb, 0x000000ca, 0x000dfffc, 0x0071ffdf, 0xffe400aa, 0x0007fffe, 0x00aaffe4, 0xffdf0071, 0xfffc000d, 0x00c90000, 0xffeb0033, 0x0036ffe9, 0x000000da,
    /*0x80*/ 0x007affdc, 0xffe200b8, 0x20020202, 0x20010103, 0x0008fffe, 0x00b7ffe2, 0xffdc007b, 0x00d90000, 0xffe90037, 0x20020204, 0x2001010b, 0x0034ffea, 0x000000d6, 0x0077ffde, 0xffe300b4, 0x00b4ffe2,
    /*0x90*/ 0xffdd0078, 0xfffd000d, 0x00d60000, 0xffea0035, 0x0077ffdd, 0x2002020c, 0x44030307, 0x03500036, 0x03008271, 0x2400bc2e, 0x28000048, 0x03200208, 0x1e109c27, 0x000005ac, 0x00002002, 0x80920212,
    /*0xA0*/ 0x002d0075, 0x00810060, 0x00d5004b, 0x012d0035, 0x01810020, 0x01d5000b, 0x0000000c, 0xf8d5ee04, 0xd5f84141, 0x000004ee, 0x44040408, 0x03480036, 0x03608271, 0x2d000057, 0x03840208, 0x2d00008a,
    /*0xB0*/ 0x0000065c, 0x0000000e, 0x48030314, 0x00500036, 0x48040415, 0x00480036, 0x60030307, 0x60040408, 0x88070701, 0x00400030, 0x1e009425, 0x2800001c, 0x0000004c, 0x0020d20d, 0x01000010, 0x0010fffb,
    /*0xC0*/ 0x0060ffdb, 0xffe800e9, 0x00020001, 0x00030002, 0x00e7ffe8, 0xffda0062, 0xfffb0010, 0x0051ffe0, 0xffec00c6, 0x00020002, 0x00c5ffec, 0xffe00052, 0x88080801, 0x00490030, 0x2d000020, 0x0000004e, 
    /*0xD0*/ 0x880b0a02, 0x01000001, 0x067202ee, 0x2d00681a, 0x501000d2, 0x50100104, 0x00000034, 0x002ee2ee, 0x00000040, 0x0000000b, 0x011a0000, 0x880e0c03, 0x02000001, 0x08988465, 0x43a0a829, 0x78100092,
    /*0xE0*/ 0x781000bb, 0x00465464};

const uint8_t XCALIBUR_LOOKUP_INDEX[][65] = {
   //MODE          0x01  0x0f  0x1b  0x50  0x51  0x52  0x54  0x55  0x56  0x58  0x59  0x5a  0x5b  0x60  0x61  0x62  0x63  0x64  0x65  0x66  0x80  0x81  0x45  0x46  0x47  0x48  0x42  0x43  0x44  0x1f  0x20  0x21  0x22  0x23  0x24  0x25  0x26  0x27  0x28  0x29  0x2a  0x2b  0x2c  0x2d  0x2e  0x2f  0x30  0x31  0x32  0x33  0x34  0x35  0x36  0x37  0x38  0x39  0x3a  0x3b  0x3c  0x3d  0x3e  0x3f  0x40  0x41
   /*0x04010101*/ {0x00, 0x02, 0x03, 0x01, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x12, 0x16, 0x17, 0x12, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, },
   /*0x04020202*/ {0x28, 0x29, 0x03, 0x01, 0x2a, 0x05, 0x2b, 0x2c, 0x08, 0x2d, 0x0a, 0x0b, 0x2e, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x2f, 0x11, 0x12, 0x30, 0x31, 0x32, 0x12, 0x16, 0x17, 0x12, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, },
   /*0x04010103*/ {0x33, 0x34, 0x03, 0x01, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x10, 0x11, 0x12, 0x35, 0x36, 0x37, 0x12, 0x16, 0x17, 0x12, 0x38, 0x39, 0x3a, 0x3b, 0x1c, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, },
   /*0x04020204*/ {0x47, 0x48, 0x03, 0x01, 0x2a, 0x05, 0x2b, 0x2c, 0x08, 0x2d, 0x0a, 0x0b, 0x2e, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x2f, 0x11, 0x12, 0x49, 0x4a, 0x4b, 0x12, 0x16, 0x17, 0x12, 0x38, 0x39, 0x3a, 0x3b, 0x1c, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, },
   /*0x0401010b*/ {0x4c, 0x4d, 0x03, 0x01, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x10, 0x11, 0x12, 0x4e, 0x4f, 0x50, 0x12, 0x16, 0x17, 0x12, 0x38, 0x51, 0x52, 0x1b, 0x1c, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x42, 0x59, 0x5a, 0x5b, 0x27, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, },
   /*0x0402020c*/ {0x5c, 0x5d, 0x03, 0x01, 0x2a, 0x05, 0x2b, 0x2c, 0x08, 0x2d, 0x0a, 0x0b, 0x2e, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x2f, 0x11, 0x12, 0x5e, 0x5f, 0x60, 0x12, 0x16, 0x17, 0x12, 0x38, 0x51, 0x52, 0x1b, 0x1c, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x42, 0x59, 0x5a, 0x5b, 0x27, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, },
   /*0x0801010d*/ {0x61, 0x62, 0x03, 0x01, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x10, 0x11, 0x12, 0x01, 0x01, 0x01, 0x12, 0x01, 0x01, 0x12, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x18, 0x19, 0x1a, 0x1b, 0x63, 0x64, 0x65, 0x55, 0x01, 0x18, 0x66, 0x67, 0x68, 0x38, 0x69, 0x6a, 0x6b, },
   /*0x0802020e*/ {0x6c, 0x6d, 0x03, 0x01, 0x2a, 0x05, 0x2b, 0x2c, 0x08, 0x2d, 0x0a, 0x0b, 0x2e, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x2f, 0x11, 0x12, 0x01, 0x01, 0x01, 0x12, 0x01, 0x01, 0x12, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x18, 0x19, 0x1a, 0x1b, 0x63, 0x64, 0x65, 0x55, 0x01, 0x18, 0x66, 0x67, 0x68, 0x38, 0x69, 0x6a, 0x6b, },
   /*0x08010119*/ {0x6e, 0x6f, 0x03, 0x01, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x10, 0x11, 0x12, 0x01, 0x01, 0x01, 0x12, 0x01, 0x01, 0x12, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x51, 0x52, 0x1b, 0x1c, 0x53, 0x54, 0x55, 0x01, 0x18, 0x66, 0x67, 0x68, 0x38, 0x69, 0x6a, 0x6b, },
   /*0x0802021a*/ {0x70, 0x71, 0x03, 0x01, 0x2a, 0x05, 0x2b, 0x2c, 0x08, 0x2d, 0x0a, 0x0b, 0x2e, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x2f, 0x11, 0x12, 0x01, 0x01, 0x01, 0x12, 0x01, 0x01, 0x12, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x51, 0x52, 0x1b, 0x1c, 0x53, 0x54, 0x55, 0x01, 0x18, 0x66, 0x67, 0x68, 0x38, 0x69, 0x6a, 0x6b, },
   /*0x20010101*/ {0x72, 0x02, 0x03, 0x01, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x12, 0x16, 0x17, 0x12, 0x56, 0x73, 0x74, 0x68, 0x75, 0x76, 0x77, 0x1f, 0x78, 0x79, 0x7a, 0x7b, 0x24, 0x7c, 0x7d, 0x55, 0x01, 0x18, 0x7e, 0x7f, 0x68, 0x63, 0x80, 0x81, 0x1f, 0x01, 0x18, 0x7e, 0x7f, 0x68, 0x63, 0x80, 0x81, 0x1f, },
   /*0x20020202*/ {0x82, 0x29, 0x03, 0x01, 0x2a, 0x05, 0x2b, 0x2c, 0x08, 0x2d, 0x0a, 0x0b, 0x2e, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x2f, 0x11, 0x12, 0x30, 0x31, 0x32, 0x12, 0x16, 0x17, 0x12, 0x56, 0x73, 0x74, 0x68, 0x75, 0x76, 0x77, 0x1f, 0x78, 0x79, 0x7a, 0x7b, 0x24, 0x7c, 0x7d, 0x55, 0x01, 0x18, 0x7e, 0x7f, 0x68, 0x63, 0x80, 0x81, 0x1f, 0x01, 0x18, 0x7e, 0x7f, 0x68, 0x63, 0x80, 0x81, 0x1f, },
   /*0x20010103*/ {0x83, 0x34, 0x03, 0x01, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x10, 0x11, 0x12, 0x35, 0x36, 0x37, 0x12, 0x16, 0x17, 0x12, 0x18, 0x7e, 0x7f, 0x68, 0x63, 0x80, 0x81, 0x1f, 0x84, 0x85, 0x86, 0x23, 0x24, 0x87, 0x88, 0x3e, 0x01, 0x18, 0x7e, 0x7f, 0x68, 0x63, 0x80, 0x81, 0x1f, 0x01, 0x18, 0x7e, 0x7f, 0x68, 0x63, 0x80, 0x81, 0x1f, },
   /*0x20020204*/ {0x89, 0x48, 0x03, 0x01, 0x2a, 0x05, 0x2b, 0x2c, 0x08, 0x2d, 0x0a, 0x0b, 0x2e, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x2f, 0x11, 0x12, 0x49, 0x4a, 0x4b, 0x12, 0x16, 0x17, 0x12, 0x18, 0x7e, 0x7f, 0x68, 0x63, 0x80, 0x81, 0x1f, 0x84, 0x85, 0x86, 0x23, 0x24, 0x87, 0x88, 0x3e, 0x01, 0x18, 0x7e, 0x7f, 0x68, 0x63, 0x80, 0x81, 0x1f, 0x01, 0x18, 0x7e, 0x7f, 0x68, 0x63, 0x80, 0x81, 0x1f, },
   /*0x2001010b*/ {0x8a, 0x4d, 0x03, 0x01, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x10, 0x11, 0x12, 0x4e, 0x4f, 0x50, 0x12, 0x16, 0x17, 0x12, 0x18, 0x8b, 0x8c, 0x68, 0x75, 0x8d, 0x8e, 0x1f, 0x84, 0x8f, 0x90, 0x91, 0x59, 0x92, 0x93, 0x3e, 0x01, 0x18, 0x8b, 0x8c, 0x68, 0x63, 0x94, 0x8e, 0x1f, 0x01, 0x18, 0x8b, 0x8c, 0x68, 0x75, 0x8d, 0x8e, 0x1f, },
   /*0x2002020c*/ {0x95, 0x5d, 0x03, 0x01, 0x2a, 0x05, 0x2b, 0x2c, 0x08, 0x2d, 0x0a, 0x0b, 0x2e, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x2f, 0x11, 0x12, 0x5e, 0x5f, 0x60, 0x12, 0x16, 0x17, 0x12, 0x18, 0x8b, 0x8c, 0x68, 0x75, 0x8d, 0x8e, 0x1f, 0x84, 0x8f, 0x90, 0x91, 0x59, 0x92, 0x93, 0x3e, 0x01, 0x18, 0x8b, 0x8c, 0x68, 0x63, 0x94, 0x8e, 0x1f, 0x01, 0x18, 0x8b, 0x8c, 0x68, 0x75, 0x8d, 0x8e, 0x1f, },
   /*0x44030307*/ {0x96, 0x97, 0x03, 0x01, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x09, 0x0a, 0x0b, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0x11, 0x12, 0xa7, 0xa8, 0xa9, 0x12, 0x16, 0x17, 0x12, 0x38, 0x51, 0x52, 0x1b, 0x1c, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x42, 0x59, 0x5a, 0x5b, 0x27, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, },
   /*0x44040408*/ {0xaa, 0xab, 0x03, 0x01, 0xac, 0x99, 0xad, 0xae, 0x9c, 0xaf, 0x0a, 0x0b, 0xb0, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xb1, 0x11, 0x12, 0x5e, 0x5f, 0x60, 0x12, 0x16, 0x17, 0x12, 0x38, 0x51, 0x52, 0x1b, 0x1c, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x42, 0x59, 0x5a, 0x5b, 0x27, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, },
   /*0x48030314*/ {0xb2, 0xb3, 0x03, 0x01, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x09, 0x0a, 0x0b, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0x11, 0x12, 0x01, 0x01, 0x01, 0x12, 0x01, 0x01, 0x12, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x51, 0x52, 0x1b, 0x1c, 0x53, 0x54, 0x55, 0x01, 0x18, 0x66, 0x67, 0x68, 0x38, 0x69, 0x6a, 0x6b, },
   /*0x48040415*/ {0xb4, 0xb5, 0x03, 0x01, 0xac, 0x99, 0xad, 0xae, 0x9c, 0xaf, 0x0a, 0x0b, 0xb0, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xb1, 0x11, 0x12, 0x01, 0x01, 0x01, 0x12, 0x01, 0x01, 0x12, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x38, 0x51, 0x52, 0x1b, 0x1c, 0x53, 0x54, 0x55, 0x01, 0x18, 0x66, 0x67, 0x68, 0x38, 0x69, 0x6a, 0x6b, },
   /*0x60030307*/ {0xb6, 0x97, 0x03, 0x01, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x09, 0x0a, 0x0b, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0x11, 0x12, 0xa7, 0xa8, 0xa9, 0x12, 0x16, 0x17, 0x12, 0x18, 0x8b, 0x8c, 0x68, 0x75, 0x8d, 0x8e, 0x1f, 0x84, 0x8f, 0x90, 0x91, 0x59, 0x92, 0x93, 0x3e, 0x01, 0x18, 0x8b, 0x8c, 0x68, 0x63, 0x94, 0x8e, 0x1f, 0x01, 0x18, 0x8b, 0x8c, 0x68, 0x75, 0x8d, 0x8e, 0x1f, },
   /*0x60040408*/ {0xb7, 0xab, 0x03, 0x01, 0xac, 0x99, 0xad, 0xae, 0x9c, 0xaf, 0x0a, 0x0b, 0xb0, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xb1, 0x11, 0x12, 0x5e, 0x5f, 0x60, 0x12, 0x16, 0x17, 0x12, 0x18, 0x8b, 0x8c, 0x68, 0x75, 0x8d, 0x8e, 0x1f, 0x84, 0x8f, 0x90, 0x91, 0x59, 0x92, 0x93, 0x3e, 0x01, 0x18, 0x8b, 0x8c, 0x68, 0x63, 0x94, 0x8e, 0x1f, 0x01, 0x18, 0x8b, 0x8c, 0x68, 0x75, 0x8d, 0x8e, 0x1f, },
   /*0x88070701*/ {0xb8, 0xb9, 0x03, 0x03, 0x07, 0xba, 0xbb, 0x07, 0xba, 0x09, 0x0a, 0x0b, 0xbc, 0xbd, 0xbe, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x10, 0x11, 0x12, 0x01, 0x01, 0x01, 0x12, 0x01, 0x01, 0x12, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0x01, 0x63, 0xc7, 0xc8, 0xc2, 0xc9, 0xca, 0xcb, 0x23, },
   /*0x88080801*/ {0xcc, 0xcd, 0x03, 0x03, 0x2c, 0xba, 0xce, 0x2c, 0xba, 0x2d, 0x0a, 0x0b, 0xcf, 0xbd, 0xbe, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x2f, 0x11, 0x12, 0x01, 0x01, 0x01, 0x12, 0x01, 0x01, 0x12, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0x01, 0x63, 0xc7, 0xc8, 0xc2, 0xc9, 0xca, 0xcb, 0x23, },
   /*0x880b0a02*/ {0xd0, 0xb9, 0x03, 0xd1, 0xd2, 0xd3, 0xd4, 0xd2, 0xd3, 0xd5, 0x0a, 0x0b, 0xd6, 0xd7, 0xbe, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd9, 0x11, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x12, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xda, 0x01, 0x01, 0x01, 0xda, 0x01, 0x01, 0x01, 0x01, 0x5a, 0x01, 0x01, 0x01, 0x5a, 0x01, 0x01, },
   /*0x880e0c03*/ {0xdb, 0xb9, 0x03, 0xdc, 0xdd, 0xde, 0xdf, 0xdd, 0xde, 0xe0, 0x0a, 0x0b, 0xd6, 0xe1, 0xbe, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0xd9, 0x11, 0x12, 0x01, 0x01, 0x01, 0x12, 0x01, 0x01, 0x12, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xda, 0x01, 0x01, 0x01, 0xda, 0x01, 0x01, 0x01, 0x01, 0x5a, 0x01, 0x01, 0x01, 0x5a, 0x01, 0x01}};
// clang-format on

uint8_t xbox_encoder_detect(void)
{
    if (xbox_smbus_poke(XBOX_SMBUS_ADDRESS_ENCODER_CONEXANT) != SMBUS_RETURN_ERROR) {
        XPRINTF("[ENCODER] Detected Conexant encoder\n");
        return XBOX_SMBUS_ADDRESS_ENCODER_CONEXANT;
    }

    if (xbox_smbus_poke(XBOX_SMBUS_ADDRESS_ENCODER_FOCUS) != SMBUS_RETURN_ERROR) {
        XPRINTF("[ENCODER] Detected Focus encoder\n");
        return XBOX_SMBUS_ADDRESS_ENCODER_FOCUS;
    }

    if (xbox_smbus_poke(XBOX_SMBUS_ADDRESS_ENCODER_XCALIBUR) != SMBUS_RETURN_ERROR) {
        XPRINTF("[ENCODER] Detected Xcalibur encoder\n");
        return XBOX_SMBUS_ADDRESS_ENCODER_XCALIBUR;
    }

    XPRINTF("[ENCODER] No encoder detected\n");
    return 0;
}

void xbox_encoder_configure(uint32_t mode_coding, display_information_t *display_info)
{
    uint32_t temp;
    uint8_t lookup_row = 0xFF;

    const VIDEO_MODE_SETTING *mode_settings = video_get_settings(mode_coding);
    if (mode_settings == NULL) {
        assert(0);
        return;
    }

    const uint16_t width = display_info->hvalid_end - display_info->hvalid_start + 1;
    const uint16_t height = display_info->vvalid_end - display_info->vvalid_start + 1;

    const uint8_t pramdac_index = ((mode_coding & 0x00FF0000) >> 16);
    const uint8_t is_ed = (mode_coding & 0x80000000) ? 1 : 0; // Enhanced definition (480p or greater)
    const uint8_t is_hd = pramdac_index >= 0x0A;              // High definition (720p, 1080i)..

    const uint8_t is_hd_interlaced = (height == 1080); // Surely a better way to check this
    const uint8_t is_sd_pal50 = (mode_coding & 0x40000000) ? 1 : 0;
    const uint8_t is_sd_pal60 = ((mode_settings->dwStandard == VIDEO_REGION_PAL) && is_sd_pal50 == 0) ? 1 : 0;
    const uint8_t is_sd_ntscj = (mode_settings->dwStandard == VIDEO_REGION_NTSCJ) ? 1 : 0;
    const uint8_t is_sd_scart = (mode_coding & 0x20000000) ? 1 : 0;
    const uint8_t is_sd_yuv = (mode_coding == 0x48030314) | (mode_coding == 0x48040415) | (mode_coding == 0x08010119) |
                              (mode_coding == 0x0802021a) | (mode_coding == 0x0801010d) |
                              (mode_coding == 0x0802020e); // Surely better way to know

    const uint16_t hsync_width = display_info->hsync_end - display_info->hsync_start;
    const uint16_t hsync_frontporch_width = display_info->hsync_start - display_info->hdisplay_end;
    const uint16_t hsync_backporch_width = display_info->htotal - display_info->hsync_end + display_info->hvalid_start;
    const uint16_t hsync_delay = hsync_backporch_width + width + hsync_width;
    const uint16_t vsync_height = display_info->vsync_end - display_info->vsync_start;
    const uint16_t vsync_frontporch_height = display_info->vsync_start - display_info->vdisplay_end;
    const uint16_t vsync_backporch_height = display_info->vtotal - display_info->vsync_end + display_info->vvalid_start;

    // const uint16_t vsync_delay = vsync_backporch_height + height + vsync_height;
    // const uint8_t is_wss = (mode_coding & 0x10000000) ? 1 : 0;

    XPRINTF("[ENCODER] Configuring %s encoder\n",
            (current_encoder_address == XBOX_SMBUS_ADDRESS_ENCODER_CONEXANT) ? "Conexant"
            : (current_encoder_address == XBOX_SMBUS_ADDRESS_ENCODER_FOCUS)  ? "Focus"
                                                                             : "Xcalibur");

    switch (current_encoder_address) {
    case XBOX_SMBUS_ADDRESS_ENCODER_XCALIBUR:
        for (uint8_t i = 0; i < XBOX_ARRAY_SIZE(XCALIBUR_LOOKUP_INDEX); i++) {
            if (XCALIBUR_VALUES[XCALIBUR_LOOKUP_INDEX[i][0]] == mode_coding) {
                lookup_row = i;
                break;
            }
        }
        if (lookup_row == 0xFF) {
            assert(0);
            return;
        }

        // Set output timing
        for (uint8_t i = 1; i < sizeof(XCALIBUR_OFFSETS); i++) {
            uint8_t command = XCALIBUR_OFFSETS[i];
            uint32_t value = XCALIBUR_VALUES[XCALIBUR_LOOKUP_INDEX[lookup_row][i]];
            xbox_smbus_output_dword(current_encoder_address, command, value);
        }

        // Enable SCART
        if (is_sd_scart) {
            xbox_smbus_input_dword(current_encoder_address, 0x01, &temp);
            xbox_smbus_output_dword(current_encoder_address, 0x01, (temp & 0xfcffffff) | 0xc00008);
            xbox_smbus_input_dword(current_encoder_address, 0x60, &temp);
            xbox_smbus_output_dword(current_encoder_address, 0x60, temp & 0xfeffffff);
            xbox_smbus_output_dword(current_encoder_address, 0x58, 0x00000000);
        }

        // Some kind of HD mode setup?
        if (mode_coding & 0xC0000000) {
            if ((mode_coding & 0xC0000000) == 0x80000000) {
                xbox_smbus_output_dword(current_encoder_address, 0x07, 0x00000000);
            }
            xbox_smbus_output_dword(current_encoder_address, 0x09, 0x00000000);
        }

        // ?
        xbox_smbus_input_dword(current_encoder_address, 0x00, &temp);
        xbox_smbus_output_dword(current_encoder_address, 0x00, temp | 2);

        // ?
        xbox_smbus_output_dword(current_encoder_address, 0x0C, 0x0000000F);
        xbox_smbus_output_dword(current_encoder_address, 0x0D, 0x00000000);
        xbox_smbus_output_dword(current_encoder_address, 0x0E, 0x00000001);

        break;
    case XBOX_SMBUS_ADDRESS_ENCODER_CONEXANT:
        break;
    case XBOX_SMBUS_ADDRESS_ENCODER_FOCUS: {
        // clang-format off

            // For the most part, xbox doesnt write to focus in words, but two bytes separately.
            // Probably not necessary to do this, but it's more accurate to the original code.
            #define FOCUS_OUTPUT_WORD(reg, value) \
                xbox_smbus_output_byte(current_encoder_address, reg, (uint8_t)((value) & 0xFF)); \
                xbox_smbus_output_byte(current_encoder_address, reg + 1, (uint8_t)((value) >> 8));

            #define FOCUS_OUTPUT_BYTE(reg, value) \
                xbox_smbus_output_byte(current_encoder_address, reg, value);

            #define ED_HD_HDI(a, b, c) \
                ((is_hd_interlaced) ? c : (is_hd) ? b : a)

            #define P50_P60_NTSCM_NTSCJ(a, b, c, d) \
                ((is_sd_pal50) ? a : (is_sd_pal60) ? b : (is_sd_ntscj) ? d : c)
        // clang-format on

        // Soft reset of encoder begin - maintain reset while bit 0 is set
        FOCUS_OUTPUT_WORD(FOCUS_CTRL_CR_16, 0x0001);

        if (!is_ed) {
            // Some constant pre-initialization values for SD modes
            static const uint8_t focus_pre_data[][2] = {
                {0x12, 0x00}, {0x13, 0x00}, {0x16, 0x00}, {0x17, 0x00}, {0x18, 0xef}, {0x19, 0x43}, {0x1c, 0x07},
                {0x1d, 0x07}, {0x24, 0x00}, {0x25, 0x00}, {0x26, 0x10}, {0x27, 0x00}, {0x44, 0x00}, {0x45, 0x00},
                {0x4d, 0x01}, {0x5e, 0xc8}, {0x5f, 0x00}, {0x65, 0x00}, {0x75, 0x10}, {0x83, 0x18}, {0x84, 0x00},
                {0x85, 0x00}, {0x86, 0x18}, {0x87, 0x00}, {0x88, 0x00}, {0x8b, 0x9c}, {0x8c, 0x03}, {0x9e, 0xe4},
                {0x9f, 0x00}, {0xa0, 0x00}, {0xa1, 0x02}, {0xa2, 0x00}, {0xa3, 0x00}, {0xa4, 0x00}, {0xa5, 0x00},
                {0xa6, 0x00}, {0xa7, 0x00}, {0xa8, 0x00}, {0xa9, 0x01}, {0xaa, 0x00}, {0xab, 0x01}, {0xac, 0x00},
                {0xad, 0x01}, {0xb6, 0xf0}, {0xb7, 0x00}, {0xc2, 0xee}, {0xc3, 0x00}};

            for (uint8_t i = 0; i < sizeof(focus_pre_data) / 2; i++) {
                FOCUS_OUTPUT_BYTE(focus_pre_data[i][0], focus_pre_data[i][1]);
            }
        }

        FOCUS_OUTPUT_WORD(FOCUS_CTRL_CR_16, (is_sd_pal50) ? 0x2101 : 0x2001);
        FOCUS_OUTPUT_WORD(FOCUS_CTRL_MISC_16, 0x0015);

        int32_t scale_height = 0;
        if (is_ed) {
            FOCUS_OUTPUT_WORD(FOCUS_CTRL_BYPASS_16, 0x00);
        } else {
            FOCUS_OUTPUT_WORD(FOCUS_SDTVI_IHO_16, hsync_width + hsync_backporch_width);
            FOCUS_OUTPUT_WORD(FOCUS_SDTVI_IVO_16, vsync_height + vsync_backporch_height);
            FOCUS_OUTPUT_WORD(FOCUS_SDTVI_IHW_16, width);

            // For PAL50, scale vertical to 625 lines.
            // For NTSC or PAL60, scale vertical to 525 lines. See Focus datasheet 2.2.1.4 for equation
            if (is_sd_pal50) {
                scale_height = (65536 * 625) / (display_info->vtotal + 1) - 65536;
            } else {
                // NTSC and PAL60 is already 525 lines so this should be zero
                scale_height = (65536 * 525) / (display_info->vtotal + 1) - 65536;
            }
            FOCUS_OUTPUT_WORD(FOCUS_SDTVI_VSC_16, (int16_t)scale_height);

            // Always scale horizontal to 720 pixels. See Focus datasheet 2.2.1.5 for equation
            int32_t scale_width = (128 * 720) / width - 128;
            if (scale_width <= 0) {
                FOCUS_OUTPUT_BYTE(FOCUS_SDTVI_HSCD_8, (int8_t)scale_width);
                FOCUS_OUTPUT_BYTE(FOCUS_SDTVI_HSCU_8, 0);
            } else if (scale_width > 0) {
                FOCUS_OUTPUT_BYTE(FOCUS_SDTVI_HSCD_8, 0);
                FOCUS_OUTPUT_BYTE(FOCUS_SDTVI_HSCU_8, (int8_t)scale_width);
            }

            // Datasheet implies it should be 0xA4 only for downscaling, but xbox does it for upscaling too
            FOCUS_OUTPUT_WORD(FOCUS_SDTVI_FIFO_LAT_16, (scale_height != 0) ? 0x00A4 : 0x0082);
        }

        // PLL CLK setup
        {
            // PLL Setup (Pretty intensive calc, have a lookup for common modes?)
            // clang-format off
                // Figure 2, this is the PLL stages for Focus.
                // XTAL * NCON / NCOD = nco_out -> / pll_n = pll_inp -> * pll_m = pll_outp -> / IP/EP Divider = actual_clock.
                // pll_inp must be 100 kHz to 1000 kHz.
                // Pll_outp must be 100 MHz to 300 MHz The NCO (NCON/ NCOD) fraction must be greater than 0 and less than 1/2.
                // Pll_n must be 27 to 270.
                // Pll_m must be 250 to 3000.
                // IP/EP divider (out_div) must be 1 to 16, IP generally equal EP. Will be in this case.
            // clang-format on
            const uint32_t XTAL_IN = 27000000;
            uint32_t nco_divider, pll_n, pll_m, out_div, actual_clock, found = 0;
            uint16_t pllg;

            uint32_t desired_clock = (display_info->vtotal + 1) * (display_info->htotal + 1);
            if (is_sd_pal50) {
                desired_clock = desired_clock * 50;
            } else {
                desired_clock = ((uint64_t)desired_clock * 5994ULL) / 100ULL; // 59.94Hz without float
            }

            if (is_hd_interlaced) {
                desired_clock /= 2;
            }

            // Between 2^24 and 2. But start at a reasonable number
            for (nco_divider = 16; nco_divider >= 2; nco_divider--) {
                uint32_t nco_out = XTAL_IN / nco_divider;

                // Start at a low divider to try prevent pll_m being a big multiplier
                for (pll_n = 2; pll_n <= 270; pll_n++) {
                    uint32_t pll_inp = nco_out / pll_n;

                    // pll_inp must be 100 kHz to 1000 kHz, but we limit to 500khz
                    // beacuse of Table 10 in the Focus datasheet.
                    if (pll_inp > 500000) {
                        continue;

                    } else if (pll_inp < 100000) {
                        break; // Divider is increasing so we can break
                    }

                    // Start at the smallest multiplier
                    for (pll_m = 250; pll_m < 3000; pll_m++) {
                        uint32_t pll_outp = pll_inp * pll_m;

                        // pll_outp must be 100 MHz to 300 MHz
                        if (pll_outp < 100000000) {
                            continue;
                        } else if (pll_outp > 300000000) {
                            break; // Multiplier is increasing so we can break
                        }

                        // Now determine the final output dividier
                        for (out_div = 16; out_div >= 1; out_div--) {
                            actual_clock = pll_outp / out_div;

                            if (actual_clock > 150000000) {
                                break;
                            }

                            // if final output is within 1Hz of desired clock, we found a match
                            if (actual_clock >= desired_clock - 1 && actual_clock <= desired_clock + 1) {
                                // XPRINTF("pllin %d kHz, pllout %d Mhz\n", pll_inp / 1000, pll_outp / 1000000);
                                found = 1;

                                // Work out the correct charge pump value
                                // Refer Table 10 in the Focus datasheet
                                uint32_t pllin_khz = pll_inp / 1000;
                                uint32_t pllout_mhz = pll_outp / 1000000;

                                pllout_mhz = ((pllout_mhz + 25) / 50) * 50; // Round to closest 50
                                pllg = (pllout_mhz * 2) / 100 - 1;
                                if (pllin_khz > 150 && pllin_khz <= 400) {
                                    pllg += 1;
                                } else if (pllin_khz > 750) {
                                    pllg -= 1;
                                }
                                pllg = XBOX_CLAMP(1, pllg, 5);
                                goto pll_search_finished;

                            } else {
                                continue;
                            }
                        }
                    }
                }
            }
        pll_search_finished:
            if (found) {
                pll_m = pll_m - 17;                             // Programmed as M-17
                pll_n = pll_n - 1;                              // Programmed as N-1
                out_div = out_div - 1;                          // Programmed as IP-1
                FOCUS_OUTPUT_WORD(FOCUS_CLOCK_NCON_32, 0x0001); // for simplicity numerator is always 1
                if (is_ed) {
                    FOCUS_OUTPUT_WORD(FOCUS_CLOCK_NCON_32 + 2, 0x000);
                }
                FOCUS_OUTPUT_WORD(FOCUS_CLOCK_NCOD_32, (uint16_t)(nco_divider & 0xFFFF));
                if (is_ed) {
                    FOCUS_OUTPUT_WORD(FOCUS_CLOCK_NCOD_32 + 2, (uint16_t)((nco_divider >> 16) & 0xFFFF));
                }

                FOCUS_OUTPUT_WORD(FOCUS_CLOCK_PLLM_PUMP_16, (pllg << 12) | (pll_m & 0xFFF));
                FOCUS_OUTPUT_WORD(FOCUS_CLOCK_PLLN_16, (uint16_t)pll_n);

                FOCUS_OUTPUT_WORD(FOCUS_CLOCK_POST_DIVIDER_16, (uint16_t)(out_div << 8 | out_div));
            } else {
                XPRINTF("[ENCODER] Failed to find PLL values for Focus encoder\n");
            }
        }

        if (is_ed) {
            FOCUS_OUTPUT_WORD(FOCUS_CTRL_VIDCNTRL0_16, ED_HD_HDI(0x483E, 0x582E, 0x58AE));
            FOCUS_OUTPUT_WORD(FOCUS_CTRL_9A_16, ED_HD_HDI(0x4000, 0x4000, 0x0001));
            FOCUS_OUTPUT_WORD(FOCUS_CTRL_DACCNTL_16, 0x00E4);
            FOCUS_OUTPUT_WORD(FOCUS_CTRL_PWR_MGNT_16, 0x0400);

            // Reset closed caption stuff
            FOCUS_OUTPUT_WORD(FOCUS_SDTVO_CCC_16, 0x0000);
            FOCUS_OUTPUT_WORD(FOCUS_SDTVO_CC_BLANK_SPL_16, 0x00F0);

            // Setup colour matrix
            FOCUS_OUTPUT_WORD(FOCUS_COL_MATRIX_RED_COEFF_16, 0x0000);
            FOCUS_OUTPUT_WORD(FOCUS_COL_MATRIX_GREEN_COEFF_16, 0x0000);
            FOCUS_OUTPUT_WORD(FOCUS_COL_MATRIX_BLUE_COEFF_16, 0x0000);
            FOCUS_OUTPUT_WORD(FOCUS_COL_MATRIX_RED_SCALE_16, 0x009D);
            FOCUS_OUTPUT_WORD(FOCUS_COL_MATRIX_GREEN_SCALE_16, 0x00A5);
            FOCUS_OUTPUT_WORD(FOCUS_COL_MATRIX_BLUE_SCALE_16, 0x009D);

            // Setup output timing
            FOCUS_OUTPUT_WORD(FOCUS_HDTV_OUT_HD_FP_SYNC_16, hsync_backporch_width << 8 | hsync_width);
            FOCUS_OUTPUT_WORD(FOCUS_HDTV_OUT_HD_YOFF_BP_16, (0 << 8) | hsync_backporch_width);
            FOCUS_OUTPUT_WORD(FOCUS_HDTV_OUT_SYNC_DL_16, hsync_delay);
            FOCUS_OUTPUT_WORD(FOCUS_HDTV_OUT_HACT_ST_16, hsync_frontporch_width + hsync_width + hsync_backporch_width);
            FOCUS_OUTPUT_WORD(FOCUS_HDTV_OUT_HACT_WD_16, width);
            FOCUS_OUTPUT_WORD(FOCUS_HDTV_OUT_VACT_ST_16,
                              vsync_frontporch_height + vsync_height + vsync_backporch_height);
            FOCUS_OUTPUT_WORD(FOCUS_HDTV_OUT_VACT_HT_16, height);

        } else {
            // Set the chroma frequency - Fixed values
            uint32_t chr_freq = P50_P60_NTSCM_NTSCJ(0xCB8A092A, 0x5389092A, 0x1F7CF021, 0x1F7CF021);
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_CHR_FREQ_32 + 0, (chr_freq >> 0) & 0xFF);
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_CHR_FREQ_32 + 1, (chr_freq >> 8) & 0xFF);
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_CHR_FREQ_32 + 2, (chr_freq >> 16) & 0xFF);
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_CHR_FREQ_32 + 3, (chr_freq >> 24) & 0xFF);

            // 0x8D for PAL and 0x89 for NTSC - Just fixed values
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_MISC46_8, (is_sd_pal50 || is_sd_pal60) ? 0x8D : 0x89);

            // Setup YUV
            uint8_t misc_47 = (is_sd_yuv) ? 0x0C : 0x00;
            uint8_t misc_74 = P50_P60_NTSCM_NTSCJ(0x49, 0x51, 0x04, 0x04);
            if (is_sd_yuv) {
                misc_74 |= 0x20;
            }
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_MISC47_8, misc_47);
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_MISC_74_8, misc_74);

            // Output timing
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_HSYNC_WID_8, P50_P60_NTSCM_NTSCJ(0x7C, 0x7C, 0x7C, 0x7C));
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_BURST_WID_8, P50_P60_NTSCM_NTSCJ(0x3C, 0x40, 0x40, 0x40));
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_BPORCH_8, P50_P60_NTSCM_NTSCJ(0x9A, 0x80, 0x80, 0x80));
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_CB_BURST_8, P50_P60_NTSCM_NTSCJ(0x2F, 0x2F, 0x3E, 0x3E));
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_CR_BURST_8, P50_P60_NTSCM_NTSCJ(0x21, 0x21, 0x00, 0x00));
            FOCUS_OUTPUT_WORD(FOCUS_SDTVO_BLACK_LVL_16, P50_P60_NTSCM_NTSCJ(0x003F, 0x003F, 0x0246, 0x013C));
            FOCUS_OUTPUT_WORD(FOCUS_SDTVO_BLANK_LVL16, P50_P60_NTSCM_NTSCJ(0x033E, 0x033E, 0x003C, 0x003C));
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_CB_GAIN_8, P50_P60_NTSCM_NTSCJ(0x9D, 0x9D, 0x91, 0x9D));
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_CR_GAIN_8, P50_P60_NTSCM_NTSCJ(0x9D, 0x9D, 0x91, 0x9D));
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_BR_WAY_8, P50_P60_NTSCM_NTSCJ(0x1A, 0x19, 0x19, 0x19));
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_FR_PORCH_8, P50_P60_NTSCM_NTSCJ(0x1E, 0x24, 0x24, 0x24));
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_1ST_LINE_8, P50_P60_NTSCM_NTSCJ(0x15, 0x11, 0x14, 0x14));
            FOCUS_OUTPUT_WORD(FOCUS_SDTVO_VBI_BL_LVL_16, P50_P60_NTSCM_NTSCJ(0x033E, 0x033E, 0x003C, 0x003C));
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_WSS_CONFIG_8, P50_P60_NTSCM_NTSCJ(0x57, 0x67, 0x67, 0x67));
            FOCUS_OUTPUT_WORD(FOCUS_SDTVO_WSS_CLK_16, P50_P60_NTSCM_NTSCJ(0x072F, 0x0C21, 0x0C21, 0x0C21));
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_WSS_LNF1_8, P50_P60_NTSCM_NTSCJ(0x16, 0x10, 0x13, 0x13));
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_WSS_LNF0_8, P50_P60_NTSCM_NTSCJ(0x16, 0x10, 0x13, 0x13));
            FOCUS_OUTPUT_WORD(FOCUS_SDTVO_CCC_16, P50_P60_NTSCM_NTSCJ(0x05D7, 0x0596, 0x0555, 0x0555));
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_PR_SC_8, is_sd_yuv ? 0x93 : 0x00);
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_PB_SC_8, is_sd_yuv ? 0x93 : 0x00);

            FOCUS_OUTPUT_WORD(FOCUS_CTRL_VIDCNTRL0_16, (is_sd_yuv) ? 0x48C5 : 0x48c4);
            FOCUS_OUTPUT_WORD(FOCUS_CTRL_9A_16, (is_sd_pal50) ? 0x8000 : 0x0000);
        }

        FOCUS_OUTPUT_WORD(FOCUS_CTRL_CR_16, is_sd_pal50 ? 0x2103 : 0x2003);
        FOCUS_OUTPUT_WORD(FOCUS_CTRL_MISC_16, 0x0415);
        FOCUS_OUTPUT_WORD(FOCUS_CTRL_CR_16, is_sd_pal50 ? 0x2100 : 0x2000);

        // Weirdly, xbox only does this if EEPROM is set to NTSCJ? Surely we should do it for all NTSCJ formats
        if ((mode_settings->dwStandard == VIDEO_REGION_NTSCJ) &&
            (mode_coding == 0x0802020e || mode_coding == 0x0801010d)) {
            FOCUS_OUTPUT_BYTE(FOCUS_SDTVO_BLACK_LVL_16, 0x3C);
        }

        if (is_sd_scart) {
            // FIXME, These actually first read the register then clear/set bits as required.
            // Turns out on xbox they are always the same anyway
            const uint8_t scart_enable[][2] = {{0x92, 0xC1}, {0x93, 0x08}, {0xA2, 0x4D}, {0xA3, 0x00}, {0xA4, 0x96},
                                               {0xA5, 0x00}, {0xA6, 0x1D}, {0xA7, 0x00}, {0xA8, 0xA0}, {0xA9, 0x00},
                                               {0xAA, 0xDB}, {0xAB, 0x00}, {0xAC, 0x7E}, {0xAD, 0x00}};
            for (uint8_t i = 0; i < sizeof(scart_enable) / 2; i++) {
                xbox_smbus_input_byte(current_encoder_address, scart_enable[i][0], (uint8_t *)&temp);
                FOCUS_OUTPUT_BYTE(scart_enable[i][0], scart_enable[i][1]);
            }
        }

        // Set wide screen signalling for sdtv
        if (!is_ed) {
            FOCUS_OUTPUT_BYTE(0x85, (is_sd_pal50) ? 0x08 : 0x00);
            FOCUS_OUTPUT_BYTE(0x84, 0x00);
            FOCUS_OUTPUT_BYTE(0x83, (is_sd_pal50) ? 0x00 : 0x18);
            FOCUS_OUTPUT_BYTE(0x88, (is_sd_pal50) ? 0x08 : 0x00);
            FOCUS_OUTPUT_BYTE(0x87, 0x00);
            FOCUS_OUTPUT_BYTE(0x86, (is_sd_pal50) ? 0x00 : 0x18);
        }
    }

    break;
    default:
        return;
    }
}
