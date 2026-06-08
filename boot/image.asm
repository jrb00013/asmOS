; Combined boot image: sector 0 bootsect + sector 1+ loader
%include "bootsect.asm"

[ORG 0x7E00]
%include "loader.asm"
