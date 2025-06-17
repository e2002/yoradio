Для использования МОД-а на WROOM, не имеющего дополнительной PSRAM
необходимо в папке проекта "..\yoRadio\src\audioI2S" заменить AAC-декодер.
Для этого:
1. папку "aac_decoder" со всем содержимым заменить одноимённой папкой
из архива "aac_decoder_WROOM.rar", находящегося там же.
2. В "myoptions.h" раскомментировать строку
"//#define WROOM_USED        /*  Set if use WROOM. (To add memory.) aac_decoder must be replased. */"