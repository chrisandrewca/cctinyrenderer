if not exist .\artifacts\* mkdir .\artifacts
if not exist .\artifacts\obj\* mkdir .\artifacts\obj

cl ^
/EHsc ^
/std:c++17 ^
/I..\ ^
/I..\deps ^
/Zi ^
/DDEBUG ^
/Fo.\artifacts\obj\ ^
    ..\deps\tinyobjloader\tiny_obj_loader.cc ^
    ..\tgaimage.cpp ^
    ..\main.cpp ^
/link ^
/out:.\artifacts\cctr.exe