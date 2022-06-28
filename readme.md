example output.



[18:58:45:351] <0x1b>[0;32mI (1478) example: ⇥	SSR: cur_bus_width 0, discard_support 0, fule_support 0, reserved 0<0x1b>[0m␍␊
[18:58:45:415] lfs ok␍␊
[18:58:45:514] dir    0B .␍␊
[18:58:45:514] dir    0B ..␍␊
[18:58:45:514] reg   3MB Store.db␍␊
[18:58:45:514] reg  653B hi.txt␍␊
[18:58:45:514] esp32_FullPathname: Store.db␍␊
[18:58:45:514] esp32_Open: 0o Store.db r␍␊
[18:58:45:592] esp32_Open: 1o Store.db r+␍␊
[18:58:45:592] [SQLite3]Opening file Store.db, with flag 3␍␊
[18:58:45:656] [SQLite3]:File Store.db opened successfully,fd 0␍␊
[18:58:45:693] esp32_DeviceCharacteristics:␍␊
[18:58:45:693] esp32_DeviceCharacteristics:␍␊
[18:58:45:693] esp32_SectorSize:␍␊
[18:58:45:693] esp32_DeviceCharacteristics:␍␊
[18:58:45:693] esp32_SectorSize:␍␊
[18:58:45:693] esp32_Read: 1r Store.db 100 0[0] ␍␊
[18:58:45:693] [SQLite3]File position set ok, pos 0␍␊
[18:58:45:693] [SQLite3]Current offset 0, required amount 100␍␊
[18:58:45:996] [SQLite3]Readed 100 bytes␍␊
[18:58:45:996] esp32_Read: 3r Store.db 100 100 OK␍␊
[18:58:46:022] esp32_FileControl:␍␊
[18:58:46:022] esp32_FileControl:␍␊
[18:58:46:022] Opened database successfully␍␊
[18:58:46:022] SELECT main.Identity.rawData FROM main.Identity␍␊
[18:58:46:022] esp32_Lock:Not locked␍␊
[18:58:46:086] esp32_CheckReservedLock:␍␊
[18:58:46:107] [SQLite3]Get size of File: JanStore.db, size 3715584: size: 1073475812]␍␊
[18:58:46:107] esp32_Open: 0o Store.db-journal r␍␊
[18:58:46:171] esp32_Open: 1o Store.db-journal r+␍␊
[18:58:46:171] esp32_Open: 2o Store.db-journal MEM OK␍␊
[18:58:46:214] esp32mem_Read: Store.db-journal [0] [1] OK␍␊
[18:58:46:214] esp32mem_Close: Store.db-journal OK␍␊
[18:58:46:214] [SQLite3]Get size of File: Store.db, size 3715584: size: 1073475812]␍␊
 
[18:58:46:214] [SQLite3]File position set ok, pos 0␍␊
[18:58:46:214] [SQLite3]Current offset 0, required amount 512␍␊
[18:58:46:214] [SQLite3]Readed 512 bytes␍␊
 
[18:58:46:214] [SQLite3]File position set ok, pos 2560␍␊
[18:58:46:214] [SQLite3]Current offset 2560, required amount 512␍␊
[18:58:47:634] [SQLite3]Readed 512 bytes␍␊
 
[18:58:47:660] [SQLite3]File position set ok, pos 3072␍␊
[18:58:47:660] [SQLite3]Current offset 3072, required amount 512␍␊
[18:58:48:330] [SQLite3]Readed 512 bytes␍␊
 
[18:58:48:356] [SQLite3]File position set ok, pos 5120␍␊
[18:58:48:356] [SQLite3]Current offset 5120, required amount 512␍␊
[18:58:49:748] [SQLite3]Readed 512 bytes␍␊
 
[18:58:49:748] [SQLite3]File position set ok, pos 6656␍␊
[18:58:49:748] [SQLite3]Current offset 6656, required amount 512␍␊
[18:58:51:098] [SQLite3]Readed 512 bytes␍␊
 
[18:58:51:098] [SQLite3]File position set ok, pos 8192␍␊
[18:58:51:098] [SQLite3]Current offset 8192, required amount 512␍␊
[18:58:52:448] [SQLite3]Readed 512 bytes␍␊
 
[18:58:52:448] [SQLite3]File position set ok, pos 10240␍␊
[18:58:52:448] [SQLite3]Current offset 10240, required amount 512␍␊
[18:58:53:801] [SQLite3]Readed 512 bytes␍␊
 
[18:58:53:801] [SQLite3]File position set ok, pos 4096␍␊
[18:58:53:801] [SQLite3]Current offset 4096, required amount 512␍␊
[18:58:55:190] [SQLite3]Readed 512 bytes␍␊
 
[18:58:55:190] [SQLite3]File position set ok, pos 2148352␍␊
[18:58:55:190] [SQLite3]Current offset 2148352, required amount 512␍␊
[18:58:56:228] [SQLite3]Readed 512 bytes␍␊
 
[18:58:56:253] [SQLite3]File position set ok, pos 2131968␍␊
[18:58:56:253] [SQLite3]Current offset 2131968, required amount 512␍␊
[18:58:57:371] [SQLite3]Readed 512 bytes␍␊
 
[18:58:57:414] Callback function called: rawData = ++zp␍␊
[18:58:57:414] ␍␊
[18:58:57:414] Callback function called: rawData = +/zb␍␊
[18:58:57:414] ␍␊
[18:58:57:414] Callback function called: rawData = +1kp␍␊
[18:58:57:414] ␍␊
[18:58:57:414] Callback function called: rawData = +5ap␍␊
[18:58:57:414] ␍␊
[18:58:57:414] Callback function called: rawData = +6c5␍␊
[18:58:57:414] ␍␊
[18:58:57:414] Callback function called: rawData = +93L␍␊
[18:58:57:414] ␍␊
[18:58:57:414] Callback function called: rawData = +A2p␍␊
[18:58:57:414] ␍␊
[18:58:57:414] Callback function called: rawData = +DxL␍␊
[18:58:57:414] ␍␊
[18:58:57:414] Callback function called: rawData = +GIr␍␊
[18:58:57:414] ␍␊
[18:58:57:414] Callback function called: rawData = +GRK␍␊
[18:58:57:457] ␍␊
[18:58:57:457] Callback function called: rawData = +GSK␍␊
[18:58:57:457] ␍␊
[18:58:57:457] Callback function called: rawData = +GSa␍␊
[18:58:57:457] ␍␊
[18:58:57:457] Callback function called: rawData = +GTK␍␊
[18:58:57:457] ␍␊
[18:58:57:457] Callback function called: rawData = +GTa␍␊
